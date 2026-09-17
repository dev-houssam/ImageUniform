/*
 * ============================================================================
 * Uniformisation d'images — Reconstitution du projet universitaire
 * ============================================================================
 *
 * Auteur : Houssam BACAR
 * Projet : Uniformisation d'images
 *
 * Reconstitution à partir de fragments de code, notes et documents conservés.
 *
 * Le dépôt original ayant été perdu, ce fichier ne prétend pas reproduire
 * bit-à-bit l'implémentation originale. Il rassemble cependant les concepts
 * effectivement retrouvés dans les sources :
 *
 *   - représentation d'une image RGB en mémoire ;
 *   - rowstride et padding ;
 *   - accès aux pixels ;
 *   - Zpixels ;
 *   - subdivision récursive en quatre zones ;
 *   - arbre de Zpixels ;
 *   - calcul d'une couleur moyenne ;
 *   - projection des Zpixels sur une image ;
 *   - tests avec assert ;
 *   - manipulation de pointeurs et de buffers.
 *
 * Une mesure exacte de la "dégradation" n'ayant pas été retrouvée, ce fichier
 * utilise une mesure simple reconstruite : max(R,V,B) - min(R,V,B) sur les
 * pixels d'un Zpixel.
 *
 * Compilation sans dépendance externe :
 *
 *     gcc -std=c11 -Wall -Wextra -O2 uniformisation_images.c -o uniformisation_images
 *
 * Exécution :
 *
 *     ./uniformisation_images
 *
 * Le programme produit une image PPM "image_uniformisee.ppm" afin de rendre
 * le résultat directement observable sur GitHub.
 * ============================================================================
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Configuration de l'image
 * -------------------------------------------------------------------------- */

#define IMAGE_PAD 1
#define IMAGE_PIXEL_ELEMENT_NUMBER 3

#define IMAGE_PIXELS_INITIAL_DATA 48
#define IMAGE_END_PIXEL_PAD_DATA (IMAGE_PIXELS_INITIAL_DATA + 1)

/* --------------------------------------------------------------------------
 * Structures
 * -------------------------------------------------------------------------- */

typedef struct {
    int nombre_ligne;
    int nombre_colonne;
    int rowstride;
    unsigned char *data;
} Image;

typedef struct {
    int x;
    int y;
    int largeur;
    int hauteur;

    unsigned char rouge;
    unsigned char vert;
    unsigned char bleu;

    double degradation;
} Zpixel;

typedef struct NoeudZpixel {
    Zpixel zpixel;

    struct NoeudZpixel *fils[4];
    int nombre_fils;
} NoeudZpixel;

/* --------------------------------------------------------------------------
 * Outils
 * -------------------------------------------------------------------------- */

static unsigned char clamp_u8(int valeur)
{
    if (valeur < 0)
        return 0;

    if (valeur > 255)
        return 255;

    return (unsigned char)valeur;
}

/*
 * Calcul de l'adresse d'un pixel dans le tableau 1D.
 *
 * Une ligne contient :
 *
 *     nombre_colonne * 3 + padding
 *
 * octets.
 */
static size_t pixel_offset(const Image *im, int x, int y)
{
    return (size_t)y * (size_t)im->rowstride
         + (size_t)x * IMAGE_PIXEL_ELEMENT_NUMBER;
}

/* --------------------------------------------------------------------------
 * Module Image
 * -------------------------------------------------------------------------- */

