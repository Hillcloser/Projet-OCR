#include "ocr.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- OUTILS MATH --- */
static double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

/* --- GESTION MEMOIRE RESEAU --- */
NeuralNet* create_network() {
    NeuralNet *net = malloc(sizeof(NeuralNet));
    net->w1 = calloc(INPUT_NODES * HIDDEN_NODES, sizeof(double));
    net->b1 = calloc(HIDDEN_NODES, sizeof(double));
    net->w2 = calloc(HIDDEN_NODES * OUTPUT_NODES, sizeof(double));
    net->b2 = calloc(OUTPUT_NODES, sizeof(double));
    net->hidden_output = calloc(HIDDEN_NODES, sizeof(double));
    net->final_output = calloc(OUTPUT_NODES, sizeof(double));
    return net;
}

void free_network(NeuralNet *net) {
    if (!net) return;
    free(net->w1); free(net->b1);
    free(net->w2); free(net->b2);
    free(net->hidden_output); free(net->final_output);
    free(net);
}

void load_weights(NeuralNet *net, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier de poids %s\n", filename);
        return;
    }
    fread(net->w1, sizeof(double), INPUT_NODES * HIDDEN_NODES, f);
    fread(net->b1, sizeof(double), HIDDEN_NODES, f);
    fread(net->w2, sizeof(double), HIDDEN_NODES * OUTPUT_NODES, f);
    fread(net->b2, sizeof(double), OUTPUT_NODES, f);
    fclose(f);
    printf("Reseau charge depuis %s\n", filename);
}

/* --- MOTEUR D'INFERENCE --- */
static void forward_pass(NeuralNet *net, double *input) {
    for (int h = 0; h < HIDDEN_NODES; h++) {
        double sum = net->b1[h];
        for (int i = 0; i < INPUT_NODES; i++) {
            sum += input[i] * net->w1[i * HIDDEN_NODES + h];
        }
        net->hidden_output[h] = sigmoid(sum);
    }
    double max_val = -10000.0;
    for (int o = 0; o < OUTPUT_NODES; o++) {
        double sum = net->b2[o];
        for (int h = 0; h < HIDDEN_NODES; h++) {
            sum += net->hidden_output[h] * net->w2[h * OUTPUT_NODES + o];
        }
        net->final_output[o] = sum;
        if (sum > max_val) max_val = sum;
    }
    double sum_exp = 0.0;
    for (int o = 0; o < OUTPUT_NODES; o++) {
        net->final_output[o] = exp(net->final_output[o] - max_val);
        sum_exp += net->final_output[o];
    }
    for (int o = 0; o < OUTPUT_NODES; o++) {
        net->final_output[o] /= sum_exp;
    }
}

static int get_prediction_index(NeuralNet *net) {
    double max_prob = -1.0;
    int max_i = 0;
    for (int k = 0; k < OUTPUT_NODES; k++) {
        if (net->final_output[k] > max_prob) {
            max_prob = net->final_output[k];
            max_i = k;
        }
    }
    return max_i;
}

/* --- PREPROCESSING (CROP + CENTER 28x28) --- */
static void preprocess_image(GdkPixbuf *img, double *input_vector) {
    for (int i = 0; i < INPUT_NODES; i++) input_vector[i] = 0.0;
    
    int w = gdk_pixbuf_get_width(img);
    int h = gdk_pixbuf_get_height(img);
    guchar *pixels = gdk_pixbuf_get_pixels(img);
    int rowstride = gdk_pixbuf_get_rowstride(img);
    int n_channels = gdk_pixbuf_get_n_channels(img);

    int min_x = w, max_x = 0, min_y = h, max_y = 0;
    int pixel_count = 0;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            // Détection pixels noirs (seuil < 200)
            if ((p[0] + p[1] + p[2]) / 3 < 200) { 
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
                pixel_count++;
            }
        }
    }

    if (pixel_count < 2) return; // Trop peu de pixels pour une lettre

    int crop_w = max_x - min_x + 1;
    int crop_h = max_y - min_y + 1;
    int max_dim = (crop_w > crop_h) ? crop_w : crop_h;
    
    // Scale pour rentrer dans du 20x20
    double scale = 20.0 / (double)max_dim;
    int scaled_w = (int)(crop_w * scale);
    int scaled_h = (int)(crop_h * scale);

    int offset_x = 4 + (20 - scaled_w) / 2;
    int offset_y = 4 + (20 - scaled_h) / 2;

    for (int sy = 0; sy < scaled_h; sy++) {
        for (int sx = 0; sx < scaled_w; sx++) {
            int orig_x = min_x + (int)(sx / scale);
            int orig_y = min_y + (int)(sy / scale);
            if(orig_x >= w) orig_x = w - 1;
            if(orig_y >= h) orig_y = h - 1;

            guchar *p = pixels + orig_y * rowstride + orig_x * n_channels;
            double val = 1.0 - ((p[0] + p[1] + p[2]) / (3.0 * 255.0));
            input_vector[(offset_y + sy) * 28 + (offset_x + sx)] = val;
        }
    }
}

