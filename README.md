# ImageUniform

## Uniformisation d'images

Projet universitaire réalisé en **C** autour du traitement d'images, de la représentation des pixels en mémoire et de la construction d'un **arbre de Zpixels** permettant de représenter une image par zones de plus en plus homogènes.

Le projet avait pour objectif de construire une chaîne complète allant de la représentation bas niveau d'une image jusqu'à son analyse, sa décomposition récursive et sa projection graphique.

> **Projet universitaire — C — Traitement d'image — Arbres — GTK — GLib — Tests unitaires — GitLab**

---

## Sommaire

* [Présentation](#présentation)
* [Objectifs](#objectifs)
* [Principe général](#principe-général)
* [Concept de Zpixel](#concept-de-zpixel)
* [Construction de l'arbre de Zpixels](#construction-de-larbre-de-zpixels)
* [Uniformisation d'une image](#uniformisation-dune-image)
* [Évaluation de la dégradation](#évaluation-de-la-dégradation)
* [Représentation d'une image en mémoire](#représentation-dune-image-en-mémoire)
* [Gestion du RGB](#gestion-du-rgb)
* [Rowstride et padding](#rowstride-et-padding)
* [Parcours de l'arbre](#parcours-de-larbre)
* [Architecture du projet](#architecture-du-projet)
* [Chaîne de compilation](#chaîne-de-compilation)
* [Tests](#tests)
* [Débogage](#débogage)
* [Interface graphique](#interface-graphique)
* [Organisation pédagogique](#organisation-pédagogique)
* [Technologies et compétences](#technologies-et-compétences)
* [Contexte](#contexte)

---

# Présentation

**Uniformisation d'images** est un projet de programmation en C consacré à la représentation et à la simplification d'images.

L'idée centrale consiste à représenter une image non pas uniquement comme une suite de pixels indépendants, mais comme une hiérarchie de **zones rectangulaires appelées Zpixels**.

Une zone de l'image peut être considérée comme suffisamment homogène lorsque les valeurs RGB de ses pixels sont suffisamment proches. Dans ce cas, la zone peut être représentée par une couleur moyenne plutôt que par l'ensemble de ses pixels.

Lorsqu'une zone n'est pas suffisamment homogène, elle est divisée en plusieurs sous-zones qui sont ensuite analysées récursivement.

Le principe général peut être résumé ainsi :

```text
                         IMAGE
                           │
                           ▼
                    ┌─────────────┐
                    │   Zpixel    │
                    │  toute      │
                    │  l'image    │
                    └──────┬──────┘
                           │
                     zone non uniforme
                           │
                           ▼
                  ┌────┬───┴───┬────┐
                  ▼    ▼       ▼    ▼
                 Z1   Z2      Z3   Z4
                  │    │       │    │
                  ▼    ▼       ▼    ▼
                analyse récursive
```

Cette organisation conduit naturellement à une représentation sous forme **d'arbre**, avec un Zpixel correspondant à un nœud de l'arbre.

---

# Objectifs

Le projet était organisé autour de plusieurs étapes :

* comprendre la représentation d'une image en mémoire ;
* gérer les pixels RGB ;
* créer et détruire dynamiquement une image ;
* accéder aux pixels par coordonnées ;
* comprendre le `rowstride` ;
* gérer le padding de chaque ligne ;
* construire une représentation en Zpixels ;
* construire un arbre de Zpixels ;
* parcourir et évaluer cet arbre ;
* calculer une représentation moyenne d'une zone ;
* définir un seuil de dégradation ;
* projeter les Zpixels sur une image ;
* développer une interface graphique ;
* intégrer les différents modules ;
* écrire et exécuter des tests ;
* utiliser `gdb` pour le débogage ;
* utiliser GitLab pour la gestion de versions.

---

# Principe général

Le traitement repose sur une idée simple :

> Une zone suffisamment uniforme peut être remplacée par une représentation simplifiée de sa couleur.

Pour chaque Zpixel, les composantes RGB sont calculées à partir de la zone correspondante :

```text
R = moyenne(R)
V = moyenne(V)
B = moyenne(B)
```

Le Zpixel peut alors être représenté par une couleur moyenne.

Si la zone n'est pas suffisamment uniforme, elle est divisée en quatre sous-zones.

Le processus est répété jusqu'à atteindre une taille minimale ou une condition d'arrêt.

Une représentation conceptuelle du traitement est donc :

```text
                    ZPIXEL
                       │
              ┌────────┴────────┐
              │                 │
        zone homogène       zone non homogène
              │                 │
              ▼                 ▼
       couleur moyenne      subdivision
                                │
                         ┌──────┼──────┐
                         ▼      ▼      ▼      ▼
                        Z1     Z2     Z3     Z4
```

---

# Concept de Zpixel

Un **Zpixel** représente une zone rectangulaire de l'image.

Il contient conceptuellement :

* sa position ;
* sa taille ;
* les pixels appartenant à sa zone ;
* une représentation RGB ;
* éventuellement des sous-Zpixels.

La représentation permet donc de passer d'une image constituée de pixels à une structure hiérarchique.

```text
Image
│
├── Zpixel principal
│   │
│   ├── Zpixel
│   │   ├── ...
│   │   ├── ...
│   │   ├── ...
│   │   └── ...
│   │
│   ├── Zpixel
│   ├── Zpixel
│   └── Zpixel
```

Cette structure est particulièrement adaptée à une stratégie de subdivision récursive.

---

# Construction de l'arbre de Zpixels

La construction de l'arbre suit une logique récursive.

Pseudo-code correspondant au principe étudié :

```c
Arbre *proto(Arbre *abr, Image *im)
{
    if (taille_minimale_non_atteinte)
    {
        division_du_zpixel_en_4();
    }

    if (taille_minimale_atteinte)
    {
        feuille = zpixel_r_v_b;
    }
}
```

Chaque Zpixel peut donc devenir :

* un **nœud interne**, lorsqu'il doit être subdivisé ;
* une **feuille**, lorsqu'il représente directement une zone suffisamment petite ou suffisamment uniforme.

Un nœud n'a donc pas nécessairement quatre fils dans tous les cas.

---

# Subdivision récursive

Le projet travaillait notamment avec des dimensions adaptées à une subdivision en quatre.

Une image peut par exemple commencer avec un Zpixel couvrant toute l'image :

```text
┌──────────────────────────────┐
│                              │
│          Zpixel racine       │
│                              │
│                              │
└──────────────────────────────┘
```

Puis être divisée :

```text
┌───────────────┬───────────────┐
│               │               │
│      Z1       │       Z2      │
│               │               │
├───────────────┼───────────────┤
│               │               │
│      Z3       │       Z4      │
│               │               │
└───────────────┴───────────────┘
```

Chaque zone peut à son tour être divisée.

Cette organisation correspond à une structure d'arbre de subdivision spatiale.

---

# Uniformisation d'une image

L'objectif du traitement est de produire une image simplifiée à partir de l'arbre de Zpixels.

Pour chaque zone, une couleur représentative est calculée à partir de ses pixels.

Par exemple :

```text
Pixels d'une zone

R V B
R V B
R V B
R V B
...

        │
        ▼

Calcul de la moyenne

R = average(R)
V = average(V)
B = average(B)

        │
        ▼

Zpixel RGB

(R, V, B)
```

Une zone uniforme peut donc être représentée par une couleur unique.

---

# Évaluation de la dégradation

Le projet comportait une **méthode d'évaluation de la dégradation**.

L'interface devait notamment permettre de récupérer un **seuil de dégradation**.

Ce seuil intervient dans la décision de conserver un Zpixel sous forme d'une zone uniforme ou de poursuivre sa subdivision.

Le principe peut être représenté ainsi :

```text
                 Zpixel
                    │
                    ▼
          Évaluation de la zone
                    │
          ┌─────────┴─────────┐
          │                   │
   dégradation faible   dégradation élevée
          │                   │
          ▼                   ▼
   conserver le Zpixel     subdiviser
          │                   │
          ▼                   ▼
     couleur moyenne     4 sous-zpixels
```

---

# Projection des Zpixels sur l'image

Une partie importante du projet concernait la **projection des Zpixels sur une image**.

Le principe consiste à prendre la représentation RGB d'un Zpixel et à la projeter sur toute la zone correspondante.

Ainsi, au lieu d'avoir :

```text
┌───┬───┬───┬───┐
│RGB│RGB│RGB│RGB│
├───┼───┼───┼───┤
│RGB│RGB│RGB│RGB│
├───┼───┼───┼───┤
│RGB│RGB│RGB│RGB│
└───┴───┴───┴───┘
```

une zone peut être uniformisée :

```text
┌───────────────┐
│               │
│   (R,V,B)     │
│   Zpixel      │
│               │
└───────────────┘
```

Le traitement permet ainsi de visualiser le résultat de la représentation par Zpixels.

---

# Représentation d'une image en mémoire

Le module `Image` définissait une représentation logique de l'image composée notamment de :

```c
struct Image {
    int nombre_ligne;
    int nombre_colonne;
    int rowstride;
    unsigned char *data;
};
```

Une image est donc manipulée logiquement comme une matrice :

```text
       colonne
    0   1   2   3   4   5

0   ■   ■   ■   ■   ■   ■
1   ■   ■   ■   ■   ■   ■
2   ■   ■   ■   ■   ■   ■
3   ■   ■   ■   ■   ■   ■
4   ■   ■   ■   ■   ■   ■
5   ■   ■   ■   ■   ■   ■
```

mais les données sont réellement stockées dans un **vecteur 1D**.

```text
data
│
├── ligne 0 ──────────────────┐
├── ligne 1 ──────────────────┤
├── ligne 2 ──────────────────┤
├── ligne 3 ──────────────────┤
├── ...
└── ligne n ──────────────────┘
```

Le code récupéré confirme cette représentation et l'allocation dynamique du tableau `data`.

---

# Organisation RGB en mémoire

Chaque pixel est représenté par trois composantes :

```text
[R][V][B]
```

avec :

```text
R = Rouge
V = Vert
B = Bleu
```

Une ligne d'image peut donc être représentée comme :

```text
[R][V][B] [R][V][B] [R][V][B] ... [PAD]
```

Le module `Image` définit :

```c
#define IMAGE_PAD 1
#define IMAGE_PIXEL_ELEMENT_NUMBER 3
```

La taille d'une ligne est ainsi calculée à partir du nombre de colonnes, de trois composantes par pixel et du padding.

---

# Rowstride et padding

Le `rowstride` représente la taille complète d'une ligne en mémoire, padding compris.

Le principe étudié était :

```text
rowstride = nombre_colonnes × 3 + padding
```

Par exemple, pour une image de 6 colonnes :

```text
6 pixels

[R][V][B] [R][V][B] [R][V][B]
[R][V][B] [R][V][B] [R][V][B]
                         │
                         ▼
                       [PAD]
```

Le buffer mémoire contient donc :

```text
[R][V][B][R][V][B][R][V][B]...[PAD]
```

puis la ligne suivante immédiatement après.

Le code récupéré calcule explicitement :

```c
int taille_rowstride =
    (IMAGE_PIXEL_ELEMENT_NUMBER * nombre_colonne)
    + IMAGE_PAD;
```

et alloue ensuite :

```c
int taille_data = taille_rowstride * nombre_ligne;
```

Le projet comportait donc également un travail de compréhension de la **représentation bas niveau des images en mémoire**.

---

# Accès aux pixels

Le module `Image` prévoyait deux fonctions principales :

```c
void setPixelImage(
    Image *im,
    unsigned int posX,
    unsigned int posY,
    unsigned char rouge,
    unsigned char vert,
    unsigned char bleu
);
```

et :

```c
void getPixelImage(
    Image *im,
    unsigned int posX,
    unsigned int posY,
    unsigned char *rouge,
    unsigned char *vert,
    unsigned char *bleu
);
```

Elles permettent d'offrir une manipulation logique :

```text
(x, y) → RGB
```

tout en conservant une représentation physique sous forme de tableau 1D.

---

# Allocation dynamique

La création d'une image repose sur une allocation dynamique.

Le principe utilisé était :

```c
Image *im = malloc(sizeof(Image));
```

puis allocation du buffer :

```c
im->data = malloc(
    sizeof(unsigned char) * taille_data
);
```

En cas d'échec de l'allocation du buffer, la structure précédemment allouée est libérée.

Le module prévoyait également :

```c
void destroyImage(Image *im);
```

afin de gérer la libération des ressources.

---

# Manipulation bas niveau de la mémoire

Le projet abordait également l'optimisation de certaines opérations sur les tableaux.

Une technique étudiée consistait à utiliser un **pointeur glissant** :

```c
ptr = t;
ptrFin = ptr + taille_tableau;

for (ptr = t; ptr < ptrFin; ptr++)
{
    *ptr = 0;
}
```

Cette approche permet d'éviter de recalculer l'adresse à partir d'indices à chaque itération.

Cela faisait partie du travail autour de la manipulation directe de la mémoire en C.

---

# Arbres avec GLib

Le projet prévoyait l'utilisation de **GLib**, notamment pour la gestion des arbres avec :

```c
GNode
```

L'arbre de Zpixels constitue une structure centrale du projet.

Il permet notamment :

* de construire une hiérarchie de zones ;
* de parcourir les Zpixels ;
* d'évaluer les différentes zones ;
* de retrouver les feuilles ;
* de représenter la subdivision de l'image.

Schématiquement :

```text
                    Racine
                      │
             ┌────────┼────────┐
             │        │        │
             ▼        ▼        ▼
            Z1       Z2       Z3
             │
        ┌────┼────┐
        ▼    ▼    ▼
       Z11  Z12  Z13
```

Le projet insistait sur l'utilisation des arbres pour les **parcours et les évaluations**.

---

# Gestion des piles

Une partie du travail abordait également la manipulation de piles et leur compilation sous forme de modules séparés.

Exemple de dépendances :

```make
prog.o : stack.o main.o
	gcc stack.o main.o -o prog

main.o : main.c stack2.h
	gcc -c main.c

stack2.o : stack2.c stack2.h
	gcc -c stack2.c
```

Cette organisation illustre la séparation entre :

```text
main.c
   │
   ▼
stack2.h
   ▲
   │
stack2.c
```

puis l'édition de liens des différents fichiers objets.

---

# Architecture logicielle

Le projet était organisé autour de plusieurs modules.

Une partie du graphe de dépendances retrouvé peut être représentée ainsi :

```text
                    testMainProgram
                    /      |       \
                   /       |        \
                  ▼        ▼         ▼
             image.o    zpixel.o    main.o
                │          │          │
                ▼          ▼          ▼
             image.c    zpixel.c    main.c
                │          │          │
                └──────┬───┴──────────┘
                       │
                       ▼
                   image.h
                   zpixel.h
```

Cette organisation illustre une architecture modulaire basée sur des fichiers sources et des fichiers d'en-tête.

---

# Exemple de chaîne de traitement

Une exécution complète peut être représentée par :

```text
              IMAGE
                │
                ▼
        Création de Image
                │
                ▼
       Chargement / accès RGB
                │
                ▼
        Création du Zpixel
                │
                ▼
       Construction de l'arbre
                │
                ▼
       Évaluation des zones
                │
         ┌──────┴──────┐
         │             │
    zone uniforme   zone non uniforme
         │             │
         │             ▼
         │        subdivision ×4
         │             │
         └──────┬──────┘
                ▼
       Couleur représentative
                │
                ▼
      Projection sur l'image
                │
                ▼
       Évaluation visuelle
                │
                ▼
           IHM GTK
```

---

# Tests

Les tests constituaient une partie explicite du projet.

L'objectif était notamment de vérifier :

* les valeurs obtenues ;
* les valeurs attendues ;
* l'état des cases mémoire ;
* l'allocation de la structure `Image` ;
* l'allocation de `data` ;
* le nombre de lignes ;
* le nombre de colonnes ;
* le `rowstride` ;
* le comportement des fonctions de manipulation des pixels.

Le code retrouvé utilise notamment `assert` pour vérifier les invariants :

```c
assert(
    im->nombre_colonne == nombre_colonne
);
```

ainsi que :

```c
assert(
    im->nombre_ligne == nombre_ligne
);
```

et :

```c
assert(
    im->rowstride == calcul_rowstride
);
```

La présence de ces tests montre que la vérification ne portait pas uniquement sur le résultat visuel, mais également sur la **représentation interne de l'image**.

---

# Débogage avec GDB

Une séance spécifique était consacrée à la présentation de **GDB**.

Le débogueur devait notamment permettre d'analyser le fonctionnement du programme C et de rechercher les erreurs liées :

* aux allocations ;
* aux pointeurs ;
* aux accès mémoire ;
* aux structures ;
* aux parcours d'arbres ;
* à l'exécution des différents modules.

Cette partie était particulièrement pertinente pour un projet manipulant directement des buffers `unsigned char *` et des structures dynamiques.

---

# Interface graphique GTK

Le projet comportait également une partie **IHM avec GTK**.

L'interface avait notamment pour rôle de permettre la récupération du **seuil de dégradation** utilisé lors de l'évaluation de l'image.

La chaîne fonctionnelle était donc :

```text
                  IHM GTK
                     │
                     ▼
             Seuil de dégradation
                     │
                     ▼
              Traitement image
                     │
                     ▼
              Arbre de Zpixels
                     │
                     ▼
                Projection
                     │
                     ▼
               Résultat visuel
```

L'intégration de l'IHM faisait partie des dernières étapes du projet.

---

# Intégration

Le projet était construit progressivement :

```text
1. Module Zpixel
        │
        ▼
2. Arbre de Zpixels
        │
        ▼
3. Évaluation de la dégradation
        │
        ▼
4. Interface GTK
        │
        ▼
5. Intégration de l'ensemble
        │
        ▼
6. Démonstration finale
```

L'intégration constituait une étape importante afin de faire fonctionner ensemble :

* la représentation de l'image ;
* les Zpixels ;
* l'arbre ;
* les algorithmes d'évaluation ;
* la projection ;
* l'interface graphique.

---

# Organisation pédagogique

Le projet était planifié sur environ **20 heures**.

| Séance | Travail                                | Durée indicative |
| ------ | -------------------------------------- | ---------------: |
| 1      | Présentation du projet                 |           30 min |
| 1      | Module `zpixel`                        |           3 h 30 |
| 2      | Présentation de GDB                    |           30 min |
| 2      | Construction de l'arbre des Zpixels    |              2 h |
| 2–3    | Méthode d'évaluation de la dégradation |              3 h |
| 3–4    | GTK / présentation                     |           30 min |
| 3–4    | Conception de l'IHM                    |              3 h |
| 4–5    | Intégration de l'ensemble              |              5 h |
| 5      | Démonstration finale                   |              2 h |

**Total : 20 h**

---

# Organisation du développement

Le projet s'appuyait sur plusieurs composants :

```text
                    Projet
                      │
        ┌─────────────┼──────────────┐
        │             │              │
        ▼             ▼              ▼
      Image         Zpixel          IHM
        │             │              │
        │             ▼              │
        │           Arbre             │
        │             │              │
        └─────────────┼──────────────┘
                      ▼
                 Intégration
                      │
                      ▼
                    Tests
```

Les modules étaient séparés entre fichiers `.c` et `.h`, permettant de travailler sur différentes fonctionnalités indépendamment.

---

# Gestion de version

L'outil de gestion de version indiqué dans le sujet était :

**GitLab**

Le projet était initialement hébergé sur l'infrastructure GitLab de l'université.

Le dépôt universitaire ayant depuis été supprimé ou vidé, ce repository constitue une **reconstitution du projet à partir des éléments techniques encore disponibles** : code source retrouvé, notes de cours, schémas, captures et documents de travail.

---

# Technologies

## Langage

* C

## Traitement d'image

* Manipulation RGB
* Représentation matricielle logique
* Stockage en tableau 1D
* `unsigned char`
* `rowstride`
* padding
* moyenne des composantes RGB
* projection de zones

## Structures de données

* Arbres
* `GNode`
* Piles
* Parcours d'arbres
* Structures C

## Mémoire

* `malloc`
* `free`
* pointeurs
* tableaux
* arithmétique des pointeurs
* manipulation de buffers
* représentation mémoire des images

## Interface

* GTK
* GLib

## Tests et développement

* `assert`
* Tests unitaires
* GDB
* GitLab
* Makefile
* GCC

---

# Compétences mobilisées

Ce projet m'a permis de travailler notamment sur :

* **Programmation C**
* **Gestion de la mémoire**
* **Pointeurs et arithmétique des pointeurs**
* **Structures de données**
* **Arbres**
* **GLib / GNode**
* **Traitement d'image**
* **Représentation RGB**
* **Manipulation de buffers**
* **Tests unitaires**
* **Débogage avec GDB**
* **GTK**
* **Compilation modulaire**
* **Makefile**
* **Git / GitLab**

---

# Ce que le projet illustre

Au-delà du traitement d'image, ce projet regroupait plusieurs notions fondamentales de programmation système et logicielle en C.

Il fallait notamment comprendre la différence entre :

```text
Représentation logique
        │
        ▼
       Image
   (x, y, RGB)
        │
        ▼
Représentation physique
        │
        ▼
 unsigned char *data
        │
        ▼
   mémoire contiguë
```

et faire le lien entre cette représentation bas niveau et une structure algorithmique plus abstraite :

```text
Image
  │
  ▼
Zpixels
  │
  ▼
Arbre
  │
  ▼
Parcours / évaluation
  │
  ▼
Image uniformisée
```

Le projet combinait ainsi **programmation bas niveau**, **algorithmique**, **structures de données**, **traitement d'image**, **tests** et **interface graphique**.

---

# Structure envisagée

La structure exacte du dépôt original n'a malheureusement pas pu être récupérée.

Les fichiers retrouvés permettent cependant d'identifier une organisation de ce type :

```text
.
├── image.c
├── image.h
├── zpixel.c
├── zpixel.h
├── main.c
├── tests/
│   └── ...
├── Makefile
└── README.md
```

Cette arborescence est une **reconstitution indicative** et ne prétend pas reproduire exactement l'organisation du dépôt universitaire original.

---

# État du projet

Le dépôt GitLab universitaire original n'étant plus accessible, ce repository est une **reconstitution documentaire et technique** du projet réalisé à l'université.

Les éléments conservés permettent néanmoins de retrouver plusieurs aspects importants du développement :

* conception du module `Image` ;
* allocation dynamique des images ;
* représentation RGB ;
* gestion du `rowstride` et du padding ;
* fonctions d'accès aux pixels ;
* construction d'un arbre de Zpixels ;
* calcul d'une couleur représentative ;
* subdivision récursive ;
* évaluation de la dégradation ;
* projection des Zpixels ;
* tests avec `assert` ;
* utilisation de GLib/GNode ;
* développement en C ;
* travail avec GTK ;
* compilation modulaire ;
* utilisation de GDB.

Certaines parties du projet original ne sont plus disponibles et ne sont donc pas reproduites ici.

---

# Contexte

**Projet universitaire — Uniformisation d'images**

**Langage :** C
**Durée pédagogique :** environ 20 heures
**Outils :** GCC, GDB, GitLab, GTK, GLib
**Structures principales :** Image, Zpixel, arbre
**Thématiques :** traitement d'image, mémoire, structures de données, tests et IHM.

---

## Auteur

**Houssam BACAR**

Projet réalisé dans le cadre de la formation universitaire en informatique.

# Licence MIT