Image *createImage(const int nombre_ligne, const int nombre_colonne)
{
    if (nombre_ligne <= 0 || nombre_colonne <= 0)
        return NULL;

    Image *im = malloc(sizeof(Image));

    if (im == NULL)
        return NULL;

    im->nombre_ligne = nombre_ligne;
    im->nombre_colonne = nombre_colonne;

    im->rowstride =
        IMAGE_PIXEL_ELEMENT_NUMBER * nombre_colonne + IMAGE_PAD;

    const size_t taille_data =
        (size_t)im->rowstride * (size_t)im->nombre_ligne;

    im->data = malloc(sizeof(unsigned char) * taille_data);

    if (im->data == NULL) {
        free(im);
        return NULL;
    }

    /*
     * Initialisation du buffer.
     *
     * Les trois octets d'un pixel sont initialisés à la même valeur,
     * conformément aux fragments retrouvés dans le projet.
     */
    memset(im->data, IMAGE_PIXELS_INITIAL_DATA, taille_data);

    /* Le dernier octet de chaque ligne correspond au padding. */
    for (int y = 0; y < im->nombre_ligne; ++y) {
        im->data[
            (size_t)y * (size_t)im->rowstride
            + (size_t)(im->rowstride - 1)
        ] = IMAGE_END_PIXEL_PAD_DATA;
    }

    return im;
}

void destroyImage(Image *im)
{
    if (im == NULL)
        return;

    free(im->data);
    free(im);
}

void setPixelImage(
    Image *im,
    const unsigned int posX,
    const unsigned int posY,
    const unsigned char rouge,
    const unsigned char vert,
    const unsigned char bleu)
{
    if (im == NULL)
        return;

    if (posX >= (unsigned int)im->nombre_colonne ||
        posY >= (unsigned int)im->nombre_ligne)
        return;

    const size_t offset = pixel_offset(im, (int)posX, (int)posY);

    im->data[offset + 0] = rouge;
    im->data[offset + 1] = vert;
    im->data[offset + 2] = bleu;
}

void getPixelImage(
    const Image *im,
    const unsigned int posX,
    const unsigned int posY,
    unsigned char *rouge,
    unsigned char *vert,
    unsigned char *bleu)
{
    if (im == NULL ||
        rouge == NULL ||
        vert == NULL ||
        bleu == NULL)
        return;

    if (posX >= (unsigned int)im->nombre_colonne ||
        posY >= (unsigned int)im->nombre_ligne)
        return;

    const size_t offset = pixel_offset(im, (int)posX, (int)posY);

    *rouge = im->data[offset + 0];
    *vert  = im->data[offset + 1];
    *bleu  = im->data[offset + 2];
}

/* --------------------------------------------------------------------------
 * Création d'une image de démonstration
 * -------------------------------------------------------------------------- */

/*
 * Le dépôt original utilisait une image externe, mais le fichier retrouvé
 * ne contient pas le chargeur d'image complet.
 *
 * Pour rendre cette reconstitution autonome, une image RGB synthétique est
 * générée ici.
 */
static void remplir_image_demo(Image *im)
{
    for (int y = 0; y < im->nombre_ligne; ++y) {
        for (int x = 0; x < im->nombre_colonne; ++x) {

            unsigned char r;
            unsigned char v;
            unsigned char b;

            /*
             * Plusieurs grandes zones sont volontairement homogènes,
             * avec une zone centrale plus contrastée afin de faire apparaître
             * la subdivision des Zpixels.
             */
            if (x < im->nombre_colonne / 2 &&
                y < im->nombre_ligne / 2) {
                r = 45;
                v = 95;
                b = 180;
            }
            else if (x >= im->nombre_colonne / 2 &&
                     y < im->nombre_ligne / 2) {
                r = 205;
                v = 70;
                b = 65;
            }
            else if (x < im->nombre_colonne / 2) {
                r = 65;
                v = 175;
                b = 90;
            }
            else {
                /*
                 * Zone volontairement détaillée.
                 */
                r = (unsigned char)(70 + (x * 11 + y * 17) % 170);
                v = (unsigned char)(50 + (x * 19 + y * 7) % 170);
                b = (unsigned char)(40 + (x * 5 + y * 23) % 170);
            }

            setPixelImage(im, (unsigned)x, (unsigned)y, r, v, b);
        }
    }
}

/* --------------------------------------------------------------------------
 * Zpixel
 * -------------------------------------------------------------------------- */

