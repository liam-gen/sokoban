/**
* @file animations.c
* @brief Jeu Sokaban
* @author Liam CHARPENTIER
* @version 2.1
* @date 20/11/2025
*
* Version modifiée avec animations
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
    t_tabDeplacement deplacements;
    int nbDeplacements;
} t_deplacements;

// Structure joueur
typedef struct {
    int posX;
    int posY;
} t_joueur;

// Structure partie
typedef struct {
    bool estFinis;
    char nomFichier;
    int zoom;
    int tentatives;
    t_deplacements deplacements;
    t_plateau plateau;
    t_joueur joueur;
} t_partie;

/* Fonctions */

void charger_partie(t_plateau plateau, char fichier[]);
void enregistrer_partie(t_plateau plateau, char fichier[]);
int kbhit();
void afficher_entete(char filename[], int deplacements);
void afficher_plateau(t_plateau plateau, int zoom);
bool demarrer_partie(t_plateau plateau, char nomDuFichier[30], bool *isFinished);
void position_joueur(t_plateau plateau, int *posX, int *posY);
bool gagne(t_plateau plateau);
void deplacer(t_plateau plateau, int posX, int posY, t_deplacements *deplacements,
    int *posJoueurX, int *posJoueurY);
void afficher_ligne(int longueur);
int afficher_encadre(char texte[]);
void gerer_sauvegarde(t_plateau plateau, t_deplacements *deplacements);
void gestion_deplacement_caisse(t_plateau plateau, char *positionSuivante,
    char *positionApresSuivante, t_deplacements *deplacements, int *playerX, int *playerY);
void gerer_redemarrage(t_plateau plateau, char nomDuFichier[30],
    t_deplacements *deplacements, int *tentatives, int *posJoueurX, int *posJoueurY);
void gerer_touches(t_plateau plateau, char touche, t_deplacements *deplacements,
    int *tentatives, char nomDuFichier[30], bool *isFinished, int *zoom,
    int *posJoueurX, int *posJoueurY);
void stocker_deplacement(t_deplacements *deplacements, int posX, int posY, int type);

void retour_arriere(t_plateau plateau, t_deplacements *deplacements,
    int *playerX, int *playerY);

void afficher_frame(char name[], char nb[]);
void afficher_gif(char name[], int nbFrames);

