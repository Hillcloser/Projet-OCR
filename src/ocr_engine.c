
// src/ocr_engine.c (ALTERNATIF AUTONOME, SANS ocr.h)
// - Réseau de neurones simple (1 couche cachée), softmax, entraînement SGD
// - Prétraitement: crop + centrage dans 28x28 (comme MNIST-like)
// - Entrée: CSV Cells/train_labels.csv (format: filename,label)
// - Sortie: ocr_network.net (binaire: w1, b1, w2, b2 en double)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>                   // access()
#include <gdk-pixbuf/gdk-pixbuf.h>    // GdkPixbuf, PNG loader

// -------------------- Constantes réseau --------------------
#ifndef INPUT_NODES
#define INPUT_NODES   (28*28)
#endif
#ifndef HIDDEN_NODES
#define HIDDEN_NODES  128
#endif
#ifndef OUTPUT_NODES
#define OUTPUT_NODES  26   // 'A'..'Z'
#endif

// -------------------- Structure réseau --------------------
typedef struct {
    double *w1;  // [INPUT_NODES * HIDDEN_NODES]
    double *b1;  // [HIDDEN_NODES]
    double *w2;  // [HIDDEN_NODES * OUTPUT_NODES]
    double *b2;  // [OUTPUT_NODES]
    double *hidden_output; // [HIDDEN_NODES]
    double *final_output;  // [OUTPUT_NODES] (softmax)
} NeuralNet;

// -------------------- Utils math --------------------
static inline double sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }
static inline double urand01(void)    { return rand() / (double)RAND_MAX; }

// -------------------- Alloc / Free --------------------
static NeuralNet* create_network(void) {
    NeuralNet *net = (NeuralNet*)malloc(sizeof(NeuralNet));
    if (!net) return NULL;
    net->w1 = (double*)calloc(INPUT_NODES * HIDDEN_NODES, sizeof(double));
    net->b1 = (double*)calloc(HIDDEN_NODES, sizeof(double));
    net->w2 = (double*)calloc(HIDDEN_NODES * OUTPUT_NODES, sizeof(double));
    net->b2 = (double*)calloc(OUTPUT_NODES, sizeof(double));
    net->hidden_output = (double*)calloc(HIDDEN_NODES, sizeof(double));
    net->final_output  = (double*)calloc(OUTPUT_NODES, sizeof(double));
    if (!net->w1 || !net->b1 || !net->w2 || !net->b2 || !net->hidden_output || !net->final_output) {
        free(net->w1); free(net->b1); free(net->w2); free(net->b2);
        free(net->hidden_output); free(net->final_output); free(net);
        return NULL;
    }
    return net;
}

static void free_network(NeuralNet *net) {
    if (!net) return;
    free(net->w1); free(net->b1); free(net->w2); free(net->b2);
    free(net->hidden_output); free(net->final_output);
    free(net);
}

// -------------------- I/O des poids --------------------
static void save_weights(NeuralNet *net, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) { fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", filename); return; }
    fwrite(net->w1, sizeof(double), INPUT_NODES * HIDDEN_NODES, f);
    fwrite(net->b1, sizeof(double), HIDDEN_NODES, f);
    fwrite(net->w2, sizeof(double), HIDDEN_NODES * OUTPUT_NODES, f);
    fwrite(net->b2, sizeof(double), OUTPUT_NODES, f);
    fclose(f);
    printf("Poids sauvegardés dans %s\n", filename);
}

static void load_weights(NeuralNet *net, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) { fprintf(stderr, "Erreur: ouverture %s\n", filename); return; }
    fread(net->w1, sizeof(double), INPUT_NODES * HIDDEN_NODES, f);
    fread(net->b1, sizeof(double), HIDDEN_NODES, f);
    fread(net->w2, sizeof(double), HIDDEN_NODES * OUTPUT_NODES, f);
    fread(net->b2, sizeof(double), OUTPUT_NODES, f);
    fclose(f);
    printf("Réseau chargé depuis %s\n", filename);
}