static Zpixel calculer_zpixel(
    const Image *im,
    int x,
    int y,
    int largeur,
    int hauteur)
{
    Zpixel z = {
        .x = x,
        .y = y,
        .largeur = largeur,
        .hauteur = hauteur,
        .rouge = 0,
        .vert = 0,
        .bleu = 0,
        .degradation = 0.0
    };

    unsigned long long somme_r = 0;
    unsigned long long somme_v = 0;
    unsigned long long somme_b = 0;

    unsigned char min_r = 255;
    unsigned char min_v = 255;
    unsigned char min_b = 255;

    unsigned char max_r = 0;
    unsigned char max_v = 0;
    unsigned char max_b = 0;

    size_t nombre_pixels = 0;

    for (int py = y; py < y + hauteur; ++py) {
        for (int px = x; px < x + largeur; ++px) {

            unsigned char r;
            unsigned char v;
            unsigned char b;

            getPixelImage(
                im,
                (unsigned)px,
                (unsigned)py,
                &r,
                &v,
                &b
            );

            somme_r += r;
            somme_v += v;
            somme_b += b;

            if (r < min_r) min_r = r;
            if (v < min_v) min_v = v;
            if (b < min_b) min_b = b;

            if (r > max_r) max_r = r;
            if (v > max_v) max_v = v;
            if (b > max_b) max_b = b;

            ++nombre_pixels;
        }
    }

    if (nombre_pixels > 0) {
        z.rouge = (unsigned char)(somme_r / nombre_pixels);
        z.vert  = (unsigned char)(somme_v / nombre_pixels);
        z.bleu  = (unsigned char)(somme_b / nombre_pixels);
    }

    /*
     * RECONSTITUTION :
     *
     * La formule exacte d'évaluation de la dégradation du projet original
     * n'a pas été retrouvée. On utilise ici l'étendue maximale des composantes
     * RGB comme mesure simple de non-uniformité.
     */
    int variation_r = (int)max_r - (int)min_r;
    int variation_v = (int)max_v - (int)min_v;
    int variation_b = (int)max_b - (int)min_b;

    int variation = variation_r;

    if (variation_v > variation) variation = variation_v;
    if (variation_b > variation) variation = variation_b;

    z.degradation = (double)variation;

    return z;
}

/* --------------------------------------------------------------------------
 * Arbre de Zpixels
 * -------------------------------------------------------------------------- */

static NoeudZpixel *creer_noeud_zpixel(Zpixel z)
{
    NoeudZpixel *noeud = malloc(sizeof(NoeudZpixel));

    if (noeud == NULL)
        return NULL;

    noeud->zpixel = z;
    noeud->nombre_fils = 0;

    for (int i = 0; i < 4; ++i)
        noeud->fils[i] = NULL;

    return noeud;
}

static int est_feuille(const NoeudZpixel *noeud)
{
    return noeud != NULL && noeud->nombre_fils == 0;
}

static void detruire_arbre(NoeudZpixel *noeud)
{
    if (noeud == NULL)
        return;

    for (int i = 0; i < noeud->nombre_fils; ++i)
        detruire_arbre(noeud->fils[i]);

    free(noeud);
}

/*
 * Subdivision d'un Zpixel en quatre zones.
 *
 * Le projet travaillait avec une subdivision en 4 et devait tenir compte
 * des contraintes de taille / puissance de 2.
 *
 * Cette version accepte également les dimensions impaires en répartissant
 * les pixels restants entre les sous-zones.
 */
