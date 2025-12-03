#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <cairo.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================
   OUTILS DE BASE
   ============================================================ */

void save_pixbuf(GdkPixbuf *pixbuf, const char *filename)
{
    GError *error = NULL;
    // On force le format "png". Le dernier paramètre doit être NULL.
    if (!gdk_pixbuf_save(pixbuf, filename, "png", &error, NULL)) {
        fprintf(stderr, "Erreur lors de la sauvegarde : %s\n", error->message);
        g_error_free(error);
    } else {
        printf("Image sauvegardée avec succès : %s\n", filename);
    }
}

static inline guchar clamp_val(double v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (guchar)v;
}

void grayscale_image(GdkPixbuf *pixbuf) {
    int w = gdk_pixbuf_get_width(pixbuf);
    int h = gdk_pixbuf_get_height(pixbuf);
    int rs = gdk_pixbuf_get_rowstride(pixbuf);
    int ch = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    if (ch < 3) return;

    for (int y = 0; y < h; y++) {
        guchar *row = pixels + y * rs;
        for (int x = 0; x < w; x++) {
            guchar *p = row + x * ch;
            guchar g = (guchar)(0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2]);
            p[0] = p[1] = p[2] = g;
        }
    }
}

void enhance_contrast(GdkPixbuf *pixbuf) {
    // Cette étape aide Hough à mieux voir les lignes de la grille
    int w = gdk_pixbuf_get_width(pixbuf);
    int h = gdk_pixbuf_get_height(pixbuf);
    int rs = gdk_pixbuf_get_rowstride(pixbuf);
    int ch = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    guchar min = 255, max = 0;
    for (int y = 0; y < h; y++) {
        guchar *row = pixels + y * rs;
        for (int x = 0; x < w; x++) {
            guchar val = row[x * ch];
            if (val < min) min = val;
            if (val > max) max = val;
        }
    }

    if (max <= min) return;
    double factor = 255.0 / (max - min);

    for (int y = 0; y < h; y++) {
        guchar *row = pixels + y * rs;
        for (int x = 0; x < w; x++) {
            guchar *p = row + x * ch;
            p[0] = p[1] = p[2] = clamp_val((p[0] - min) * factor);
        }
    }
}

void binarize_image_otsu(GdkPixbuf *pixbuf) {
    int w = gdk_pixbuf_get_width(pixbuf);
    int h = gdk_pixbuf_get_height(pixbuf);
    int rs = gdk_pixbuf_get_rowstride(pixbuf);
    int ch = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    int hist[256] = {0};
    int total = w * h;

    for (int y = 0; y < h; y++) {
        guchar *row = pixels + y * rs;
        for (int x = 0; x < w; x++) {
            hist[row[x * ch]]++;
        }
    }

    float sum = 0;
    for (int i = 0; i < 256; i++) sum += i * hist[i];

    float sumB = 0;
    int wB = 0;
    int wF = 0;
    float varMax = 0;
    int threshold = 128;

    for (int t = 0; t < 256; t++) {
        wB += hist[t];
        if (wB == 0) continue;
        wF = total - wB;
        if (wF == 0) break;

        sumB += (float)(t * hist[t]);
        float mB = sumB / wB;
        float mF = (sum - sumB) / wF;
        float varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);

        if (varBetween > varMax) {
            varMax = varBetween;
            threshold = t;
        }
    }

    for (int y = 0; y < h; y++) {
        guchar *row = pixels + y * rs;
        for (int x = 0; x < w; x++) {
            guchar *p = row + x * ch;
            guchar val = (p[0] <= threshold) ? 0 : 255;
            p[0] = p[1] = p[2] = val;
        }
    }
}

/* ============================================================
   NOUVELLE DÉTECTION D'ANGLE : TRANSFORMÉE DE HOUGH
   ============================================================ */
