# Projet-OCR  
  
----  
  
## Table des matières  
  
### Make  
  
### Detection  
  
### Image  
  
### NN  
  
### Solver  
  
----  
  
## Make  
  
Avant d'utiliser un des programmes suivants, il faut les compiler à l'aide des commandes suivantes :  
Pour tout compiler ```make all```  
Pour compiler Detection ```make detection```  
Pour compiler Image ```make image```  
Pour compiler NN ```make nn```  
Pour compiler Solver ```make solver```  
Pour supprimer les fichiers compilés ```make clean```  
  
----  
  
## Detection  
  
```./detection```  
  
----  
  
## Image  
  
Pour utiliser le fichier image.c il faut posséder le package gtk3 normalement préinstallé sur les PC de l'école.  
Le but de ce programme est de mettre en noir et blanc l'image envoyée en entrée et de la placer dans l'image de sortie,
et si besoin de faire une rotation de 90 degrés à l'image ainsi que de l'afficher. Le but de ce programme est de permettre une meilleure gestion de la grille
dans la suite du projet.  
Pour utiliser image.c, il suffit d'appeler ```./image (image_d'entrée) (image_de_sortie) (angle_de_rotation)```.  
L'image de sortie sera créée si elle n'existe pas, et si on ne veut pas de rotation, ne pas mettre de paramètre d'angle de rotation
ou mettre 0 dans l'angle de rotation.  
Pour compiler le fichier image.c, ajouter à la fin de la commande gcc ```pkg-config --cflags --libs gtk+-3.0 -lm``` pour l'utilisation de gtk3.  
```./image input_image.png output_image.png {angle_de_rotation_désiré}```  
  
----  
  
## NN  
  
test pour tous les cas```./nn```  
```./nn {0_ou_1} {0_ou_1}```  
```./nn {0_ou_1} {0_ou_1} {nombre_d'entraînements_du_NN_souhaité}```  
  
----  
  
## Solver  
  
Pour utiliser le fichier solver.c il faut simplement appeler ```./solver``` avec deux paramètres :  
le premier, le nom du fichier .txt contenant la grille dans laquelle chercher les mots, et ensuite le mot
à chercher dans la grille. Si le mot est trouvé, cela retournera les coordonnées de la première lettre et de la
dernière lettre, sinon, si le mot n'est pas trouvé, le programme retournera simplement **Not Found**.  
```./solver grid.txt {mot_à_chercher}```