static int subdiviser_zpixel(
    const Image *im,
    NoeudZpixel *noeud)
{
    if (noeud == NULL)
        return 0;

    Zpixel z = noeud->zpixel;

    if (z.largeur <= 1 || z.hauteur <= 1)
        return 0;

    const int gauche = z.largeur / 2;
    const int droite = z.largeur - gauche;
    const int haut = z.hauteur / 2;
    const int bas = z.hauteur - haut;

    if (gauche <= 0 || droite <= 0 || haut <= 0 || bas <= 0)
        return 0;

    const int x = z.x;
    const int y = z.y;

    Zpixel zones[4];

    zones[0] = calculer_zpixel(im, x,          y,          gauche, haut);
    zones[1] = calculer_zpixel(im, x + gauche, y,          droite, haut);
    zones[2] = calculer_zpixel(im, x,          y + haut,   gauche, bas);
    zones[3] = calculer_zpixel(im, x + gauche, y + haut,   droite, bas);

    for (int i = 0; i < 4; ++i) {
        noeud->fils[i] = creer_noeud_zpixel(zones[i]);

        if (noeud->fils[i] == NULL) {
            for (int j = 0; j < i; ++j) {
                detruire_arbre(noeud->fils[j]);
                noeud->fils[j] = NULL;
            }

            noeud->nombre_fils = 0;
            return 0;
        }
    }

    noeud->nombre_fils = 4;
    return 1;
}

/*
 * Construction récursive de l'arbre.
 *
 * Un Zpixel est conservé comme feuille si :
 *
 *   - sa taille minimale est atteinte ;
 *   - ou sa dégradation est inférieure au seuil.
 *
 * Sinon il est divisé en quatre sous-Zpixels.
 */
static int construire_arbre(
    const Image *im,
    NoeudZpixel *noeud,
    double seuil_degradation,
    int taille_minimale)
{
    if (noeud == NULL)
        return 0;

    const Zpixel *z = &noeud->zpixel;

    if (z->largeur <= taille_minimale ||
        z->hauteur <= taille_minimale ||
        z->degradation <= seuil_degradation) {
        return 1;
    }

    if (!subdiviser_zpixel(im, noeud))
        return 1;

    for (int i = 0; i < noeud->nombre_fils; ++i) {
        if (!construire_arbre(
                im,
                noeud->fils[i],
                seuil_degradation,
                taille_minimale)) {
            return 0;
        }
    }

    return 1;
}

/* --------------------------------------------------------------------------
 * Statistiques de l'arbre
 * -------------------------------------------------------------------------- */

static size_t compter_noeuds(const NoeudZpixel *noeud)
{
    if (noeud == NULL)
        return 0;

    size_t total = 1;

    for (int i = 0; i < noeud->nombre_fils; ++i)
        total += compter_noeuds(noeud->fils[i]);

    return total;
}

static size_t compter_feuilles(const NoeudZpixel *noeud)
{
    if (noeud == NULL)
        return 0;

    if (est_feuille(noeud))
        return 1;

    size_t total = 0;

    for (int i = 0; i < noeud->nombre_fils; ++i)
        total += compter_feuilles(noeud->fils[i]);

    return total;
}

static int profondeur_arbre(const NoeudZpixel *noeud)
{
    if (noeud == NULL || est_feuille(noeud))
        return 0;

    int profondeur = 0;

    for (int i = 0; i < noeud->nombre_fils; ++i) {
        int p = profondeur_arbre(noeud->fils[i]);

        if (p > profondeur)
            profondeur = p;
    }

    return profondeur + 1;
}

/* --------------------------------------------------------------------------
 * Projection des Zpixels
 * -------------------------------------------------------------------------- */

static void remplir_zone(
    Image *im,
    const Zpixel *z)
{
    for (int y = z->y; y < z->y + z->hauteur; ++y) {
        for (int x = z->x; x < z->x + z->largeur; ++x) {
            setPixelImage(
                im,
                (unsigned)x,
                (unsigned)y,
                z->rouge,
                z->vert,
                z->bleu
            );
        }
    }
}

static void projeter_arbre(
    Image *im,
    const NoeudZpixel *noeud)
{
    if (noeud == NULL)
        return;

    if (est_feuille(noeud)) {
        remplir_zone(im, &noeud->zpixel);
        return;
    }

    for (int i = 0; i < noeud->nombre_fils; ++i)
        projeter_arbre(im, noeud->fils[i]);
}

/* --------------------------------------------------------------------------
 * Affichage de l'arbre
 * -------------------------------------------------------------------------- */