// -------------------- Init Xavier --------------------
static void init_weights_xavier(NeuralNet *net) {
    double lim1 = sqrt(6.0 / (INPUT_NODES + HIDDEN_NODES));
    for (int i = 0; i < INPUT_NODES * HIDDEN_NODES; ++i)
        net->w1[i] = (urand01()*2.0 - 1.0) * lim1;
    for (int h = 0; h < HIDDEN_NODES; ++h) net->b1[h] = 0.0;

    double lim2 = sqrt(6.0 / (HIDDEN_NODES + OUTPUT_NODES));
    for (int i = 0; i < HIDDEN_NODES * OUTPUT_NODES; ++i)
        net->w2[i] = (urand01()*2.0 - 1.0) * lim2;
    for (int o = 0; o < OUTPUT_NODES; ++o) net->b2[o] = 0.0;
}

// -------------------- Inference --------------------
static void forward_pass(NeuralNet *net, double *input) {
    for (int h = 0; h < HIDDEN_NODES; ++h) {
        double sum = net->b1[h];
        for (int i = 0; i < INPUT_NODES; ++i)
            sum += input[i] * net->w1[i * HIDDEN_NODES + h];
        net->hidden_output[h] = sigmoid(sum);
    }
    double maxv = -1e30;
    for (int o = 0; o < OUTPUT_NODES; ++o) {
        double sum = net->b2[o];
        for (int h = 0; h < HIDDEN_NODES; ++h)
            sum += net->hidden_output[h] * net->w2[h * OUTPUT_NODES + o];
        net->final_output[o] = sum;
        if (sum > maxv) maxv = sum;
    }
    double s = 0.0;
    for (int o = 0; o < OUTPUT_NODES; ++o) {
        net->final_output[o] = exp(net->final_output[o] - maxv);
        s += net->final_output[o];
    }
    if (s <= 0.0) s = 1.0;
    for (int o = 0; o < OUTPUT_NODES; ++o)
        net->final_output[o] /= s;
}

static int get_prediction_index(NeuralNet *net) {
    int argmax = 0; double mv = -1.0;
    for (int o = 0; o < OUTPUT_NODES; ++o)
        if (net->final_output[o] > mv) { mv = net->final_output[o]; argmax = o; }
    return argmax;
}

// -------------------- Prétraitement 28x28 --------------------
static void preprocess_image(GdkPixbuf *img, double *input_vector) {
    for (int i = 0; i < INPUT_NODES; ++i) input_vector[i] = 0.0;

    int w = gdk_pixbuf_get_width(img);
    int h = gdk_pixbuf_get_height(img);
    guchar *pixels = gdk_pixbuf_get_pixels(img);
    int rowstride = gdk_pixbuf_get_rowstride(img);
    int n_channels = gdk_pixbuf_get_n_channels(img);

    int min_x = w, max_x = 0, min_y = h, max_y = 0, pixel_count = 0;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        guchar *p = pixels + y * rowstride + x * n_channels;
        if ((p[0] + p[1] + p[2]) / 3 < 200) { // encre
            if (x < min_x) min_x = x; if (x > max_x) max_x = x;
            if (y < min_y) min_y = y; if (y > max_y) max_y = y;
            pixel_count++;
        }
    }
    if (pixel_count < 2) return;

    int crop_w = max_x - min_x + 1;
    int crop_h = max_y - min_y + 1;
    int max_dim = (crop_w > crop_h) ? crop_w : crop_h;

    double scale = 20.0 / (double)max_dim;
    int scaled_w = (int)(crop_w * scale);
    int scaled_h = (int)(crop_h * scale);
    int offset_x = 4 + (20 - scaled_w) / 2;
    int offset_y = 4 + (20 - scaled_h) / 2;

    for (int sy = 0; sy < scaled_h; ++sy) for (int sx = 0; sx < scaled_w; ++sx) {
        int ox = min_x + (int)(sx / scale);
        int oy = min_y + (int)(sy / scale);
        if (ox >= w) ox = w - 1; if (oy >= h) oy = h - 1;
        guchar *p = pixels + oy * rowstride + ox * n_channels;
        double val = 1.0 - ((p[0] + p[1] + p[2]) / (3.0 * 255.0));
        input_vector[(offset_y + sy) * 28 + (offset_x + sx)] = val;
    }
}

