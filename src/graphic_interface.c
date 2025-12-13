#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <gtk/gtk.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <cairo.h>

#include "image.h"
#include "detect_cut.h"
#include "ocr.h"
#include "solver.h"

/* --- VARIABLES GLOBALES --- */
static NeuralNet *ocr_brain = NULL;
static GtkWidget *image_widget = NULL;
static GdkPixbuf *original_pixbuf = NULL; // Image brute chargée
static GdkPixbuf *current_pixbuf = NULL;  // Image actuellement affichée

// Boutons (pour pouvoir les cacher/montrer)
static GtkWidget *pre_button = NULL;
static GtkWidget *detect_button = NULL;
static GtkWidget *solve_button = NULL;

/* ------------------- FONCTION UTILITAIRE : SAUVEGARDE ------------------- */
static void on_save_clicked(GtkWidget *widget, gpointer window)
{
    if (!current_pixbuf) {
        g_print("Aucune image à sauvegarder.\n");
        return;
    }

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Enregistrer le résultat", GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Enregistrer", GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "resultat.png");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gdk_pixbuf_save(current_pixbuf, filename, "png", NULL, NULL);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

/* ------------------- ÉTAPE 4 : RÉSOLUTION & AFFICHAGE ------------------- */
static void on_solve_clicked(GtkWidget *widget, gpointer data)
{
    if (!ocr_brain) {
        g_print("Erreur : Le réseau de neurones n'est pas chargé.\n");
        return;
    }

    // 1. Charger l'image propre de la grille (générée par detect_cut)
    GdkPixbuf *grid_display = NULL;
    
    if (access("grille.png", F_OK) != -1) {
        grid_display = gdk_pixbuf_new_from_file("grille.png", NULL);
    } else if (access("grilleleft.png", F_OK) != -1) {
        grid_display = gdk_pixbuf_new_from_file("grilleleft.png", NULL);
    } else {
        // Fallback : Si l'image n'existe pas, on prend ce qu'il y a à l'écran
        if (current_pixbuf) grid_display = gdk_pixbuf_copy(current_pixbuf);
    }

    if (!grid_display) {
        GtkWidget *err = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Impossible de charger l'image de la grille pour le dessin.");
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_destroy(err);
        return;
    }

    // 2. Préparation Cairo (Dessin sur l'image)
    int w = gdk_pixbuf_get_width(grid_display);
    int h = gdk_pixbuf_get_height(grid_display);

    // Création d'une surface Cairo
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(surface);

    // Peindre l'image de fond
    gdk_cairo_set_source_pixbuf(cr, grid_display, 0, 0);
    cairo_paint(cr);

    // Configurer le style du trait (Rouge semi-transparent)
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.6); 
    cairo_set_line_width(cr, 6.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    // 3. Exécution de l'OCR
    printf("--- Lancement OCR Grille ---\n");
    process_grid_files(ocr_brain); // Génère grid.txt
    
    printf("--- Lancement OCR Mots ---\n");
    process_word_files(ocr_brain); // Génère words.txt

    // 4. Récupérer les dimensions de la grille
    FILE *f_info = fopen("Cells/grid_info.txt", "r");
    int rows = 0, cols = 0;
    if (f_info) {
        fscanf(f_info, "%d %d", &rows, &cols);
        fclose(f_info);
    }

    if (rows == 0 || cols == 0) {
        printf("Erreur : Dimensions invalides dans grid_info.txt\n");
        // Nettoyage
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        g_object_unref(grid_display);
        return;
    }

    double cell_w = (double)w / cols;
    double cell_h = (double)h / rows;

    // 5. Résolution (Lecture des mots et appel du Solver)
    FILE *f_words = fopen("words.txt", "r");
    int count_found = 0;

    if (f_words) {
        char word_buf[100];
        while (fgets(word_buf, sizeof(word_buf), f_words)) {
            // Nettoyage du \n
            word_buf[strcspn(word_buf, "\n")] = 0;
            if (strlen(word_buf) < 2) continue;

            int res[4] = {0}; // y1, x1, y2, x2
            
            // APPEL SOLVER
            if (solver("grid.txt", word_buf, res)) {
                printf("MOT TROUVÉ : %s (%d,%d)->(%d,%d)\n", word_buf, res[1], res[0], res[3], res[2]);
                count_found++;

                // Calcul des coordonnées pixels (centre des cases)
                double x1 = res[1] * cell_w + (cell_w / 2.0);
                double y1 = res[0] * cell_h + (cell_h / 2.0);
                double x2 = res[3] * cell_w + (cell_w / 2.0);
                double y2 = res[2] * cell_h + (cell_h / 2.0);

                // Dessin
                cairo_move_to(cr, x1, y1);
                cairo_line_to(cr, x2, y2);
                cairo_stroke(cr);
            } else {
                printf("Mot non trouvé : %s\n", word_buf);
            }
        }
        fclose(f_words);
    } else {
        printf("Erreur : words.txt introuvable.\n");
    }

    // 6. Mise à jour de l'affichage
    if (current_pixbuf) g_object_unref(current_pixbuf);
    
    // Convertir la surface Cairo en Pixbuf affichable
    current_pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, w, h);
    gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), current_pixbuf);

    // Nettoyage ressources Cairo
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(grid_display);

    // Popup de fin
    char msg[256];
    snprintf(msg, sizeof(msg), "Analyse terminée !\n%d mots trouvés.", count_found);
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* ------------------- ÉTAPE 3 : SEGMENTATION ------------------- */
static void on_segmentation_clicked(GtkWidget *widget, gpointer data)
{
    // On segmente l'image actuellement affichée (le résultat du prétraitement)
    if (!current_pixbuf) return;
    
    // detect_cut_main2 renvoie 0 en cas de succès (standard C)
    // Assurez-vous d'avoir corrigé detect_cut.c pour qu'il reset ses compteurs static !
    int result = detect_cut_main2(current_pixbuf);

    if (result == 0) // Succès
    { 
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Segmentation réussie !\nGrille et mots isolés.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        // On débloque l'étape suivante
        gtk_widget_show(solve_button);
    } 
    else {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Échec de la segmentation.\nVérifiez l'image.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

/* ------------------- ÉTAPE 2 : PRÉTRAITEMENT ------------------- */
static void on_pretraitement(GtkWidget *widget, gpointer data)
{
    if (!original_pixbuf) return;

    if (current_pixbuf) g_object_unref(current_pixbuf);
    
    // Appel à votre fonction de filtrage (image.c)
    current_pixbuf = pretraitement_image(original_pixbuf);

    if (current_pixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), current_pixbuf);
        
        // On débloque l'étape suivante
        gtk_widget_show(detect_button);
    }
}