static void afficher_arbre(
    const NoeudZpixel *noeud,
    int profondeur)
{
    if (noeud == NULL)
        return;

    for (int i = 0; i < profondeur; ++i)
        printf("  ");

    printf(
        "Zpixel [%d,%d] %dx%d "
        "RGB=(%u,%u,%u) "
        "degradation=%.1f "
        "fils=%d\n",
        noeud->zpixel.x,
        noeud->zpixel.y,
        noeud->zpixel.largeur,
        noeud->zpixel.hauteur,
        noeud->zpixel.rouge,
        noeud->zpixel.vert,
        noeud->zpixel.bleu,
        noeud->zpixel.degradation,
        noeud->nombre_fils
    );

    for (int i = 0; i < noeud->nombre_fils; ++i)
        afficher_arbre(noeud->fils[i], profondeur + 1);
}

/* --------------------------------------------------------------------------
 * Export PPM
 * -------------------------------------------------------------------------- */

/*
 * Le format PPM n'était pas présent dans les fragments retrouvés.
 * Il est utilisé ici uniquement pour rendre le résultat de la reconstitution
 * facilement visualisable sans dépendance à GTK.
 */
static int sauvegarder_ppm(
    const Image *im,
    const char *nom_fichier)
{
    FILE *fichier = fopen(nom_fichier, "wb");

    if (fichier == NULL) {
        fprintf(
            stderr,
            "Erreur : impossible d'ouvrir %s : %s\n",
            nom_fichier,
            strerror(errno)
        );
        return 0;
    }

    fprintf(
        fichier,
        "P6\n%d %d\n255\n",
        im->nombre_colonne,
        im->nombre_ligne
    );

    for (int y = 0; y < im->nombre_ligne; ++y) {
        for (int x = 0; x < im->nombre_colonne; ++x) {
            unsigned char r;
            unsigned char v;
            unsigned char b;

            getPixelImage(
                im,
                (unsigned)x,
                (unsigned)y,
                &r,
                &v,
                &b
            );

            fputc(r, fichier);
            fputc(v, fichier);
            fputc(b, fichier);
        }
    }

    fclose(fichier);
    return 1;
}

/* --------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------- */

static void tester_image(void)
{
    printf("[TEST] Module Image...\n");

    const int lignes = 6;
    const int colonnes = 8;

    Image *im = createImage(lignes, colonnes);

    assert(im != NULL);
    assert(im->nombre_ligne == lignes);
    assert(im->nombre_colonne == colonnes);

    const int rowstride =
        colonnes * IMAGE_PIXEL_ELEMENT_NUMBER + IMAGE_PAD;

    assert(im->rowstride == rowstride);
    assert(im->data != NULL);

    setPixelImage(im, 2, 3, 10, 20, 30);

    unsigned char r;
    unsigned char v;
    unsigned char b;

    getPixelImage(im, 2, 3, &r, &v, &b);

    assert(r == 10);
    assert(v == 20);
    assert(b == 30);

    /*
     * Vérification du padding de chaque ligne.
     */
    for (int y = 0; y < lignes; ++y) {
        size_t offset =
            (size_t)y * (size_t)im->rowstride
            + (size_t)(im->rowstride - 1);

        assert(im->data[offset] == IMAGE_END_PIXEL_PAD_DATA);
    }

    destroyImage(im);

    printf("[TEST] Module Image : OK\n");
}

static void tester_arbre(void)
{
    printf("[TEST] Construction d'un arbre de Zpixels...\n");

    Image *im = createImage(16, 16);

    assert(im != NULL);

    remplir_image_demo(im);

    Zpixel racine_zpixel =
        calculer_zpixel(
            im,
            0,
            0,
            im->nombre_colonne,
            im->nombre_ligne
        );

    NoeudZpixel *racine =
        creer_noeud_zpixel(racine_zpixel);

    assert(racine != NULL);

    assert(
        construire_arbre(im, racine, 15.0, 2)
    );

    assert(compter_noeuds(racine) >= 1);
    assert(compter_feuilles(racine) >= 1);

    detruire_arbre(racine);
    destroyImage(im);

    printf("[TEST] Arbre de Zpixels : OK\n");
}

