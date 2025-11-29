/**
* @file main.c
* @brief Jeu Sokaban
* @author Liam CHARPENTIER
* @version 2
* @date 12/11/2025
*
* Deuxième version du jeu Sokoban dans le terminal
* Nouveautés :
* - Ajout d'un système de zoom
* - Ajout de sauvegarde des déplacements
* - Possibilité de revenir au déplacement prédécent
* - Modification avec des structures
* - Les lignes vides ne sont plus affichées
*/

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define TAILLE 12

/* Définition des touches */

#define MOVE_LEFT   'q'
#define MOVE_RIGHT  'd'
#define MOVE_DOWN   's'
#define MOVE_UP     'z'
#define GIVE_UP     'x'
#define RESTART     'r'
#define ZOOM_IN     '+'
#define ZOOM_OUT    '-'
#define UNDO        'u'

#define MAX_DEP     999

/* Définition des caractères */

const char PLAYER           = '@';
const char MUR              = '#';
const char CAISSE           = '$';
const char CIBLE            = '.';
const char PLAYER_SUR_CIBLE = '+';
const char CAISSE_SUR_CIBLE = '*';
const char VIDE             = ' ';

const int ZOOM_MAX          = 3;
const int ZOOM_MIN          = 1;

const char FIN_COULEUR[10]  = "\033[0m";
const char ROUGE[10]        = "\033[31m";
const char VERT[10]         = "\033[32m";
const char JAUNE[10]        = "\033[33m";


/* Définition des variables de stockage */

#define DEP_SOK_GAU        'g'
#define DEP_SOK_HAU        'h'
#define DEP_SOK_BAS        'b'
#define DEP_SOK_DRO        'd'
#define DEP_SOK_CAI_G      'G'
#define DEP_SOK_CAI_H      'H'
#define DEP_SOK_CAI_B      'B'
#define DEP_SOK_CAI_D      'D'

/* Définition du tableau */

typedef char t_plateau[TAILLE][TAILLE];
typedef char t_tabDeplacement[MAX_DEP];

// Structure déplacements
typedef struct {
    t_tabDeplacement liste; // liste des déplacements (pour stocker et undo)
    int nbDeplacements;
} t_deplacements;

// Structure joueur
typedef struct {
    int posX;
    int posY;
} t_joueur;

// Structure partie
typedef struct {
    bool estFinis; // est ce que le jeu est finis, pour la boucle
    char nomFichier[30]; // nom du fichier de jeu
    int zoom; // niveau de zoom 1 2 3
    int tentatives; // nombre de tentatives (quand on recommence : tentative ++)
    t_deplacements deplacements;
    t_plateau plateau;
    t_joueur joueur;
} t_partie;

/* Fonctions */

void charger_partie(t_plateau plateau, char fichier[]);
void enregistrer_partie(t_plateau plateau, char fichier[]);
int kbhit();
void enregistrer_deplacements(t_tabDeplacement t, int nb, char fic[]);

void initialiser_jeu(t_partie *jeu);
void afficher_entete(t_partie jeu);
void afficher_plateau(t_partie jeu);
void afficher_case(t_partie jeu, int ligne, int colonne);
bool demarrer_partie(t_partie *jeu);
void position_joueur(t_partie *jeu);
bool gagne(t_partie jeu);
void deplacer(t_partie *jeu, int depX, int depY);
void afficher_ligne(int longueur);
int afficher_encadre(char texte[]);
void gerer_sauvegarde(t_partie *jeu);
void gestion_deplacement_caisse(char *positionSuivante,
    char *positionApresSuivante, t_partie *jeu);
void gerer_redemarrage(t_partie *jeu);
void gerer_touches(t_partie *jeu, char touche);
void stocker_deplacement(t_partie *jeu, int depX, int depY, int type);
void retour_arriere(t_partie *jeu);
void initialiser_jeu(t_partie *jeu);
int recuperer_lignes_utiles(t_partie jeu);
void gerer_gagner(t_partie *jeu);

/**
*
* @brief Récupérer les infos du niveau, lancer le jeu et gérer les touches
* @return entier : code de sortie
*/
int main()
{
    /* Variables */
    char touche = '\0';
    t_partie jeu;

    initialiser_jeu(&jeu);

    if(demarrer_partie(&jeu)){

        position_joueur(&jeu);

        while(!jeu.estFinis)
        {
            if (kbhit()){
                touche = getchar();

                gerer_touches(&jeu, touche);

                if(!jeu.estFinis){
                    afficher_entete(jeu);
                    afficher_plateau(jeu);
                }

                if(gagne(jeu)){
                    gerer_gagner(&jeu);
                }
            }
        }
    }

    return 0;
}

