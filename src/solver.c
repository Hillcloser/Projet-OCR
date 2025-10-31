#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main(int argc , char *argv[])
{
  if (argc != 3 )
  {
    return EXIT_FAILURE;
  }
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL)
  {
    printf("Le Fichier na pas put etre ouvert\n");
      return EXIT_FAILURE;
  }
  int lignes = 0;
  int colonnes = 0;
  char grille [100][100];
  char ligne[256];
  char* word = argv[2];
  while ( fgets(ligne, sizeof(ligne), fp))
  {
    colonnes = 0;
    for (int i = 0 ; ligne[i] != '\0' ; i++)
    {
      grille[lignes][colonnes] = ligne[i];
      colonnes++;
    }
    lignes++;
  }
  fclose(fp);
  for (int letter = 0 ; word[letter] != '\0' ; letter++)
  {
    if (word[letter] >= 'a' && word[letter] <= 'z')
    {
      int offset = 'a' - 'A';
      word[letter] = (word[letter] - offset);

    }

  }
  int first_pos_x = 0;
  int first_pos_y = 0 ;
  int len = strlen(word);
  int trouve = 0;
  for (int i = 0 ; i < lignes; i++)
  {
    for (int j = 0; j < colonnes ; j++)
    {
        if (word[0] == grille[i][j])
        {
          first_pos_x = i;
          first_pos_y = j;
          int k = 0;
          int new_i = i;
          int new_j = j;
          int letter = 1;
          if (grille[new_i-1][new_j-1] == word[letter])
              {
                new_i--;
                new_j--;
               int save_i = new_i;
               int save_j = new_j;

                for (int letter = 1; letter<len ; letter++)
                {
                  if (grille[new_i][new_j] == word[letter])
                  {
                    k++;
                    new_i = new_i -1;
                    new_j = new_j-1;
                  }
                  else{
                    new_i =save_i;
                    new_j =save_j;
                    k=0;
                    break;
                  }
                }
                new_i++;
                new_j++;

             }
             if (grille[new_i-1][new_j] == word[letter])
              { 
                new_i--;
                int save_i = new_i;
                int save_j = new_j;
                for (int letter = 1; letter<len ; letter++)
                { 
                  if (grille[new_i][new_j] == word[letter])
                  {
                    k++;
                    new_i = new_i - 1;
                  }
                  else{
                    new_i =save_i;
                    new_j =save_j;
                    k=0;
                    break;
                  }
                }
                new_i++;
             }
             if (grille[new_i-1][new_j+1] == word[letter])
              {
                new_i--;
                new_j++;
                int save_i = new_i;
                int save_j = new_j;
                for (int letter = 1; letter<len ; letter++)
                {
                  if (grille[new_i][new_j] == word[letter])
                  {
                    k++;
                    new_i = new_i - 1;
                    new_j = new_j+1;
                  }
                  else{
                    new_i =save_i;
                    new_j =save_j;
                    k=0;
                    break;
                  }
                }
                new_i++;
                new_j--;
             }
            if (grille[new_i][new_j-1] == word[letter])
              {
                new_j--;
                int save_i = new_i;
                int save_j = new_j;
                for (int letter = 1; letter<len ; letter++)
                {
                  if (grille[new_i][new_j] == word[letter])
                  {
                    k++;
                    new_j = new_j-1;
                  }
                  else{
                    new_i =save_i;
                    new_j =save_j;
                    k=0;
                    break;
                  }
                }
                new_j++;
              }
          if (grille[new_i][new_j+1] == word[letter])
              {
                new_j++;
                int save_i = new_i;
                int save_j = new_j;
                for (int letter = 1; letter<len ; letter++)
                {
                  if (grille[new_i][new_j] == word[letter])
                  {
                    k++;
                    new_j = new_j+1;
                  }
                  else{
                    new_i =save_i;
                    new_j =save_j;
                    k=0;
                    break;
                  }
                }
                new_j--;
             }
          if (grille[new_i+1][new_j-1] == word[letter])
              {
                new_i++;
                new_j--;
                int save_i = new_i;
                int save_j = new_j;
                for (int letter = 1; letter<len ; letter++)
                {
                  if (grille[new_i][new_j] == word[letter])
                  {
                    k++;
                    new_i = new_i + 1;
                    new_j = new_j-1;
                  }
                  else{
                    new_i =save_i;
                    new_j =save_j;
                    k=0;
                    break;
                  }
                }
                new_i--;
                new_j++;
             }
          if (grille[new_i+1][new_j] == word[letter])
              {
                new_i++;
                int save_i = new_i;
                int save_j = new_j;
                for (int letter = 1; letter<len ; letter++)
                {
                  if (grille[new_i][new_j] == word[letter])
                  {
                    k++;
                    new_i = new_i + 1;
                  }
                  else{
                    new_i =save_i;
                    new_j =save_j;
                    k=0;
                    break;
                  }
                }
                new_i--;
             }
          if (grille[new_i+1][new_j+1] == word[letter])
              {
                new_i++;
                new_j++;
                int save_i = new_i;
                int save_j = new_j;
                for (int letter = 1; letter<len ; letter++)
                {
                  if (grille[new_i][new_j] == word[letter])
                  {
                    k++;
                    new_i = new_i + 1;
                    new_j = new_j+1;
                  }
                  else{
                    new_i =save_i;
                    new_j =save_j;
                    k=0;
                    break;
                  }
                }
                new_i--;
                new_j--;
              }
          if (k == len-1)
          {
            trouve = 1;
            printf("(%i,%i)(%i,%i)\n",first_pos_y,first_pos_x,new_j,new_i);
            return 0;
          }
          
        }
      }
  }
  if (trouve == 0)
  {
      printf("Not Found");
      return 0;
  }
}