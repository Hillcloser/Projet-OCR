#ifndef IMAGE_H
#define IMAGE_H

#include <gtk/gtk.h>

// Structure principale de prétraitement
GdkPixbuf *pretraitement_image(GdkPixbuf *src);

// Fonctions individuelles (utiles pour le debug ou l'interface avancée)
void grayscale_image(GdkPixbuf *pixbuf);
void binarize_image_otsu(GdkPixbuf *pixbuf);
double detect_skew_angle(GdkPixbuf *pixbuf);
GdkPixbuf *rotate_pixbuf(GdkPixbuf *src, double angle);
void save_pixbuf(GdkPixbuf *pixbuf, const char *filename);
#endif
