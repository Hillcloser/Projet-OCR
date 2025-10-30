# Projet-OCR


Un fichier README décrivant de manière concise comment utiliser votre application. Ce fichier devra
être au format texte et pouvoir être lu correctement dans un terminal avec une commande comme cat
ou less. Il pourra éventuellement être au format markdown ;


Pour le utiliser le fichier solver.c il faut simplement appeller ./solver avec deux parametre 
le premier le nom du fichier.txt contenant la grille dans laquelle chercher les mots et ensuite le mot
a chercher dans la grille si le mot est trouve cela retournera les coordonnées de la premier lettre et de la
derniere lettre sinon si le mot n'est pas trouve le programme retournera simplement Not Found


Pour utiliser le fichier image.c il faut posseder le package gtk3 normalement preinstaller sur les pcs de l'ecole.
Le but de ce programme est de mettre en noir et blanc l'image envoye en entree et de la mettre dans l'image de sortie
et si besoin de faire une rotation de 90 degree a l'image ainsi que l'afficher. Le but de ce programme est de permettre une meilleur gestion de la grille
dans la suite du projet.
Pour utiliser image.c simplement appeler ./image (image d'entrée) (image de sortie) (angle de la rotation).
L'image de sortie sera crée si elle n'existe pas et si on ne veut pas de rotation ne simplement pas mettre de parametre angle de rotation
ou mettre 0 dans angle de rotation
Pour compiler le fichier image.c ajouter a la fin de la commande gcc "`pkg-config --cflags --libs gtk+-3.0 cairo' -lm" pour l'utilisation de gtk3