#include <gtk/gtk.h>
#include "image.h"


static GtkWidget *image_widget = NULL;
static GdkPixbuf *original_pixbuf = NULL; // On garde l'original ici
static GdkPixbuf *current_pixbuf = NULL;  // Celle qu'on affiche

static GtkWidget *pre_button = NULL;
static GtkWidget *detect_button = NULL;
static GtkWidget *solve_button = NULL;


static GridMap current_grid = {0};













// Fonction appelée par le bouton "Sauvegarder"
static void on_save_clicked(GtkWidget *widget, gpointer window)
{
    if (!current_pixbuf) {
        printf("Aucune image à sauvegarder !\n");
        return;
    }

    // Création de la boîte de dialogue "Enregistrer sous"
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Enregistrer l'image",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Enregistrer", GTK_RESPONSE_ACCEPT,
        NULL);

    // Création d'un "Doorman" (Confirmation d'écrasement)
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    // On propose un nom par défaut
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "resultat.png");

    // Exécution de la boîte de dialogue
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // Appel de notre fonction backend
        save_pixbuf(current_pixbuf, filename);
        
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}












/*
static void on_segmentation(GtkWidget *widget, gpointer data)
{
    // Important : Il faut que l'image soit déjà prétraitée (Noir et Blanc) !
    if (!current_pixbuf) return;

    // Si une grille existait, on la nettoie
    if (current_grid.count > 0) free_grid(&current_grid);

    // 1. Détection
    current_grid = detect_grid(current_pixbuf);

    printf("Segmentation terminée : %d éléments trouvés.\n", current_grid.count);

    // 2. Visualisation (Boites vertes)
    GdkPixbuf *visu = draw_segmentation_boxes(current_pixbuf, &current_grid);

    // 3. Affichage
    gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), visu);

    g_object_unref(visu);
}
*/

/* ------------------- PRÉTRAITEMENT ------------------- */
static void on_pretraitement(GtkWidget *widget, gpointer data)
{
    // On repart TOUJOURS de l'original pour éviter la rotation infinie
    if (!original_pixbuf)
        return;

    // Si on a déjà un current, on le nettoie avant d'en créer un nouveau
    if (current_pixbuf)
        g_object_unref(current_pixbuf);

    current_pixbuf = pretraitement_image(original_pixbuf);

    if (!current_pixbuf)
        return;

    gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), current_pixbuf);
}

/* ------------------ CHARGEMENT DE L'IMAGE ------------------ */

void on_open_image(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Choisir une image",
        GTK_WINDOW(user_data),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Ouvrir", GTK_RESPONSE_ACCEPT,
        NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(filter);
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        // Nettoyage des anciennes images
        if (original_pixbuf) g_object_unref(original_pixbuf);
        if (current_pixbuf) g_object_unref(current_pixbuf);

        // Chargement dans l'original
        original_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
        
        // Au début, l'image courante est une simple copie de l'original
        current_pixbuf = gdk_pixbuf_copy(original_pixbuf);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), current_pixbuf);

        gtk_widget_show(pre_button);
        gtk_widget_show(detect_button);
        gtk_widget_show(solve_button);

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

/* --------------------------- MAIN --------------------------- */

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "OCR – Interface GTK3");
    
    // Taille fixe de départ, mais redimensionnable
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // --- Barre d'outils (Boutons) ---
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    // On met un peu de marge autour des boutons
    gtk_container_set_border_width(GTK_CONTAINER(button_box), 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_box, FALSE, FALSE, 0);

    GtkWidget *open_button = gtk_button_new_with_label("Charger");
    pre_button = gtk_button_new_with_label("Prétraitement");
    detect_button = gtk_button_new_with_label("Segmentation");
    solve_button = gtk_button_new_with_label("Résoudre");
    GtkWidget *save_button = gtk_button_new_with_label("Sauvegarder");
    
    
    
    gtk_box_pack_start(GTK_BOX(button_box), save_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), open_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), pre_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), detect_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), solve_button, FALSE, FALSE, 0);

    // --- Zone Image avec Scrollbars (ScrolledWindow) ---
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    // Politique : Automatique (les barres apparaissent si l'image est trop grande)
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    // L'image s'étendra dans tout l'espace restant
    gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);

    image_widget = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), image_widget);

    // --- Signaux ---
    g_signal_connect(open_button, "clicked", G_CALLBACK(on_open_image), window);
    g_signal_connect(pre_button, "clicked", G_CALLBACK(on_pretraitement), NULL);

    // Vérifie que tu as bien cette ligne quelque part avant gtk_widget_show_all
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked),window );

    // Masquage initial
    gtk_widget_hide(pre_button);
    gtk_widget_hide(detect_button);
    gtk_widget_hide(solve_button);

    gtk_widget_show_all(window);
    
    // On re-cache après le show_all sinon ils réapparaissent
    if(!original_pixbuf) {
        gtk_widget_hide(pre_button);
        gtk_widget_hide(detect_button);
        gtk_widget_hide(solve_button);
    }

    gtk_main();

    if (original_pixbuf) g_object_unref(original_pixbuf);
    if (current_pixbuf) g_object_unref(current_pixbuf);

    return 0;
}





