#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // Pour toupper/tolower
#include "solver.h"

int solver(const char *file, const char *target_word, int res[4])
{
    if (file == NULL || target_word == NULL) return 0;

    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        printf("Erreur : Impossible d'ouvrir %s\n", file);
        return 0;
    }

    // 1. Chargement de la grille
    char grille[100][100];
    int lignes = 0;
    int colonnes = 0;
    char ligne_buf[256];

    while (fgets(ligne_buf, sizeof(ligne_buf), fp))
    {
        int col_temp = 0;
        for (int i = 0; ligne_buf[i] != '\0' && ligne_buf[i] != '\n'; i++)
        {
            // On ignore les espaces éventuels dans le fichier grid.txt
            if(ligne_buf[i] != ' ') {
                grille[lignes][col_temp] = ligne_buf[i];
                col_temp++;
            }
        }
        if (col_temp > colonnes) colonnes = col_temp;
        lignes++;
    }
    fclose(fp);

    // 2. Normalisation du mot (tout en majuscule pour matcher la grille OCR)
    char word[100];
    strcpy(word, target_word);
    
    // Nettoyage du \n à la fin du mot si présent
    word[strcspn(word, "\n")] = 0;

    for(int i = 0; word[i]; i++){
        word[i] = toupper(word[i]);
    }
    
    // Le Solver existant (logique de recherche)
    int len = strlen(word);
    if (len == 0) return 0;

    // Directions : {dy, dx}
    int dirs[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        { 0, -1},          { 0, 1},
        { 1, -1}, { 1, 0}, { 1, 1}
    };

    for (int i = 0; i < lignes; i++) {
        for (int j = 0; j < colonnes; j++) {
            
            // Si la première lettre correspond
            if (grille[i][j] == word[0]) {
                
                // On teste les 8 directions
                for (int d = 0; d < 8; d++) {
                    int k;
                    int curr_y = i + dirs[d][0];
                    int curr_x = j + dirs[d][1];

                    // Vérification du reste du mot
                    for (k = 1; k < len; k++) {
                        // Check bornes
                        if (curr_y < 0 || curr_y >= lignes || curr_x < 0 || curr_x >= colonnes)
                            break;
                        
                        if (grille[curr_y][curr_x] != word[k])
                            break;

                        curr_y += dirs[d][0];
                        curr_x += dirs[d][1];
                    }

                    // Si on a parcouru tout le mot (k == len)
                    if (k == len) {
                        res[0] = i; // y start
                        res[1] = j; // x start
                        res[2] = curr_y - dirs[d][0]; // y end (reculer d'un pas)
                        res[3] = curr_x - dirs[d][1]; // x end
                        
                        // printf("Trouvé: %s (%d,%d)->(%d,%d)\n", word, res[1], res[0], res[3], res[2]);
                        return 1; // Succès
                    }
                }
            }
        }
    }

    return 0; // Pas trouvé
}