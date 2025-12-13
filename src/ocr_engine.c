#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#define INPUT_NODES 784   // 28x28
#define HIDDEN_NODES 128  // Assez large pour capturer les détails
#define OUTPUT_NODES 26   // A-Z
#define LEARNING_RATE 0.1 // Taux standard pour Sigmoide
#define EPOCHS 5

// Définition de la structure (identique à celle dans ocr_engine.c)
typedef struct {
    double *w1, *b1, *w2, *b2;
    double *hidden_input, *hidden_output;
    double *final_input, *final_output;
} NeuralNet;

double sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }
double sigmoid_prime(double x) { return x * (1.0 - x); } // x est déjà l'activation

// Initialisation aléatoire optimisée (Xavier/Glorot)
double init_weight(int n_inputs) {
    return ((double)rand() / RAND_MAX * 2.0 - 1.0) / sqrt(n_inputs);
}

// --- GESTION MEMOIRE ---

// Fonction pour grossir artificiellement les lettres (Dilatation)
// Cela transforme une lettre fine EMNIST en lettre "grasse" style grille
void augment_bold(double *input_img, double *output_img) {
    // On copie d'abord
    for(int i=0; i<784; i++) output_img[i] = input_img[i];

    // On applique la dilatation : un pixel devient blanc si un de ses voisins est blanc
    for (int y = 1; y < 27; y++) {
        for (int x = 1; x < 27; x++) {
            int idx = y * 28 + x;
            
            // Si un voisin est allumé, on allume le pixel central
            // (C'est l'inverse de l'érosion qu'on a fait avant !)
            if (input_img[(y-1)*28 + x] > 0.5 || 
                input_img[(y+1)*28 + x] > 0.5 || 
                input_img[y*28 + (x-1)] > 0.5 || 
                input_img[y*28 + (x+1)] > 0.5) {
                
                output_img[idx] = 1.0; 
            }
        }
    }
}


NeuralNet* create_network() {
    NeuralNet *net = malloc(sizeof(NeuralNet));
    
    net->w1 = malloc(INPUT_NODES * HIDDEN_NODES * sizeof(double));
    net->b1 = calloc(HIDDEN_NODES, sizeof(double));
    net->w2 = malloc(HIDDEN_NODES * OUTPUT_NODES * sizeof(double));
    net->b2 = calloc(OUTPUT_NODES, sizeof(double));

    net->hidden_input = calloc(HIDDEN_NODES, sizeof(double));
    net->hidden_output = calloc(HIDDEN_NODES, sizeof(double));
    net->final_input = calloc(OUTPUT_NODES, sizeof(double));
    net->final_output = calloc(OUTPUT_NODES, sizeof(double));

    // Initialisation des poids
    for(int i = 0; i < INPUT_NODES * HIDDEN_NODES; i++) 
        net->w1[i] = init_weight(INPUT_NODES);
    
    for(int i = 0; i < HIDDEN_NODES * OUTPUT_NODES; i++) 
        net->w2[i] = init_weight(HIDDEN_NODES);

    return net;
}

void free_network(NeuralNet *net) {
    free(net->w1); free(net->b1);
    free(net->w2); free(net->b2);
    free(net->hidden_input); free(net->hidden_output);
    free(net->final_input); free(net->final_output);
    free(net);
}

// --- MOTEUR IA ---

void forward_pass(NeuralNet *net, double *input) {
    // 1. Input -> Hidden
    for (int h = 0; h < HIDDEN_NODES; h++) {
        double sum = net->b1[h];
        for (int i = 0; i < INPUT_NODES; i++) {
            // Acces 1D: index = i * Largeur + h
            sum += input[i] * net->w1[i * HIDDEN_NODES + h];
        }
        net->hidden_input[h] = sum;
        net->hidden_output[h] = sigmoid(sum);
    }

    // 2. Hidden -> Output
    double max_val = -1e9; // Pour stabilité Softmax
    for (int o = 0; o < OUTPUT_NODES; o++) {
        double sum = net->b2[o];
        for (int h = 0; h < HIDDEN_NODES; h++) {
            sum += net->hidden_output[h] * net->w2[h * OUTPUT_NODES + o];
        }
        net->final_input[o] = sum;
        if (sum > max_val) max_val = sum;
    }

    // 3. Softmax
    double sum_exp = 0.0;
    for (int o = 0; o < OUTPUT_NODES; o++) {
        net->final_output[o] = exp(net->final_input[o] - max_val);
        sum_exp += net->final_output[o];
    }
    for (int o = 0; o < OUTPUT_NODES; o++) {
        net->final_output[o] /= sum_exp;
    }
}