double detect_skew_angle(GdkPixbuf *pixbuf)
{
    // 1. Préparation : On a besoin des Bords (Edges)
    // On travaille sur une copie
    GdkPixbuf *temp = gdk_pixbuf_copy(pixbuf);
    grayscale_image(temp);
    // Un léger flou pour réduire le bruit (optionnel mais aidait)
    binarize_image_otsu(temp); 

    int w = gdk_pixbuf_get_width(temp);
    int h = gdk_pixbuf_get_height(temp);
    int rs = gdk_pixbuf_get_rowstride(temp);
    int ch = gdk_pixbuf_get_n_channels(temp);
    guchar *pixels = gdk_pixbuf_get_pixels(temp);

    // Paramètres Hough
    // On cherche l'angle theta entre -45 et +45 degrés
    // Précision : 0.5 degré
    int num_angles = 180;
    double angle_step = 0.5;
    double min_angle = -45.0;
    
    double max_rho = sqrt(w*w + h*h);
    int num_rho = (int)(max_rho * 2) + 1;

    unsigned int *accumulator = calloc(num_angles * num_rho, sizeof(unsigned int));

    double *sin_table = malloc(num_angles * sizeof(double));
    double *cos_table = malloc(num_angles * sizeof(double));
    for (int i = 0; i < num_angles; i++) {
        double theta = (min_angle + i * angle_step) * M_PI / 180.0;
        sin_table[i] = sin(theta);
        cos_table[i] = cos(theta);
    }

    int step = 2; 

    for (int y = step; y < h - step; y += step) {
        for (int x = step; x < w - step; x += step) {
            guchar val = pixels[y * rs + x * ch];
            
            if (val == 0) {
                guchar up = pixels[(y-1) * rs + x * ch];
                guchar left = pixels[y * rs + (x-1) * ch];
                if (up == 255 || left == 255) {
                    
                    for (int t = 0; t < num_angles; t++) {
                        double rho = x * cos_table[t] + y * sin_table[t];
                        
                        int rho_idx = (int)(rho + max_rho);
                        
                        if (rho_idx >= 0 && rho_idx < num_rho) {
                            accumulator[t * num_rho + rho_idx]++;
                        }
                    }
                }
            }
        }
    }

    unsigned int max_votes = 0;
    int best_angle_idx = -1;

    for (int t = 0; t < num_angles; t++) {
        for (int r = 0; r < num_rho; r++) {
            if (accumulator[t * num_rho + r] > max_votes) {
                max_votes = accumulator[t * num_rho + r];
                best_angle_idx = t;
            }
        }
    }

    free(accumulator);
    free(sin_table);
    free(cos_table);
    g_object_unref(temp);

    // Résultat
    if (best_angle_idx != -1) {
        return min_angle + best_angle_idx * angle_step;
    }

    return 0.0;
}

GdkPixbuf *rotate_pixbuf(GdkPixbuf *src, double angle_deg)
{
    if (fabs(angle_deg) < 0.1) return gdk_pixbuf_copy(src);

    double angle = angle_deg * (M_PI / 180.0);
    int w = gdk_pixbuf_get_width(src);
    int h = gdk_pixbuf_get_height(src);

    double cos_a = fabs(cos(angle));
    double sin_a = fabs(sin(angle));
    int new_w = ceil(w * cos_a + h * sin_a);
    int new_h = ceil(h * cos_a + w * sin_a);

    cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_w, new_h);
    cairo_t *cr = cairo_create(surf);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_translate(cr, new_w / 2.0, new_h / 2.0);
    cairo_rotate(cr, angle);
    cairo_translate(cr, -w / 2.0, -h / 2.0);

    gdk_cairo_set_source_pixbuf(cr, src, 0, 0);
    cairo_paint(cr);

    GdkPixbuf *res = gdk_pixbuf_get_from_surface(surf, 0, 0, new_w, new_h);
    
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    return res;
}

GdkPixbuf *pretraitement_image(GdkPixbuf *src)
{
    double angle = detect_skew_angle(src);
    
    
    GdkPixbuf *rotated = NULL;
    if (fabs(angle) > 0.5) {
        rotated = rotate_pixbuf(src, -angle);
    } else {
        rotated = gdk_pixbuf_copy(src);
    }

    grayscale_image(rotated);
    enhance_contrast(rotated);
    binarize_image_otsu(rotated);

    return rotated;
}