/**
*
* @brief Récupérer les infos du niveau, lancer le jeu et gérer les touches
* @return entier : code de sortie
*/
int main()
{
    /* Variables */
    char touche = '\0', nomDuFichier[30];
    t_plateau plateau;
    int tentatives = 1;
    bool isFinished = false;
    int zoom = ZOOM_MIN;
    t_deplacements deplacements;
    deplacements.nbDeplacements = 0;

    int posJoueurX;
    int posJoueurY;

    // Si le joueur ne choisis pas de quitter
    if(demarrer_partie(plateau, nomDuFichier, &isFinished)){

        position_joueur(plateau, &posJoueurX, &posJoueurY);    

        while(!isFinished)
        {
            if (kbhit()){
                touche = getchar();

                gerer_touches(plateau, touche, &deplacements, &tentatives, nomDuFichier,
                    &isFinished, &zoom, &posJoueurX, &posJoueurY);

                if(!isFinished){
                    afficher_entete(nomDuFichier, deplacements.nbDeplacements);
                    afficher_plateau(plateau, zoom);
                }

                if(gagne(plateau)){
                    afficher_encadre("Vous avez gagner !");
                    printf("Il vous a fallu %d déplacements et %d tentative(s) pour finir ce niveau.\n",
                        deplacements.nbDeplacements, tentatives);
                        
                    afficher_gif("victory", 5);
                    //isFinished = true;

                    if(demarrer_partie(plateau, nomDuFichier, &isFinished)){
                        deplacements.nbDeplacements = 0;
                        position_joueur(plateau, &posJoueurX, &posJoueurY); 
                    }
                }
            }
        }
    }

    

    return 0;
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

/* Fonction développées */

/**
*
* @brief Affichage de l'entête
* @param filename de type chaîne de caractères, Entrée : nom du fichier niveau.
* @param deplacements de type entier, Entrée : Nombre de déplacements effectués
*
*/
void afficher_entete(char filename[], int deplacements)
{
    int longueurTexte;

    system("clear");
    longueurTexte = afficher_encadre("SOKOBAN v2 - Liam CHARPENTIER");

    printf("Fichier chargé : %s\n", filename);
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

    printf("Déplacements : %d\n\n", deplacements);
    afficher_ligne(longueurTexte);
}

/**
*
* @brief Affichage du plateau
* @param plateau de type t_plateau, Entrée : tableau du plateau.
* Les joueurs sur une cible sont seulement représentés pas un joueur
* Les caisses sur une cible sont seulement représentés par une caisse
*
*/
void afficher_plateau(t_plateau plateau, int zoom)
{
    for (int ligne = 0 ; ligne < TAILLE ; ligne++){
        for(int i = 0;i<zoom;i++){
            for (int colonne = 0 ; colonne < TAILLE ; colonne++){
                for(int i = 0;i<zoom;i++){
                    if(plateau[ligne][colonne] == PLAYER_SUR_CIBLE){
                        printf("%c", PLAYER);
                    }
                    else if (plateau[ligne][colonne] == CAISSE_SUR_CIBLE){
                        printf("%c", CAISSE);
                    }
                    else {
                        printf("%c", plateau[ligne][colonne]);
                    }
                }
                
            }
            printf("\n");
        }
    }
}

/**
*
* @brief Démarrage d'une partie
* @param plateau de type t_plateau, Entrée : tableau du plateau.
* @param nomDuFichier de type chaîne de caractères Entrée/Sortie
*   nom du fichier niveau.
* TODO: prendre en paramètre déplacements et position joueur pour éviter redondance
*/
bool demarrer_partie(t_plateau plateau, char nomDuFichier[30], bool *isFinished,
    t_deplacements *deplacements, )
{
    bool commencerPartie = true;
    afficher_encadre("SOKOBAN v2");

    printf("Entrez le nom du fichier de jeu (ou \"e\" pour fermer) : ");
    scanf("%s", nomDuFichier);

    if(strcmp(nomDuFichier, "e") == 0){
        printf("Sortie du jeu...");
        *isFinished = true;
        commencerPartie = false;
    }
    else{
        deplacements.nbDeplacements = 0;

        afficher_entete(nomDuFichier, 0);
        charger_partie(plateau, nomDuFichier);

        position_joueur(plateau, &posJoueurX, &posJoueurY);

        afficher_plateau(plateau, ZOOM_MIN);
    }

    return commencerPartie;
}

/**
*
* @brief Récupérer la position x et y du joueur
* @param plateau de type t_plateau, Entrée : tableau du plateau.
* @param posX de type entier, Sortie : position X du joueur.
* @param posY de type entier, Sortie : position Y du joueur.
*
*/
void position_joueur(t_plateau plateau, int *posX, int *posY)
{
    bool joueurTrouve = false;
    int ligne = 0;
    int colonne = 0;

    while(!joueurTrouve && ligne < TAILLE){
        while(!joueurTrouve && colonne < TAILLE){
            if(plateau[ligne][colonne] == PLAYER ||
                plateau[ligne][colonne] == PLAYER_SUR_CIBLE){
                (*posX) = ligne;
                (*posY) = colonne;
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
* @param plateau de type t_plateau, Entrée : tableau du plateau.
* @return vrai si la partie est gagnée sinon faux
*/
bool gagne(t_plateau plateau)
{
    bool aGagner = true;

    for (int ligne = 0 ; ligne < TAILLE ; ligne++){
        for (int colonne = 0 ; colonne < TAILLE ; colonne++){
            // Si y a encore une cible ou qu'un joueur est sur une cible pas encore gagné
            if(
                plateau[ligne][colonne] == CIBLE || plateau[ligne][colonne] == PLAYER_SUR_CIBLE
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
* @param deplacements de type t_deplacements, Entrée/Sortie : déplacements
*/
void gestion_deplacement_caisse(t_plateau plateau, char *positionSuivante,
    char *positionApresSuivante, t_deplacements *deplacements, int *playerX, int *playerY){

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
    if(plateau[*playerX][*playerY] == PLAYER_SUR_CIBLE){
        plateau[*playerX][*playerY] = CIBLE;
    }
    else
    {
        plateau[*playerX][*playerY] = VIDE;
    }

}

/**
*
* @brief Faire se déplacer le joueur
* @param plateau de type t_plateau, Entrée : tableau du plateau.
* @param posX de type entier, Entrée : position X du joueur.
* @param posY de type entier, Entrée : position Y du joueur.
* @param deplacements de type t_deplacements, Entrée/Sortie : déplacements.
*/
void deplacer(t_plateau plateau, int posX, int posY, t_deplacements *deplacements,
    int *posJoueurX, int *posJoueurY)
{
    char *positionSuivante = &plateau[*posJoueurX+posX][*posJoueurY+posY];
    char *positionApresSuivante = &plateau[*posJoueurX+posX+posX][*posJoueurY+posY+posY];

    // Si on est pas contre un bord et qu'on ne sort pas du plateau
    if(*positionSuivante != MUR && *posJoueurX+posX < TAILLE && *posJoueurY+posY < TAILLE
        && *posJoueurX+posX >= 0 && *posJoueurY+posY >= 0)
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
            if(plateau[*posJoueurX][*posJoueurY] == PLAYER_SUR_CIBLE){
                plateau[*posJoueurX][*posJoueurY] = CIBLE;
            }
            else
            {
                plateau[*posJoueurX][*posJoueurY] = VIDE;
            }

            (*posJoueurX) += posX;
            (*posJoueurY) += posY;


            stocker_deplacement(deplacements, posX, posY, 0);
        }
        // Sinon on tente de pousser une caisse mais on vérifie que la case derrière
        // n'est pas un mur ou une autre caisse
        else if(
            *positionApresSuivante != MUR &&
            *positionApresSuivante != CAISSE &&
            *positionApresSuivante != CAISSE_SUR_CIBLE
        ){
            gestion_deplacement_caisse(plateau, positionSuivante,
                positionApresSuivante, deplacements, posJoueurX, posJoueurY);
            stocker_deplacement(deplacements, posX, posY, 1);

            (*posJoueurX) += posX;
            (*posJoueurY) += posY;
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
void gerer_sauvegarde(t_plateau plateau, t_deplacements *deplacements){
    char sauvePartie = 'n';
    char nomDuFichier[30];

    printf("Voulez-vous enregistrer la partie (a = plateau + déplacements/o = plateau/n = non)");
    scanf("%c", &sauvePartie);

    if(sauvePartie == 'o' || sauvePartie == 'a'){

        printf("Dans quel fichier voulez vous sauvegarder la partie : ");
        scanf("%s", nomDuFichier);

        enregistrer_partie(plateau, nomDuFichier);
        printf("Partie sauvegardée !\n\n");
    }

    if(sauvePartie == 'a')
    {
        printf("Dans quel fichier voulez vous sauvegarder les déplacements : ");
        scanf("%s", nomDuFichier);

        enregistrer_deplacements(deplacements->deplacements,
            deplacements->nbDeplacements, nomDuFichier);
        printf("Déplacements sauvegardée !\n\n");
    }

    afficher_encadre("Vous avez abandonner :/");

    afficher_gif("ouin", 10);
}

/**
*
* @brief Demander et gérer le fait de recommencer un niveau
* @param plateau de type t_plateau, Entrée : plateau de jeu.
* @param deplacements de type t_deplacements, Entrée/Sortie : nombre de déplacements.
* @param nomDuFichier de type caractère, Entrée : Fichier de jeu.
*/
void gerer_redemarrage(t_plateau plateau, char nomDuFichier[30],
    t_deplacements *deplacements, int *tentatives, int *posJoueurX, int *posJoueurY){

    char touche = '\0';

    printf("Voulez vous recommencer (o/n) : ");
    scanf("%c", &touche);

    if(touche == 'o'){
        // On recharge le fichier 
        charger_partie(plateau, nomDuFichier);
        position_joueur(plateau, posJoueurX, posJoueurY);
        deplacements->nbDeplacements = 0;
        (*tentatives)++;
    }
}

/**
*
* @brief Redistribuer les actions en fonction des touches pressées
* @param plateau de type t_plateau, Entrée : plateau de jeu.
* @param touche de type caractère, Entrée : touche pressée.
* @param deplacements de type t_deplacements, Entrée/Sortie : nombre de déplacements.
* @param nomDuFichier de type caractère, Entrée : Fichier de jeu.
* @param isFinished de type booléen, Entrée : La partie est elle terminée.
*/
void gerer_touches(t_plateau plateau, char touche, t_deplacements *deplacements,
     int *tentatives, char nomDuFichier[30], bool *isFinished, int *zoom, int *posJoueurX, int *posJoueurY){
        
    switch(touche)
    {
        case MOVE_UP:
            deplacer(plateau, -1, 0, deplacements, posJoueurX, posJoueurY);
            break;
        case MOVE_LEFT:
            deplacer(plateau, 0, -1, deplacements, posJoueurX, posJoueurY);
            break;
        case MOVE_DOWN:
            deplacer(plateau, 1, 0, deplacements, posJoueurX, posJoueurY);
            break;
        case MOVE_RIGHT:
            deplacer(plateau, 0, 1, deplacements, posJoueurX, posJoueurY);
            break;
        case GIVE_UP:
            *isFinished = true;
            gerer_sauvegarde(plateau, deplacements);
            break;
        case ZOOM_IN:
            if(*zoom < ZOOM_MAX){
                (*zoom)++;
            }
            break;
            
        case ZOOM_OUT:
            if(*zoom > ZOOM_MIN){
                (*zoom)--;
            }
            break;

        case UNDO:
            if (deplacements->nbDeplacements > 0){
                retour_arriere(plateau, deplacements, posJoueurX, posJoueurY);
            }
            
            break;

        case RESTART:
            gerer_redemarrage(plateau, nomDuFichier, deplacements, tentatives,
                posJoueurX, posJoueurY);
            break;
    }
}

/**
*
* @brief Stocker les déplacements dans le talbeau
* @param posX de type entier, Entrée : Indice de déplacement x
* @param posY de type entier, Entrée : Indice de déplacement y
* @param type de type entier, Entrée : Type de déplacement
* 0 = joueur simple, 1 = joueur et caisse
*/
void stocker_deplacement(t_deplacements *deplacements, int posX, int posY, int type){

    if(posX == -1 && posY == 0){
        deplacements->deplacements[deplacements->nbDeplacements] = type ? DEP_SOK_CAI_H : DEP_SOK_HAU;
    }
    else if(posX == 0 && posY == -1){
        deplacements->deplacements[deplacements->nbDeplacements] = type ? DEP_SOK_CAI_G : DEP_SOK_GAU;
    }
    else if(posX == 1 && posY == 0){
        deplacements->deplacements[deplacements->nbDeplacements] = type ? DEP_SOK_CAI_B : DEP_SOK_BAS;
    }
    else{
        deplacements->deplacements[deplacements->nbDeplacements] = type ? DEP_SOK_CAI_D : DEP_SOK_DRO;
    }

    deplacements->nbDeplacements++;
}

// TODO: MOINS DE 60 LIGNES PAR PITIE
/**
*
* @brief Revenir à l'action précédente
* @param plateau de type t_plateau, Entrée : plateau de jeu.
* @param tabDeplacement de type t_deplacements, Entrée/Sortie : Structure avec les déplacements.
* @param deplacements de type entier, Entrée/Sortie : Nombre de déplacements.
*
*/
void retour_arriere(t_plateau plateau, t_deplacements *deplacements, 
    int *playerX, int *playerY){

    char deplacement = deplacements->deplacements[deplacements->nbDeplacements-1];

    int depX = 0;
    int depY = 0;
    bool depCaisse = false;

    // Si le déplacement est une majuscule alors c'est qu'on bouge une caisse
    if(isupper(deplacement)){
        depCaisse = true;
    }

    // Mettre en minuscule pour récupérer juste l'axe de déplacement
    switch(tolower(deplacement)){
        case DEP_SOK_GAU:
            depY = 1;
            break;
        case DEP_SOK_HAU:
            depX = 1;
            break;
        case DEP_SOK_BAS:
            depX = -1;
            break;
        case DEP_SOK_DRO:
            depY = -1;
            break;
    }

    /* Déplacer le joueur */
    if(plateau[*playerX+depX][*playerY+depY] == CIBLE){
        plateau[*playerX+depX][*playerY+depY] = PLAYER_SUR_CIBLE;
    }
    else{
        plateau[*playerX+depX][*playerY+depY] = PLAYER;
    }

    if(depCaisse){
        if(plateau[*playerX][*playerY] == PLAYER_SUR_CIBLE || plateau[*playerX][*playerY] == CIBLE){
            plateau[*playerX][*playerY] = CAISSE_SUR_CIBLE;
        }
        else{
            plateau[*playerX][*playerY] = CAISSE;
        }
        
        if(plateau[*playerX-depX][*playerY-depY] == CAISSE_SUR_CIBLE){
            plateau[*playerX-depX][*playerY-depY] = CIBLE;
        }
        else{
            plateau[*playerX-depX][*playerY-depY] = VIDE;
        }
    }

    else{
        if(plateau[*playerX][*playerY] == PLAYER_SUR_CIBLE){
            plateau[*playerX][*playerY] = CIBLE;
        }
        else{
            plateau[*playerX][*playerY] = VIDE;
        }
    }

    *playerX = *playerX+depX;
    *playerY = *playerY+depY;

    deplacements->nbDeplacements--;
}

void afficher_frame(char name[], char nb[]){
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    char nomFichier[100] = "frames/";
    strcat(nomFichier, name);
    strcat(nomFichier, "/f");
    strcat(nomFichier, nb);

    printf("\n%s\n", nb);

    fp = fopen(nomFichier, "r");
    if (fp == NULL){
        printf("ERREUR FICHIER");
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        printf("%s", line);
    }

    fclose(fp);
    if (line)
        free(line);

    printf("\n");
}


void afficher_gif(char name[], int nbFrames){
    
    for(int i = 1;i<5;i++){
        char str[20];
        for(int z = 1;z<nbFrames;z++){
            
            sprintf(str, "%d", z);

            afficher_frame(name, str);

            usleep(100 * 1000);
            system("clear");
        }
    }
}