/**
*
* @brief Affichage de l'entête
* @param jeu de type t_partie, Entrée : structure de la partie.
*
*/
void afficher_entete(t_partie jeu)
{
    int longueurTexte;

    system("clear");
    longueurTexte = afficher_encadre("SOKOBAN v2 - Liam CHARPENTIER");

    printf("Fichier chargé : %s\n", jeu.nomFichier);
    afficher_ligne(longueurTexte);

    printf("Haut : %c | Bas : %c\nGauche : %c | Droite : %c\n",
        MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT
    );
    printf("Zoomer : %c | Dézoomer : %c\n",
        ZOOM_IN, ZOOM_OUT
    );
    printf("Abandonner : %c | Recommencer : %c | Action précédente : %c\n", 
        GIVE_UP, RESTART, UNDO
    );
    afficher_ligne(longueurTexte);

    printf("Déplacements : %d\n\n", jeu.deplacements.nbDeplacements);
    afficher_ligne(longueurTexte);
}


/**
*
* @brief Affichage une case du plateau
* @param plateau de type t_partie, Entrée : structure de la partie
* @param ligne de type entier, Entrée : indice de la ligne
* @param colonne de type entier, Entrée : indice de la colonne
* Les joueurs sur une cible sont seulement représentés pas un joueur
* Les caisses sur une cible sont seulement représentés par une caisse
* Eléments importants en couleur
*
*/
void afficher_case(t_partie jeu, int ligne, int colonne)
{
    if(jeu.plateau[ligne][colonne] == PLAYER){
        printf("%s%c%s", JAUNE, PLAYER, FIN_COULEUR);
    }
    else if(jeu.plateau[ligne][colonne] == PLAYER_SUR_CIBLE){
        printf("%s%c%s", ROUGE, PLAYER, FIN_COULEUR);
    }
    else if (jeu.plateau[ligne][colonne] == CAISSE_SUR_CIBLE){
        printf("%s%c%s", VERT, CAISSE, FIN_COULEUR);
    }
    else if (jeu.plateau[ligne][colonne] == CIBLE){
        printf("%s%c%s", ROUGE, CIBLE, FIN_COULEUR);
    }
    else {
        printf("%c", jeu.plateau[ligne][colonne]);
    }
}


/**
*
* @brief Affichage du plateau
* @param plateau de type t_partie, Entrée : structure de la partie
*
*/
void afficher_plateau(t_partie jeu)
{
    for (int ligne = 0 ; ligne < recuperer_lignes_utiles(jeu) ; ligne++){
        for(int i = 0;i<jeu.zoom;i++){
            for (int colonne = 0 ; colonne < TAILLE ; colonne++){
                for(int i = 0;i<jeu.zoom;i++){
                    afficher_case(jeu, ligne, colonne);
                }
                
            }
            printf("\n");
        }
    }
}

/**
*
* @brief Démarrage d'une partie
* @param plateau de type t_partie, Entrée/Sortie : structure de la partie
* @return booléen : est ce que l'uilisateur veut recommencer la partie
*
*/
bool demarrer_partie(t_partie *jeu)
{
    char nomDuFichier[30];
    bool commencerPartie = true;
    afficher_encadre("SOKOBAN v2");

    printf("Entrez le nom du fichier de jeu (ou \"e\" pour fermer) : ");
    scanf("%s", nomDuFichier);

    strcpy(jeu->nomFichier, nomDuFichier);

    if(strcmp(nomDuFichier, "e") == 0){
        printf("Sortie du jeu...\n\n");
        jeu->estFinis = true;
        commencerPartie = false;
    }
    else{
        jeu->deplacements.nbDeplacements = 0;

        afficher_entete(*jeu);
        charger_partie(jeu->plateau, jeu->nomFichier);

        position_joueur(jeu);

        afficher_plateau(*jeu);
    }

    return commencerPartie;
}