// -------------------- OCR grille --------------------
static void process_grid_files(NeuralNet *net) {
    FILE *f_info = fopen("t/grid_info.txt", "r");
    if (!f_info) { printf("Erreur: t/grid_info.txt introuvable.\n"); return; }
    int rows, cols;
    if (fscanf(f_info, "%d %d", &rows, &cols) != 2) { fclose(f_info); printf("Format grid_info.txt invalide\n"); return; }
    fclose(f_info);

    FILE *f_out = fopen("grid.txt", "w");
    if (!f_out) return;

    double input[INPUT_NODES];
    printf("Processing Grid (%dx%d)...\n", rows, cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            char path[256];
            snprintf(path, sizeof(path), "t/cell_%02d_%02d.png", r, c);
            GdkPixbuf *img = gdk_pixbuf_new_from_file(path, NULL);
            char letter = ' ';
            if (img) {
                preprocess_image(img, input);
                forward_pass(net, input);
                letter = (char)('A' + get_prediction_index(net));
                g_object_unref(img);
            }
            fputc(letter, f_out);
        }
        fputc('\n', f_out);
    }
    fclose(f_out);
    printf("Fichier grid.txt généré.\n");
}

// -------------------- OCR mots --------------------
static char predict_letter_image(NeuralNet *net, GdkPixbuf *letter_img) {
    double input[INPUT_NODES];
    preprocess_image(letter_img, input);
    forward_pass(net, input);
    return (char)('A' + get_prediction_index(net));
}

static void segment_and_predict_word(NeuralNet *net, GdkPixbuf *word_img, FILE *f_out) {
    int w = gdk_pixbuf_get_width(word_img);
    int h = gdk_pixbuf_get_height(word_img);
    int rowstride = gdk_pixbuf_get_rowstride(word_img);
    int n_channels = gdk_pixbuf_get_n_channels(word_img);
    guchar *pixels = gdk_pixbuf_get_pixels(word_img);

    int *vproj = (int*)calloc(w, sizeof(int));
    for (int x = 0; x < w; ++x)
        for (int y = 0; y < h; ++y) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            if ((p[0] + p[1] + p[2]) / 3 < 200) vproj[x]++;
        }

    int start = -1;
    for (int x = 0; x < w; ++x) {
        int in_letter = (vproj[x] > 0);
        if (in_letter && start == -1) start = x;
        else if (!in_letter && start != -1) {
            int width = x - start;
            if (width > 2) {
                GdkPixbuf *sub = gdk_pixbuf_new_subpixbuf(word_img, start, 0, width, h);
                char c = predict_letter_image(net, sub);
                fputc(c, f_out); printf("%c", c);
                g_object_unref(sub);
            }
            start = -1;
        }
    }
    if (start != -1) {
        int width = w - start;
        if (width > 2) {
            GdkPixbuf *sub = gdk_pixbuf_new_subpixbuf(word_img, start, 0, width, h);
            char c = predict_letter_image(net, sub);
            fputc(c, f_out); printf("%c", c);
            g_object_unref(sub);
        }
    }
    free(vproj);
    fputc('\n', f_out); printf("\n");
}

static void process_word_files(NeuralNet *net) {
    FILE *f_out = fopen("words.txt", "w");
    if (!f_out) { printf("Erreur: Impossible de créer words.txt\n"); return; }
    printf("\n--- Traitement des Mots ---\n");
    for (int i = 0; ; ++i) {
        char path[256];
        snprintf(path, sizeof(path), "Words/mot_%02d.png", i);
        if (access(path, F_OK) != 0) break;
        GdkPixbuf *img = gdk_pixbuf_new_from_file(path, NULL);
        if (img) {
            printf("Mot %02d: ", i);
            segment_and_predict_word(net, img, f_out);
            g_object_unref(img);
        }
    }
    fclose(f_out);
    printf("Liste des mots sauvegardée dans 'words.txt'.\n");
}