/* --------------------------------------------------------------------------
 * Programme principal
 * -------------------------------------------------------------------------- */

int main(void)
{
    printf("===============================================\n");
    printf(" Uniformisation d'images\n");
    printf(" Reconstitution du projet universitaire\n");
    printf("===============================================\n\n");

    /*
     * Tests de base.
     */
    tester_image();
    tester_arbre();

    printf("\n");

    /*
     * Image de démonstration.
     *
     * 64 x 64 permet d'obtenir plusieurs niveaux de subdivision tout en
     * gardant une exécution très légère.
     */
    const int lignes = 64;
    const int colonnes = 64;

    Image *image_originale =
        createImage(lignes, colonnes);

    if (image_originale == NULL) {
        fprintf(
            stderr,
            "Erreur : impossible de créer l'image.\n"
        );
        return EXIT_FAILURE;
    }

    remplir_image_demo(image_originale);

    /*
     * Export de l'image avant uniformisation.
     */
    if (!sauvegarder_ppm(
            image_originale,
            "image_originale.ppm")) {
        destroyImage(image_originale);
        return EXIT_FAILURE;
    }

    /*
     * Création du Zpixel racine couvrant toute l'image.
     */
    Zpixel racine_zpixel =
        calculer_zpixel(
            image_originale,
            0,
            0,
            colonnes,
            lignes
        );

    NoeudZpixel *racine =
        creer_noeud_zpixel(racine_zpixel);

    if (racine == NULL) {
        destroyImage(image_originale);
        return EXIT_FAILURE;
    }

    /*
     * Paramètres de la reconstitution.
     *
     * Le seuil exact utilisé dans le projet original n'a pas été retrouvé.
     */
    const double seuil_degradation = 20.0;
    const int taille_minimale = 2;

    printf("Image : %d x %d\n", colonnes, lignes);
    printf(
        "Seuil de degradation : %.1f\n",
        seuil_degradation
    );
    printf(
        "Taille minimale d'un Zpixel : %d\n\n",
        taille_minimale
    );

    if (!construire_arbre(
            image_originale,
            racine,
            seuil_degradation,
            taille_minimale)) {

        fprintf(
            stderr,
            "Erreur pendant la construction de l'arbre.\n"
        );

        detruire_arbre(racine);
        destroyImage(image_originale);

        return EXIT_FAILURE;
    }

    printf("Arbre construit :\n");
    printf("  Noeuds   : %zu\n", compter_noeuds(racine));
    printf("  Feuilles : %zu\n", compter_feuilles(racine));
    printf("  Profondeur : %d\n\n", profondeur_arbre(racine));

    /*
     * Décommenter pour afficher l'arbre complet.
     *
     * afficher_arbre(racine, 0);
     */

    /*
     * Création de l'image uniformisée.
     */
    Image *image_uniformisee =
        createImage(lignes, colonnes);

    if (image_uniformisee == NULL) {
        detruire_arbre(racine);
        destroyImage(image_originale);
        return EXIT_FAILURE;
    }

    projeter_arbre(
        image_uniformisee,
        racine
    );

    if (!sauvegarder_ppm(
            image_uniformisee,
            "image_uniformisee.ppm")) {

        destroyImage(image_uniformisee);
        detruire_arbre(racine);
        destroyImage(image_originale);

        return EXIT_FAILURE;
    }

    printf("Images generees :\n");
    printf("  - image_originale.ppm\n");
    printf("  - image_uniformisee.ppm\n\n");

    /*
     * Nettoyage.
     */
    destroyImage(image_uniformisee);
    detruire_arbre(racine);
    destroyImage(image_originale);

    printf("Programme termine avec succes.\n");

    return EXIT_SUCCESS;
}