/**
*
* @brief Récupérer la position x et y du joueur
* @param plateau de type t_partie, Entrée/Sortie : structure de la partie
*
*/
void position_joueur(t_partie *jeu)
{
    bool joueurTrouve = false;
    int ligne = 0;
    int colonne = 0;

    while(!joueurTrouve && ligne < TAILLE){
        while(!joueurTrouve && colonne < TAILLE){
            if(jeu->plateau[ligne][colonne] == PLAYER ||
                jeu->plateau[ligne][colonne] == PLAYER_SUR_CIBLE){
                jeu->joueur.posX = ligne;
                jeu->joueur.posY = colonne;
                joueurTrouve = true;
            }
            colonne++;
        }
        colonne = 0;
        ligne++;
    }
}

/**
*
* @brief Savoir si la partie est gagnée
* @param plateau de type t_partie, Entrée : structure de la partie
* @return vrai si la partie est gagnée sinon faux
*/
bool gagne(t_partie jeu)
{
    bool aGagner = true;

    for (int ligne = 0 ; ligne < TAILLE ; ligne++){
        for (int colonne = 0 ; colonne < TAILLE ; colonne++){
            // Si y a encore une cible ou qu'un joueur est sur une cible pas encore gagné
            if(
                jeu.plateau[ligne][colonne] == CIBLE ||
                jeu.plateau[ligne][colonne] == PLAYER_SUR_CIBLE
            ){
                aGagner = false;
            }
        }
    }

    return aGagner;
}

/**
*
* @brief Gérer le déplacement d'une caissez
* @param positionSuivante de type t_plateau, Entrée : position après le joueur
* @param positionApresSuivante de type entier, Entrée : position encore après
* @param plateau de type t_partie, Entrée/Sortie : structure de la partie
*/
void gestion_deplacement_caisse(char *positionSuivante,
    char *positionApresSuivante, t_partie *jeu){

    // Déplacer la caisse et si c'est une cible on met une caisse sur une cible
    if(*positionApresSuivante == CIBLE){
        *positionApresSuivante = CAISSE_SUR_CIBLE;
    }
    else{
        *positionApresSuivante = CAISSE;
    }
            
    // Déplacer le joueur et si c'est une cible ou une caisse sur une cible,
    //on met le joueur sur une cible sinon on met juste un joueur
    if(*positionSuivante == CIBLE || *positionSuivante == CAISSE_SUR_CIBLE){
        *positionSuivante = PLAYER_SUR_CIBLE;
    }
    else{
        *positionSuivante = PLAYER;
    }

    // Si le joueur est sur une cible on la remet
    if(jeu->plateau[jeu->joueur.posX][jeu->joueur.posY] == PLAYER_SUR_CIBLE){
        jeu->plateau[jeu->joueur.posX][jeu->joueur.posY] = CIBLE;
    }
    else
    {
        jeu->plateau[jeu->joueur.posX][jeu->joueur.posY] = VIDE;
    }

}

/**
*
* @brief Faire se déplacer le joueur
* @param plateau de type t_partie, Entrée/Sortie : structure de la partie
* @param depX de type entier, Entrée : position X du joueur.
* @param depY de type entier, Entrée : position Y du joueur.
*/
void deplacer(t_partie *jeu, int depX, int depY)
{

    char *positionSuivante =
        &jeu->plateau[jeu->joueur.posX+depX][jeu->joueur.posY+depY];
    char *positionApresSuivante =
        &jeu->plateau[jeu->joueur.posX+depX+depX][jeu->joueur.posY+depY+depY];

    // Si on est pas contre un bord et qu'on ne sort pas du plateau
    if(*positionSuivante != MUR && jeu->joueur.posX+depX < TAILLE &&
        jeu->joueur.posY+depY < TAILLE
        && jeu->joueur.posX+depX >= 0 && jeu->joueur.posY+depY >= 0)
    {
        // Si on ne pousse pas de caisse
        if(*positionSuivante != CAISSE && *positionSuivante != CAISSE_SUR_CIBLE)
        {

            // Si la position suivante est une cible
            //on ajoute un joueur sur la cible sinon on met juste un joueur
            if(*positionSuivante == CIBLE)
            {
                *positionSuivante = PLAYER_SUR_CIBLE;
            }
            else{
                *positionSuivante = PLAYER;
            }
            
            // Si le joueur est sur une cible on la remet sinon on met du vide
            if(jeu->plateau[jeu->joueur.posX][jeu->joueur.posY] == PLAYER_SUR_CIBLE){
                jeu->plateau[jeu->joueur.posX][jeu->joueur.posY] = CIBLE;
            }
            else
            {
                jeu->plateau[jeu->joueur.posX][jeu->joueur.posY] = VIDE;
            }

            jeu->joueur.posX += depX;
            jeu->joueur.posY += depY;


            stocker_deplacement(jeu, depX, depY, 0);
        }
        // Sinon on tente de pousser une caisse mais on vérifie que la case derrière
        // n'est pas un mur ou une autre caisse
        else if(
            *positionApresSuivante != MUR &&
            *positionApresSuivante != CAISSE &&
            *positionApresSuivante != CAISSE_SUR_CIBLE
        ){
            gestion_deplacement_caisse(positionSuivante, positionApresSuivante, jeu);
            stocker_deplacement(jeu, depX, depY, 1);

            jeu->joueur.posX += depX;
            jeu->joueur.posY += depY;
        }
    }
}