// -------------------- Entraînement --------------------
static int label_to_index(char c) {
    c = (char)toupper((unsigned char)c);
    return (c >= 'A' && c <= 'Z') ? (c - 'A') : -1;
}

static double train_step(NeuralNet *net, const double *input, int y_idx, double lr) {
    forward_pass(net, (double*)input);

    double loss = 0.0;
    double delta_out[OUTPUT_NODES];
    for (int o = 0; o < OUTPUT_NODES; ++o) {
        double y = (o == y_idx) ? 1.0 : 0.0;
        double p = net->final_output[o]; if (p < 1e-12) p = 1e-12;
        loss += -y * log(p);
        delta_out[o] = (net->final_output[o] - y); // dL/dz (softmax+CE)
    }

    double delta_hid[HIDDEN_NODES];
    for (int h = 0; h < HIDDEN_NODES; ++h) {
        double sum = 0.0;
        for (int o = 0; o < OUTPUT_NODES; ++o)
            sum += net->w2[h * OUTPUT_NODES + o] * delta_out[o];
        double hact = net->hidden_output[h];
        delta_hid[h] = sum * hact * (1.0 - hact);
    }

    for (int h = 0; h < HIDDEN_NODES; ++h) {
        double hact = net->hidden_output[h];
        for (int o = 0; o < OUTPUT_NODES; ++o)
            net->w2[h * OUTPUT_NODES + o] -= lr * (hact * delta_out[o]);
    }
    for (int o = 0; o < OUTPUT_NODES; ++o) net->b2[o] -= lr * delta_out[o];

    for (int i = 0; i < INPUT_NODES; ++i) {
        double xi = input[i]; if (xi == 0.0) continue;
        for (int h = 0; h < HIDDEN_NODES; ++h)
            net->w1[i * HIDDEN_NODES + h] -= lr * (xi * delta_hid[h]);
    }
    for (int h = 0; h < HIDDEN_NODES; ++h) net->b1[h] -= lr * delta_hid[h];

    return loss;
}

static int load_preprocess_cell(const char *cells_dir, const char *fname, double *input) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", cells_dir, fname);
    GdkPixbuf *img = gdk_pixbuf_new_from_file(path, NULL);
    if (!img) return -1;
    preprocess_image(img, input);
    g_object_unref(img);
    return 0;
}

static void shuffle_indices(int *idx, int n) {
    for (int i = n - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int t = idx[i]; idx[i] = idx[j]; idx[j] = t;
    }
}