void train_step(NeuralNet *net, double *input, int target_label) {
    // 1. Forward
    forward_pass(net, input);

    // 2. Calcul Erreurs
    double output_errors[OUTPUT_NODES];
    for (int o = 0; o < OUTPUT_NODES; o++) {
        double target = (o == target_label) ? 1.0 : 0.0;
        output_errors[o] = net->final_output[o] - target;
    }

    double hidden_errors[HIDDEN_NODES];
    for (int h = 0; h < HIDDEN_NODES; h++) {
        double error = 0.0;
        for (int o = 0; o < OUTPUT_NODES; o++) {
            error += output_errors[o] * net->w2[h * OUTPUT_NODES + o];
        }
        hidden_errors[h] = error * sigmoid_prime(net->hidden_output[h]);
    }

    // 3. Update Poids (Descente de Gradient)
    // Hidden -> Output
    for (int h = 0; h < HIDDEN_NODES; h++) {
        for (int o = 0; o < OUTPUT_NODES; o++) {
            net->w2[h * OUTPUT_NODES + o] -= LEARNING_RATE * output_errors[o] * net->hidden_output[h];
        }
    }
    for (int o = 0; o < OUTPUT_NODES; o++) {
        net->b2[o] -= LEARNING_RATE * output_errors[o];
    }

    // Input -> Hidden
    for (int i = 0; i < INPUT_NODES; i++) {
        for (int h = 0; h < HIDDEN_NODES; h++) {
            net->w1[i * HIDDEN_NODES + h] -= LEARNING_RATE * hidden_errors[h] * input[i];
        }
    }
    for (int h = 0; h < HIDDEN_NODES; h++) {
        net->b1[h] -= LEARNING_RATE * hidden_errors[h];
    }
}

// --- OUTILS CHARGEMENT ---

uint32_t swap_endian(uint32_t val) {
    return ((val << 24) & 0xFF000000) | ((val << 8) & 0x00FF0000) |
           ((val >> 8)  & 0x0000FF00) | ((val >> 24) & 0x000000FF);
}

void save_weights(NeuralNet *net, const char *filename) {
    FILE *f = fopen(filename, "wb"); // Mode Binaire
    if(!f) return;
    
    // On écrit les tableaux bruts, c'est ultra rapide
    fwrite(net->w1, sizeof(double), INPUT_NODES * HIDDEN_NODES, f);
    fwrite(net->b1, sizeof(double), HIDDEN_NODES, f);
    fwrite(net->w2, sizeof(double), HIDDEN_NODES * OUTPUT_NODES, f);
    fwrite(net->b2, sizeof(double), OUTPUT_NODES, f);
    
    fclose(f);
    printf("Sauvegarde terminee dans %s\n", filename);
}

// --- MAIN ---

int main() {
    srand(time(NULL));
    
    // 1. Ouverture Fichiers
    FILE *f_img = fopen("emnist-letters-train-images-idx3-ubyte", "rb");
    FILE *f_lbl = fopen("emnist-letters-train-labels-idx1-ubyte", "rb");
    
    if (!f_img || !f_lbl) {
        printf("ERREUR: Fichiers EMNIST introuvables !\n");
        return 1;
    }

    // 2. Lecture En-têtes
    uint32_t magic, num_items, rows, cols;
    fread(&magic, 4, 1, f_img);
    fread(&num_items, 4, 1, f_img); num_items = swap_endian(num_items);
    fread(&rows, 4, 1, f_img);
    fread(&cols, 4, 1, f_img);
    
    // Skip label header
    fseek(f_lbl, 8, SEEK_SET);

    printf("Chargement de %d images en memoire...\n", num_items);

    // 3. Chargement de TOUT en mémoire (pour mélanger vite)
    // Tableau géant de pixels : [Image1][Image2]...
    uint8_t *all_pixels = malloc(num_items * INPUT_NODES);
    uint8_t *all_labels = malloc(num_items);
    
    fread(all_pixels, 1, num_items * INPUT_NODES, f_img);
    fread(all_labels, 1, num_items, f_lbl);
    
    fclose(f_img);
    fclose(f_lbl);

    // 4. Préparation Indices pour Shuffle
    int *indices = malloc(num_items * sizeof(int));
    for(int i=0; i<num_items; i++) indices[i] = i;

    // 5. Création Réseau
    NeuralNet *net = create_network();
    double *input_buffer = malloc(INPUT_NODES * sizeof(double));

    
    

   

    // 6. Boucle Entrainement
    printf("Debut de l'entrainement (%d Epoques)...\n", EPOCHS);
    
    for (int e = 0; e < EPOCHS; e++) {
        // Shuffle (Mélange de Fisher-Yates)
        for (int i = num_items - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int temp = indices[i]; indices[i] = indices[j]; indices[j] = temp;
        }

        int correct = 0;
        
        for (int i = 0; i < num_items; i++) {
            int idx = indices[i]; // Index réel de l'image mélangée
            
            // Conversion 0-255 -> 0.0-1.0 et TRANSPOSEE (Rotation)
            // EMNIST est stocké [x][y], on veut [y][x] pour le remettre droit
            for(int r=0; r<28; r++) {
                for(int c=0; c<28; c++) {
                    // Source: image 'idx', pixel r,c
                    uint8_t val = all_pixels[idx * 784 + r * 28 + c];
                    // Destination: c * 28 + r (Rotation)
                    input_buffer[c * 28 + r] = val / 255.0;
                }
            }

            int target = all_labels[idx] - 1; // 1..26 -> 0..25
            
            // Protection données corrompues
            if (target < 0 || target >= 26) continue;


            // Train
            train_step(net, input_buffer, target);

            // Stat : Est-ce que la prédiction était bonne ?
            // On regarde juste l'index max de final_output
            int max_idx = 0;
            for(int k=1; k<26; k++) 
                if(net->final_output[k] > net->final_output[max_idx]) max_idx = k;
            
            if(max_idx == target) correct++;
        }

        printf("Epoch %d: Precision = %.2f%%\n", e+1, (double)correct / num_items * 100.0);
        save_weights(net, "ocr_network.net");
    }

    // Nettoyage
    free(all_pixels); free(all_labels); free(indices); free(input_buffer);
    free_network(net);
    
    return 0;
}