/**
*
* @brief Afficher une ligne de longeur X du caractère *
* @param longueur de type entier, Entrée : longueur de la ligne.
*/
void afficher_ligne(int longueur){
    for (int i = 0 ; i < longueur ; i++)
    {
        printf("*");
    }

    printf("\n");
}

/**
*
* @brief Afficher un encadré autour du texte
* @param texte de type chaîne de caractères, Entrée : texte à afficher.
* @return longeur du texte
*/
int afficher_encadre(char texte[]){

    // taille du texte
    int longueur = strlen(texte);
    int largeur = longueur + 4;

    afficher_ligne(largeur);
    printf("* %s *\n", texte);
    afficher_ligne(largeur);
    printf("\n");

    return largeur;
}

/**
*
* @brief Demande et gérer la sauvegarde lors de l'abandon
* @param plateau de type t_plateau, Entrée : plateau à sauvegarder.
*/
void gerer_sauvegarde(t_partie *jeu){
    char sauvePartie = 'n';

    char nomDuFichierSauvegarde[30];

    printf("Voulez-vous enregistrer la partie ");
    printf("(a = plateau + déplacements/o = plateau/n = non) : ");
    scanf("%c", &sauvePartie);

    if(sauvePartie == 'o' || sauvePartie == 'a'){

        printf("Dans quel fichier voulez vous sauvegarder la partie : ");
        scanf("%s", nomDuFichierSauvegarde);

        enregistrer_partie(jeu->plateau, nomDuFichierSauvegarde);
        printf("Partie sauvegardée !\n\n");
    }

    if(sauvePartie == 'a')
    {
        printf("Dans quel fichier voulez vous sauvegarder les déplacements : ");
        scanf("%s", nomDuFichierSauvegarde);

        enregistrer_deplacements(jeu->deplacements.liste,
            jeu->deplacements.nbDeplacements, nomDuFichierSauvegarde);
        printf("Déplacements sauvegardée !\n\n");
    }

    printf("\n\n");

    afficher_encadre("Vous avez abandonner :/");

    if(demarrer_partie(jeu)){
        jeu->deplacements.nbDeplacements = 0;
        position_joueur(jeu); 
    }
}

/**
*
* @brief Demander et gérer le fait de recommencer un niveau
* @param plateau de type t_partie, Entrée/Sortie : structure de la partie
*/
void gerer_redemarrage(t_partie *jeu){

    char touche = '\0';

    printf("Voulez vous recommencer (o/n) : ");
    scanf("%c", &touche);

    if(touche == 'o'){
        // On recharge le fichier 
        charger_partie(jeu->plateau, jeu->nomFichier);
        position_joueur(jeu);
        jeu->deplacements.nbDeplacements = 0;
        jeu->tentatives++;
    }
}

/**
*
* @brief Redistribuer les actions en fonction des touches pressées
* @param plateau de type t_partie, Entrée/Sortie : structure de la partie
* @param touche de type caractère, Entrée : touche pressée.
*/
void gerer_touches(t_partie *jeu, char touche){
        
    switch(touche)
    {
        case MOVE_UP:
            deplacer(jeu, -1, 0);
            break;
        case MOVE_LEFT:
            deplacer(jeu, 0, -1);
            break;
        case MOVE_DOWN:
            deplacer(jeu, 1, 0);
            break;
        case MOVE_RIGHT:
            deplacer(jeu, 0, 1);
            break;
        case GIVE_UP:
            gerer_sauvegarde(jeu);
            break;
        case ZOOM_IN:
            if(jeu->zoom < ZOOM_MAX){
                jeu->zoom++;
            }
            break;
            
        case ZOOM_OUT:
            if(jeu->zoom > ZOOM_MIN){
                jeu->zoom--;
            }
            break;

        case UNDO:
            if (jeu->deplacements.nbDeplacements > 0){
                retour_arriere(jeu);
            }
            
            break;

        case RESTART:
            gerer_redemarrage(jeu);
            break;
    }
}