static int train_from_csv(NeuralNet *net, const char *csv_path, const char *cells_dir,
                          int epochs, double lr, const char *out_weights_path) {
    FILE *f = fopen(csv_path, "r");
    if (!f) { fprintf(stderr, "Erreur: ouverture %s\n", csv_path); return -1; }

    typedef struct { char fname[256]; char label; } Sample;
    int cap = 128, n = 0;
    Sample *samples = (Sample*)malloc(sizeof(Sample) * cap);
    if (!samples) { fclose(f); return -1; }

    char line[512]; int first = 1;
    while (fgets(line, sizeof(line), f)) {
        char *p = line; while (*p==' '||*p=='\t'||*p=='\r') ++p; if (!*p) continue;
        if (first && (strstr(p, "filename") || strstr(p, "label"))) { first = 0; continue; }
        first = 0;
        char *comma = strchr(p, ','); if (!comma) continue; *comma = '\0';
        char *fname = p, *lab = comma + 1;
        char *nl = strpbrk(lab, "\r\n"); if (nl) *nl = '\0';
        if (!*fname || !*lab) continue;
        char L = (char)toupper((unsigned char)lab[0]);
        if (L < 'A' || L > 'Z') continue;

        if (n == cap) { cap *= 2; Sample *tmp = (Sample*)realloc(samples, sizeof(Sample)*cap);
                        if (!tmp) { free(samples); fclose(f); return -1; } samples = tmp; }
        strncpy(samples[n].fname, fname, sizeof(samples[n].fname)-1);
        samples[n].fname[sizeof(samples[n].fname)-1] = '\0';
        samples[n].label = L; ++n;
    }
    fclose(f);
    if (n == 0) { fprintf(stderr, "Aucun échantillon dans %s\n", csv_path); free(samples); return -1; }

    int *order = (int*)malloc(sizeof(int)*n); if (!order) { free(samples); return -1; }
    for (int i = 0; i < n; ++i) order[i] = i;

    double input[INPUT_NODES];
    for (int e = 1; e <= epochs; ++e) {
        shuffle_indices(order, n);
        double sum_loss = 0.0; int correct = 0, seen = 0;

        for (int k = 0; k < n; ++k) {
            int idx = order[k];
            if (load_preprocess_cell(cells_dir, samples[idx].fname, input) != 0) {
                fprintf(stderr, "Ignoré: %s introuvable/illisible\n", samples[idx].fname);
                continue;
            }
            int y = label_to_index(samples[idx].label); if (y < 0) continue;
            double L = train_step(net, input, y, lr);
            sum_loss += L; ++seen;
            if (get_prediction_index(net) == y) ++correct;
        }

        double avg = (seen>0) ? sum_loss/seen : 0.0;
        double acc = (seen>0) ? (100.0*correct/seen) : 0.0;
        if (e == 1 || e % 10 == 0 || e == epochs)
            printf("Epoch %3d/%d  loss=%.4f  acc=%.2f%%  (samples=%d)\n", e, epochs, avg, acc, seen);
    }

    if (out_weights_path && *out_weights_path) save_weights(net, out_weights_path);
    free(order); free(samples);
    return 0;
}























// === Segmentation: exposer les "runs" (colonnes [start,end]) pour un mot ===
typedef struct { int start, end; } Run;

/* Renvoie un tableau alloué de segments (runs) "encre" pour word_img.
 * out_n reçoit le nombre de segments. L'appelant doit free(runs).
 * Logique identique à segment_and_predict_word(...) (seuil 200, width>2).
 */
static Run* get_letter_runs(GdkPixbuf *word_img, int *out_n) {
    int w = gdk_pixbuf_get_width(word_img);
    int h = gdk_pixbuf_get_height(word_img);
    int rowstride = gdk_pixbuf_get_rowstride(word_img);
    int n_channels = gdk_pixbuf_get_n_channels(word_img);
    guchar *pixels = gdk_pixbuf_get_pixels(word_img);

    // 1) Projection verticale (seuil 200)
    int *v_proj = calloc(w, sizeof(int));
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            if ((p[0] + p[1] + p[2]) / 3 < 200) v_proj[x]++;
        }
    }

    // 2) State machine pour runs
    Run *runs = malloc(sizeof(Run) * (w+1));
    int rn = 0;
    int start = -1;
    for (int x = 0; x < w; ++x) {
        int in_letter = (v_proj[x] > 0);
        if (in_letter && start == -1) {
            start = x;
        } else if (!in_letter && start != -1) {
            int end = x - 1;
            int width = end - start + 1;
            if (width > 2) runs[rn++] = (Run){start, end};
            start = -1;
        }
    }
    if (start != -1) {
        int end = w - 1;
        int width = end - start + 1;
        if (width > 2) runs[rn++] = (Run){start, end};
    }

    free(v_proj);
    *out_n = rn;
    return runs;
}

/* Construit un corpus de lettres à partir d'un CSV "Words/words_labels.csv" (filename,label_word).
 * Pour chaque mot, découpe en segments (runs), vérifie que rn == longueur(label),
 * puis extrait chaque sous-image de lettre, la prétraite (28x28) et stocke l'input+label.
 * Réutilise preprocess_image(...) existante pour garantir cohérence train/test. */
typedef struct {
    double input[INPUT_NODES];
    int label_idx; // 0..25
} LetterSample;