/* ------------------- ÉTAPE 1 : CHARGEMENT ------------------- */
void on_open_image(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Choisir une image",
        GTK_WINDOW(user_data), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Ouvrir", GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(filter);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        // Nettoyage mémoire
        if (original_pixbuf) g_object_unref(original_pixbuf);
        if (current_pixbuf) g_object_unref(current_pixbuf);

        original_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
        current_pixbuf = gdk_pixbuf_copy(original_pixbuf); // Copie pour affichage

        gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), current_pixbuf);

        // --- RESET DE L'ÉTAT ---
        // On affiche le bouton Prétraitement
        gtk_widget_show(pre_button);
        // On cache les étapes suivantes pour forcer le nouvel utilisateur à recommencer
        gtk_widget_hide(detect_button);
        gtk_widget_hide(solve_button);

        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

/* ------------------- MAIN ------------------- */
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // Initialisation IA
    ocr_brain = create_network();
    // Assurez-vous que ce fichier existe
    load_weights(ocr_brain, "ocr_network.net"); 

    // Fenêtre principale
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Word Search Solver");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Conteneur principal vertical
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // Barre de boutons horizontale
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(button_box), 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_box, FALSE, FALSE, 0);

    // Création des boutons
    GtkWidget *open_button = gtk_button_new_with_label("1. Charger Image");
    pre_button = gtk_button_new_with_label("2. Prétraitement");
    detect_button = gtk_button_new_with_label("3. Segmentation");
    solve_button = gtk_button_new_with_label("4. Résoudre");
    GtkWidget *save_button = gtk_button_new_with_label("Sauvegarder");

    // Ajout dans la boîte
    gtk_box_pack_start(GTK_BOX(button_box), open_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), pre_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), detect_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), solve_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), save_button, FALSE, FALSE, 0);

    // Zone Image avec barres de défilement
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    // L'image prend tout l'espace restant
    gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);

    image_widget = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), image_widget);

    // Connexion des signaux
    g_signal_connect(open_button, "clicked", G_CALLBACK(on_open_image), window);
    g_signal_connect(pre_button, "clicked", G_CALLBACK(on_pretraitement), NULL);
    g_signal_connect(detect_button, "clicked", G_CALLBACK(on_segmentation_clicked), NULL);
    g_signal_connect(solve_button, "clicked", G_CALLBACK(on_solve_clicked), NULL);
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked), window);

    // État initial : on cache les étapes intermédiaires
    gtk_widget_hide(pre_button);
    gtk_widget_hide(detect_button);
    gtk_widget_hide(solve_button);

    gtk_widget_show_all(window);
    
    // On re-cache après le show_all car show_all montre tout récursivement
    if (!original_pixbuf) {
        gtk_widget_hide(pre_button);
        gtk_widget_hide(detect_button);
        gtk_widget_hide(solve_button);
    }

    gtk_main();

    // Nettoyage final
    if (original_pixbuf) g_object_unref(original_pixbuf);
    if (current_pixbuf) g_object_unref(current_pixbuf);
    free_network(ocr_brain);

    return 0;
}