/**
*
* @brief Stocker les déplacements dans le talbeau
* @param plateau de type t_partie, Entrée/Sortie : structure de la partie
* @param depX de type entier, Entrée : Indice de déplacement x
* @param depY de type entier, Entrée : Indice de déplacement y
* @param type de type entier, Entrée : Type de déplacement
* 0 = joueur simple, 1 = joueur et caisse
*/
void stocker_deplacement(t_partie *jeu, int depX, int depY, int type){

    if(depX == -1 && depY == 0){
        // syntaxe raccourcie de if pour la val d'une variable
        // x = a ? b : c
        // si a : x = b sinon x = c
        jeu->deplacements.liste[jeu->deplacements.nbDeplacements] =
            type ? DEP_SOK_CAI_H : DEP_SOK_HAU;
    }
    else if(depX == 0 && depY == -1){
        jeu->deplacements.liste[jeu->deplacements.nbDeplacements] =
            type ? DEP_SOK_CAI_G : DEP_SOK_GAU;
    }
    else if(depX == 1 && depY == 0){
        jeu->deplacements.liste[jeu->deplacements.nbDeplacements] =
            type ? DEP_SOK_CAI_B : DEP_SOK_BAS;
    }
    else{
        jeu->deplacements.liste[jeu->deplacements.nbDeplacements] =
            type ? DEP_SOK_CAI_D : DEP_SOK_DRO;
    }

    jeu->deplacements.nbDeplacements++;
}

/**
*
* @brief Renvoyer le type de déplacement pour le retour arrière
*   (coordonnées + si c'est une caisse)
* @param jeu de type t_partie, Entrée/Sortie : structure de la partie
* @param depCaisse de type booléen, Entrée/Sortie : si on déplace une caisse
* @param depX de type entier, Entrée/Sortie : déplacement axe x
* @param depY de type entier, Entrée/Sortie : déplacement axe y
*
*/
void recuperer_deplacement(t_partie *jeu, bool *depCaisse, int *depX, int *depY)
{
    char deplacement = jeu->deplacements.liste[jeu->deplacements.nbDeplacements-1];

    // Si le déplacement est une majuscule alors c'est qu'on bouge une caisse
    if(isupper(deplacement)){
        *depCaisse = true;
    }

    // Mettre en minuscule pour récupérer juste l'axe de déplacement
    switch(tolower(deplacement)){
        case DEP_SOK_GAU:
            *depY = 1;
            break;
        case DEP_SOK_HAU:
            *depX = 1;
            break;
        case DEP_SOK_BAS:
            *depX = -1;
            break;
        case DEP_SOK_DRO:
            *depY = -1;
            break;
    }

    jeu->deplacements.nbDeplacements--;
}


/**
*
* @brief Revenir à l'action précédente
* @param jeu de type t_partie, Entrée/Sortie : structure de la partie
*
*/
void retour_arriere(t_partie *jeu){

    int depX = 0;
    int depY = 0;
    bool depCaisse = false;

    // Récupérer axe déplacement et si on bouge une caisse
    recuperer_deplacement(jeu, &depCaisse, &depX, &depY);

    char *positionJoueur = &jeu->plateau[jeu->joueur.posX][jeu->joueur.posY];
    char *positionApres =
        &jeu->plateau[jeu->joueur.posX+depX][jeu->joueur.posY+depY];
    char *positionAvant =
        &jeu->plateau[jeu->joueur.posX-depX][jeu->joueur.posY-depY];

    /* Déplacer le joueur */

    // Si la position suivante est une cible on met un joueur sur une cible
    if(*positionApres == CIBLE){
        *positionApres = PLAYER_SUR_CIBLE;
    }
    else{
        *positionApres = PLAYER;
    }

    if(depCaisse){
        // Si la position ou va aller la caisse est une cible on met
        // une caisse sur une cible
        if(*positionJoueur == PLAYER_SUR_CIBLE || *positionJoueur == CIBLE){
            *positionJoueur = CAISSE_SUR_CIBLE;
        }
        else{
            *positionJoueur = CAISSE;
        }
        
        if(*positionAvant == CAISSE_SUR_CIBLE){
            *positionAvant = CIBLE;
        }
        else{
            *positionAvant = VIDE;
        }
    }

    else{
        if(*positionJoueur == PLAYER_SUR_CIBLE){
            *positionJoueur = CIBLE;
        }
        else{
            *positionJoueur = VIDE;
        }
    }

    jeu->joueur.posX += depX;
    jeu->joueur.posY += depY;
}

