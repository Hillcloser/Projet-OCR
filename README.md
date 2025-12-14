# Projet OCR

Ce projet implémente un OCR (Optical Character Recognition) avec interface graphique en GTK+3. Il fournit également un binaire de test pour vérifier le fonctionnement des composants internes.

## Structure du projet

```

.
├── include/              # Fichiers headers (.h)
├── src/                  # Code source (.c)
│   ├── detect_cut.c
│   ├── solver.c
│   ├── image.c
│   ├── ocr.c
│   ├── graphic_interface.c
│   └── test.c
├── build/                # Dossier de compilation pour les exécutables
├── Makefile              # Script de compilation
└── README.md

````

## Prérequis

- GCC
- GTK+3 (avec `pkg-config`)
- Make

Sous Debian/Ubuntu, vous pouvez installer les dépendances nécessaires avec :

```bash
sudo apt update
sudo apt install build-essential libgtk-3-dev pkg-config
````

## Compilation

Le projet utilise un Makefile pour automatiser la compilation. Les commandes disponibles sont :

* **Compiler l’OCR avec l’interface graphique :**

```bash
make ocr
```

* **Compiler et exécuter le test :**

```bash
make test
```

* **Compiler tous les exécutables :**

```bash
make all
```

* **Nettoyer le dossier `build/` :**

```bash
make clean
```

> Les exécutables sont générés dans le dossier `build/`.

Lancer l’interface graphique OCR directement :

```bash
./build/ocr
```

Exécuter les tests unitaires :

```bash
./build/test
```

## Notes

* Les flags de compilation incluent `-Wall -Wextra -g` pour activer les warnings et le debug.
* Le Makefile crée automatiquement le dossier `build/` si nécessaire.