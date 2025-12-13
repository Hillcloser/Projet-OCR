#ifndef DETECT_CUT_H
#define DETECT_CUT_H

#include <gtk/gtk.h>

// On définit les types
typedef enum {
    TYPE_GRID,  // Lettre de la grille
    TYPE_LIST,  // Lettre de la liste
    TYPE_NOISE  // Bruit (optionnel)
} LetterType;

typedef struct {
    int x, y, w, h;
    GdkPixbuf *img;
    LetterType type; // <--- NOUVEAU
} Letter;

void detect_grid(GdkPixbuf *image_base, size_t *length, Letter **result);
void save_letters_to_folder(Letter *letters, size_t count, const char *main_folder);

#endif