/* --- FONCTION PRINCIPALE : TRAITEMENT DE LA GRILLE --- */
void process_grid_files(NeuralNet *net) {
    FILE *f_info = fopen("Cells/grid_info.txt", "r");
    if (!f_info) {
        printf("Erreur: Cells/grid_info.txt introuvable.\n");
        return;
    }
    int rows, cols;
    fscanf(f_info, "%d %d", &rows, &cols);
    fclose(f_info);

    FILE *f_out = fopen("grid.txt", "w");
    if (!f_out) return;

    double input_vector[INPUT_NODES];
    printf("Processing Grid (%dx%d)...\n", rows, cols);

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            char filename[256];
            snprintf(filename, sizeof(filename), "Cells/cell_%02d_%02d.png", r, c);
            GdkPixbuf *img = gdk_pixbuf_new_from_file(filename, NULL);
            char letter = '?';
            
            if (img) {
                preprocess_image(img, input_vector);
                forward_pass(net, input_vector);
                letter = (char)('A' + get_prediction_index(net));
                g_object_unref(img);
            } else {
                letter = ' '; // Case vide ou erreur
            }
            fputc(letter, f_out);
        }
        fputc('\n', f_out);
    }
    fclose(f_out);
    printf("Fichier grid.txt généré.\n");
}

/* --- FONCTION PRINCIPALE : TRAITEMENT DES MOTS --- */

// Helper: Prédit une lettre isolée (déjà cropée)
static char predict_letter_image(NeuralNet *net, GdkPixbuf *letter_img) {
    double input[INPUT_NODES];
    preprocess_image(letter_img, input);
    forward_pass(net, input);
    return (char)('A' + get_prediction_index(net));
}

// Helper: Découpe un mot en lettres et écrit le résultat dans le fichier
static void segment_and_predict_word(NeuralNet *net, GdkPixbuf *word_img, FILE *f_out) {
    int w = gdk_pixbuf_get_width(word_img);
    int h = gdk_pixbuf_get_height(word_img);
    int rowstride = gdk_pixbuf_get_rowstride(word_img);
    int n_channels = gdk_pixbuf_get_n_channels(word_img);
    guchar *pixels = gdk_pixbuf_get_pixels(word_img);

    // 1. Projection Verticale
    int *v_proj = calloc(w, sizeof(int));
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            // Si pixel noir (encre)
            if ((p[0] + p[1] + p[2]) / 3 < 200) { 
                v_proj[x]++;
            }
        }
    }

    // 2. Détection des intervalles de lettres (State Machine)
    int start = -1;
    for (int x = 0; x < w; x++) {
        // Est-ce qu'on est dans une lettre ? (Au moins 1 pixel noir sur la colonne)
        int in_letter = (v_proj[x] > 0); 

        if (in_letter && start == -1) {
            // Début de lettre
            start = x;
        } 
        else if (!in_letter && start != -1) {
            // Fin de lettre détectée (gap blanc)
            int end = x;
            int width = end - start;

            // Filtre bruit : ignorer les trucs minuscules (< 2 pixels large)
            if (width > 2) {
                GdkPixbuf *letter_sub = gdk_pixbuf_new_subpixbuf(word_img, start, 0, width, h);
                
                // Prédiction
                char c = predict_letter_image(net, letter_sub);
                fputc(c, f_out); // Ecriture
                printf("%c", c); // Debug console
                
                g_object_unref(letter_sub);
            }
            start = -1; // Reset
        }
    }

    // Cas où la dernière lettre touche le bord droit
    if (start != -1) {
        int width = w - start;
        if (width > 2) {
            GdkPixbuf *letter_sub = gdk_pixbuf_new_subpixbuf(word_img, start, 0, width, h);
            char c = predict_letter_image(net, letter_sub);
            fputc(c, f_out);
            printf("%c", c);
            g_object_unref(letter_sub);
        }
    }

    free(v_proj);
    fputc('\n', f_out); // Fin du mot
    printf("\n");
}

void process_word_files(NeuralNet *net) {
    FILE *f_out = fopen("words.txt", "w");
    if (!f_out) {
        printf("Erreur: Impossible de créer words.txt\n");
        return;
    }

    printf("\n--- Traitement des Mots ---\n");
    
    // On suppose que detect_cut a généré mot_00.png, mot_01.png, etc.
    int i = 0;
    while (1) {
        char filename[256];
        snprintf(filename, sizeof(filename), "Words/mot_%02d.png", i);
        
        // On vérifie si le fichier existe
        if (access(filename, F_OK) == -1) {
            break; // Plus de mots, on arrête
        }

        GdkPixbuf *word_img = gdk_pixbuf_new_from_file(filename, NULL);
        if (word_img) {
            printf("Mot %02d: ", i);
            segment_and_predict_word(net, word_img, f_out);
            g_object_unref(word_img);
        }
        i++;
    }

    fclose(f_out);
    printf("Liste des mots sauvegardée dans 'words.txt'.\n");
}