/**
*
* @brief Définir les valeurs de la partie par défaut
* @param plateau de type t_partie, Entrée/Sortie : structure de la partie
*
*/
void initialiser_jeu(t_partie *jeu){
    jeu->deplacements.nbDeplacements = 0;
    jeu->estFinis = false;
    jeu->zoom = ZOOM_MIN;
    jeu->tentatives  = 1;
}

/**
*
* @brief Récupérer le nombre de lignes vraiment utiles à l'affichage
*  du plateau
* @return entier : nombre de lignes utiles
*/
int recuperer_lignes_utiles(t_partie jeu){
    int lignesAAfficher = TAILLE;
    int ligne = TAILLE - 1;
    int colonne = 0;
    bool finAtteinte = false;
    bool ligneVide;

    while(!finAtteinte && ligne >= 0){
        ligneVide = true;
        while(!finAtteinte && colonne < TAILLE){
            if(jeu.plateau[ligne][colonne] != VIDE){
                finAtteinte = true;
                ligneVide = false;
            }
            colonne++;
        }

        if(ligneVide){
            lignesAAfficher--;
        }

        colonne = 0;
        ligne--;
    }

    return lignesAAfficher;
}

/**
*
* @brief Afficher la victoire et gérer le redémarrage
* @param plateau de type t_partie, Entrée/Sortie : structure de la partie
*
*/
void gerer_gagner(t_partie *jeu){
    afficher_encadre("Vous avez gagner !");
    printf(
        "Il vous a fallu %d déplacements et %d tentative(s) pour finir ce niveau.",
        jeu->deplacements.nbDeplacements, jeu->tentatives);

    printf("\n\n\n");

    if(demarrer_partie(jeu)){
        jeu->deplacements.nbDeplacements = 0;
        position_joueur(jeu); 
    }
}

/* Fonctions fournies */

void charger_partie(t_plateau plateau, char fichier[]){
    FILE * f;
    char finDeLigne;

    f = fopen(fichier, "r");
    if (f==NULL){
        printf("ERREUR SUR FICHIER");
        exit(EXIT_FAILURE);
    } else {
        for (int ligne = 0 ; ligne < TAILLE ; ligne++){
            for (int colonne = 0 ; colonne < TAILLE ; colonne++){
                fread(&plateau[ligne][colonne], sizeof(char), 1, f);
            }
            fread(&finDeLigne, sizeof(char), 1, f);
        }
        fclose(f);
    }
}

void enregistrer_partie(t_plateau plateau, char fichier[]){
    FILE * f;
    char finDeLigne = '\n';

    f = fopen(fichier, "w");
    for (int ligne=0 ; ligne<TAILLE ; ligne++){
        for (int colonne=0 ; colonne<TAILLE ; colonne++){
            fwrite(&plateau[ligne][colonne], sizeof(char), 1, f);
        }
        fwrite(&finDeLigne, sizeof(char), 1, f);
    }
    fclose(f);
}

int kbhit(){
	// la fonction retourne :
	// 1 si un caractere est present
	// 0 si pas de caractere présent
	int unCaractere=0;
	struct termios oldt, newt;
	int ch;
	int oldf;

	// mettre le terminal en mode non bloquant
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
	ch = getchar();

	// restaurer le mode du terminal
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
 
	if(ch != EOF){
		ungetc(ch, stdin);
		unCaractere=1;
	} 
	return unCaractere;
}

void enregistrer_deplacements(t_tabDeplacement t, int nb, char fic[]){
    FILE * f;

    f = fopen(fic, "w");
    fwrite(t,sizeof(char), nb, f);
    fclose(f);
}