static int build_letter_corpus_from_words(const char *csv_path, const char *words_dir,
                                          LetterSample **out_samples, int *out_n) {
    FILE *f = fopen(csv_path, "r");
    if (!f) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", csv_path);
        return -1;
    }
    // Lire lignes CSV
    typedef struct { char fname[256]; char word[256]; } Row;
    int capR = 128, nR = 0;
    Row *rows = (Row*)malloc(sizeof(Row) * capR);
    if (!rows) { fclose(f); return -1; }

    char line[512]; int first = 1;
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p==' '||*p=='\t'||*p=='\r') ++p; if (!*p) continue;
        if (first && (strstr(p, "filename") || strstr(p, "label"))) { first = 0; continue; }
        first = 0;
        char *comma = strchr(p, ','); if (!comma) continue; *comma = '\0';
        char *fname = p, *lab = comma + 1;
        char *nl = strpbrk(lab, "\r\n"); if (nl) *nl = '\0';
        if (!*fname || !*lab) continue;

        if (nR == capR) {
            capR *= 2; Row *tmp = (Row*)realloc(rows, sizeof(Row)*capR);
            if (!tmp) { free(rows); fclose(f); return -1; }
            rows = tmp;
        }
        strncpy(rows[nR].fname, fname, sizeof(rows[nR].fname)-1);
        rows[nR].fname[sizeof(rows[nR].fname)-1] = '\0';
        // Upper-case & retirer espaces du label mot
        int wlen = 0;
        for (const char *q = lab; *q; ++q) if (*q != ' ')
            rows[nR].word[wlen++] = (char)toupper((unsigned char)*q);
        rows[nR].word[wlen] = '\0';
        nR++;
    }
    fclose(f);

    if (nR == 0) { fprintf(stderr, "CSV vide: %s\n", csv_path); free(rows); return -1; }

    // Construire corpus de lettres
    int capS = 1024, nS = 0;
    LetterSample *samples = (LetterSample*)malloc(sizeof(LetterSample)*capS);
    if (!samples) { free(rows); return -1; }

    for (int r = 0; r < nR; ++r) {
        // Charger image mot
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", words_dir, rows[r].fname);
        GdkPixbuf *img = gdk_pixbuf_new_from_file(path, NULL);
        if (!img) { fprintf(stderr, "Ignoré (image introuvable): %s\n", path); continue; }

        // Segments via la même logique que segment_and_predict_word(...)
        int rn = 0;
        Run *runs = get_letter_runs(img, &rn);
        int wlen = (int)strlen(rows[r].word);

        if (rn != wlen) {
            fprintf(stderr, "Mismatch segments/label: %s  (segments=%d, label_len=%d) -> ignoré\n",
                    rows[r].fname, rn, wlen);
            g_object_unref(img); free(runs); continue;
        }

        // Extraire chaque lettre + prétraiter avec preprocess_image(...)
        int h = gdk_pixbuf_get_height(img);
        for (int i = 0; i < rn; ++i) {
            int width = runs[i].end - runs[i].start + 1;
            if (width < 2) continue;
            GdkPixbuf *sub = gdk_pixbuf_new_subpixbuf(img, runs[i].start, 0, width, h);

            double in28[INPUT_NODES];
            preprocess_image(sub, in28);  // <-- même pipeline que l'inférence
            g_object_unref(sub);

            int yidx = rows[r].word[i] - 'A';
            if (yidx < 0 || yidx >= 26) continue;

            if (nS == capS) {
                capS *= 2;
                LetterSample *tmp = (LetterSample*)realloc(samples, sizeof(LetterSample)*capS);
                if (!tmp) { g_object_unref(img); free(runs); free(samples); free(rows); return -1; }
                samples = tmp;
            }
            memcpy(samples[nS].input, in28, sizeof(double)*INPUT_NODES);
            samples[nS].label_idx = yidx;
            nS++;
        }
        g_object_unref(img);
        free(runs);
    }

    free(rows);
    if (nS == 0) {
        fprintf(stderr, "Aucun échantillon lettre extrait depuis les mots.\n");
        free(samples);
        return -1;
    }

    *out_samples = samples;
    *out_n = nS;
    printf("Corpus lettres (mots) construit: %d échantillons\n", nS);
    return 0;
}

