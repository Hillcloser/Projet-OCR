#ifndef OCR_H
#define OCR_H

#include <gtk/gtk.h>

// Définitions du réseau (DOIT matcher ocr_engine.c)
#define INPUT_NODES 784   // 28x28
#define HIDDEN_NODES 128
#define OUTPUT_NODES 26   // A-Z

typedef struct {
    double *w1;
    double *b1;
    double *w2;
    double *b2;
    double *hidden_output;
    double *final_output;
} NeuralNet;

// Gestion du réseau
NeuralNet* create_network();
void free_network(NeuralNet *net);
void load_weights(NeuralNet *net, const char *filename);

// Traitement de la GRILLE (Cells/ -> grid.txt)
void process_grid_files(NeuralNet *net);

// Traitement des MOTS (Words/ -> words.txt) [NOUVEAU]
void process_word_files(NeuralNet *net);

#endif