/* Entraînement depuis Words/words_labels.csv (lettres extraites des mots)
 * Utilise train_step(...) existante et forward_pass(...) pour l'accuracy. */
static void shuffle_indices_int(int *idx, int n) {
    for (int i = n - 1; i > 0; --i) { int j = rand() % (i + 1); int t = idx[i]; idx[i] = idx[j]; idx[j] = t; }
}

static int train_from_words_csv(NeuralNet *net, const char *csv_path, const char *words_dir,
                                int epochs, double lr, const char *out_weights_path) {
    LetterSample *samples = NULL; int nS = 0;
    if (build_letter_corpus_from_words(csv_path, words_dir, &samples, &nS) != 0) return -1;

    int *order = (int*)malloc(sizeof(int)*nS);
    if (!order) { free(samples); return -1; }
    for (int i = 0; i < nS; ++i) order[i] = i;

    for (int e = 1; e <= epochs; ++e) {
        shuffle_indices_int(order, nS);
        double sum_loss = 0.0; int correct = 0;

        for (int k = 0; k < nS; ++k) {
            int idx = order[k];
            double L = train_step(net, samples[idx].input, samples[idx].label_idx, lr);
            sum_loss += L;

            // Accuracy sur le batch courant
            forward_pass(net, samples[idx].input);
            if (get_prediction_index(net) == samples[idx].label_idx) ++correct;
        }
        double avg = sum_loss / (double)nS;
        double acc = 100.0 * correct / (double)nS;
        if (e == 1 || e % 10 == 0 || e == epochs)
            printf("Epoch %3d/%d  loss=%.4f  acc=%.2f%%  (letters=%d)\n", e, epochs, avg, acc, nS);
    }

    if (out_weights_path && *out_weights_path) save_weights(net, out_weights_path);

    free(order);
    free(samples);
    return 0;
}







    
    


   















// -------------------- MAIN (NO-ARG): Train + Save --------------------
int main(void) {
    

    srand((unsigned)time(NULL));
    NeuralNet *net = create_network();
    if (!net) { fprintf(stderr, "Erreur: allocation réseau\n"); return 2; }
	
    const char *CSV1      = "t/11/train_labels.csv";
    const char *CELLSDIR1 = "t/11";
	const char *CSVM1     = "t/11/train_words.csv";
    const char *WORDSDIR1 = "t/11";
	const char *CSV2      = "t/12/train_labels.csv";
    const char *CELLSDIR2 = "t/12";
	const char *CSVM2     = "t/12/train_words.csv";
    const char *WORDSDIR2 = "t/12";
	const char *CSV3      = "t/21/train_labels.csv";
    const char *CELLSDIR3 = "t/21";
	const char *CSVM3     = "t/21/train_words.csv";
    const char *WORDSDIR3 = "t/21";
    const char *OUTNET  = "ocr_network.net";
    int    EPOCHS = 30;
    double LR     = 0.05;
	
	
	//init_weights_xavier(net);
	load_weights(net, OUTNET);


    if (access(CSV1, F_OK) != 0) {
        fprintf(stderr, "Erreur: CSV1 introuvable: %s\n", CSV1);
        fprintf(stderr, "Format attendu: filename,label (ex: cell_00_00.png,S)\n");
        return 1;
    }
	    if (access(CSVM1, F_OK) != 0) {
        fprintf(stderr, "Erreur: CSVM1 introuvable: %s\n", CSVM1);
        fprintf(stderr, "Format attendu: filename,label (ex: mot_00.png,HELLO)\n");
        return 1;
    }
		if (access(CSV2, F_OK) != 0) {
        fprintf(stderr, "Erreur: CSV2 introuvable: %s\n", CSV2);
        fprintf(stderr, "Format attendu: filename,label (ex: cell_00_00.png,S)\n");
        return 1;
    }
	    if (access(CSVM2, F_OK) != 0) {
        fprintf(stderr, "Erreur: CSVM2 introuvable: %s\n", CSVM2);
        fprintf(stderr, "Format attendu: filename,label (ex: mot_00.png,HELLO)\n");
        return 1;
    }
		if (access(CSV3, F_OK) != 0) {
        fprintf(stderr, "Erreur: CSV3 introuvable: %s\n", CSV3);
        fprintf(stderr, "Format attendu: filename,label (ex: cell_00_00.png,S)\n");
        return 1;
    }
	    if (access(CSVM3, F_OK) != 0) {
        fprintf(stderr, "Erreur: CSVM3 introuvable: %s\n", CSVM3);
        fprintf(stderr, "Format attendu: filename,label (ex: mot_00.png,HELLO)\n");
        return 1;
    }
	
	
	printf("=== Entraînement ===\nCSV1: %s\nImages: %s\nSortie: %s\nEpochs: %d  LR: %.3f\n",
           CSV1, CELLSDIR1, OUTNET, EPOCHS, LR);
	printf("=== Entraînement (mots) ===\nCSVM1: %s\nDir: %s\nSortie: %s\nEpochs: %d  LR: %.3f\n",
           CSVM1, WORDSDIR1, OUTNET, EPOCHS, LR);
	printf("=== Entraînement ===\nCSV2: %s\nImages: %s\nSortie: %s\nEpochs: %d  LR: %.3f\n",
           CSV2, CELLSDIR2, OUTNET, EPOCHS, LR);
	printf("=== Entraînement (mots) ===\nCSVM2: %s\nDir: %s\nSortie: %s\nEpochs: %d  LR: %.3f\n",
           CSVM2, WORDSDIR2, OUTNET, EPOCHS, LR);
    printf("=== Entraînement ===\nCSV3: %s\nImages: %s\nSortie: %s\nEpochs: %d  LR: %.3f\n",
           CSV3, CELLSDIR3, OUTNET, EPOCHS, LR);
	printf("=== Entraînement (mots) ===\nCSVM3: %s\nDir: %s\nSortie: %s\nEpochs: %d  LR: %.3f\n",
           CSVM3, WORDSDIR3, OUTNET, EPOCHS, LR);

	for (int i =0; i<3; i++)
	{
		if (train_from_csv(net, CSV1, CELLSDIR1, EPOCHS, LR, OUTNET) != 0) {
			fprintf(stderr, "Echec de l'entraînement.\n");
			free_network(net);
			return 3;
		}
		    if (train_from_words_csv(net, CSVM1, WORDSDIR1, EPOCHS, LR, OUTNET) != 0) {
        fprintf(stderr, "Echec de l'entraînement sur mots.\n");
        free_network(net);
        return 3;
    }
		
		if (train_from_csv(net, CSV2, CELLSDIR2, EPOCHS, LR, OUTNET) != 0) {
			fprintf(stderr, "Echec de l'entraînement.\n");
			free_network(net);
			return 3;
		}
		    if (train_from_words_csv(net, CSVM2, WORDSDIR2, EPOCHS, LR, OUTNET) != 0) {
        fprintf(stderr, "Echec de l'entraînement sur mots.\n");
        free_network(net);
        return 3;
    }
		
		if (train_from_csv(net, CSV3, CELLSDIR3, EPOCHS, LR, OUTNET) != 0) {
			fprintf(stderr, "Echec de l'entraînement.\n");
			free_network(net);
			return 3;
		}
		    if (train_from_words_csv(net, CSVM3, WORDSDIR3, EPOCHS, LR, OUTNET) != 0) {
        fprintf(stderr, "Echec de l'entraînement sur mots.\n");
        free_network(net);
        return 3;
    }
		
	}
    printf("Terminé: poids sauvegardés dans '%s'.\n", OUTNET);
    free_network(net);
    return 0;
}
