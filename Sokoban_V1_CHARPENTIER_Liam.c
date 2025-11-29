/**
* @file main.c
* @brief Jeu Sokaban
* @author Liam CHARPENTIER
* @version 1
* @date 21/10/2025
*
* Première version du jeu Sokoban dans le terminal
*
*/

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define TAILLE 12

/* Définition des touches */

#define MOVE_LEFT   'q'
#define MOVE_RIGHT  'd'
#define MOVE_DOWN   's'
#define MOVE_UP     'z'
#define GIVE_UP     'x'
#define RESTART     'r'

/* Définition des caractères */

const char PLAYER           = '@';
const char MUR              = '#';
const char CAISSE           = '$';
const char CIBLE            = '.';
const char PLAYER_SUR_CIBLE = '+';
const char CAISSE_SUR_CIBLE = '*';
const char VIDE             = ' ';

/* Définition du tableau */

typedef char t_plateau[TAILLE][TAILLE];

/* Fonctions */

void charger_partie(t_plateau plateau, char fichier[]);
void enregistrer_partie(t_plateau plateau, char fichier[]);
int kbhit();
void afficher_entete(char filename[], int deplacements);
void afficher_plateau(t_plateau plateau);
void demarrer_partie(t_plateau plateau, char nomDuFichier[30]);
void position_joueur(t_plateau plateau, int *posX, int *posY);
bool gagne(t_plateau plateau);
void deplacer(t_plateau plateau, int posX, int posY, int *deplacements);
void afficher_ligne(int longueur);
int afficher_encadre(char texte[]);
void gerer_sauvegarde(t_plateau plateau);
void gestion_deplacement_caisse(t_plateau plateau, char *positionSuivante,
    char *positionApresSuivante, int *deplacements, int playerX, int playerY);
void gerer_redemarrage(t_plateau plateau, char nomDuFichier[30],
    int *deplacements, int *tentatives);
void gerer_touches(t_plateau plateau, char touche,
    int *deplacements, int *tentatives, char nomDuFichier[30], bool *isFinished);


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
    int deplacements = 0;
    int tentatives = 1;
    bool isFinished = false;

    demarrer_partie(plateau, nomDuFichier);

    while(!isFinished)
    {
        if (kbhit()){
            touche = getchar();

            gerer_touches(plateau, touche, &deplacements, &tentatives, nomDuFichier, &isFinished);

            if(!isFinished){
                afficher_entete(nomDuFichier, deplacements);
                afficher_plateau(plateau);
            }

            if(gagne(plateau)){
                afficher_encadre("Vous avez gagner !");
                printf("Il vous a fallu %d déplacements et %d tentative(s) pour finir ce niveau.\n", deplacements, tentatives);
                isFinished = true;
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
    longueurTexte = afficher_encadre("SOKOBAN v1 - Liam CHARPENTIER");

    printf("Fichier chargé : %s\n", filename);
    afficher_ligne(longueurTexte);

    printf("Haut : %c - Bas : %c\nGauche : %c - Droite : %c\n",
        MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT
    );
    printf("Abandonner : x - Recommencer : r\n");
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
void afficher_plateau(t_plateau plateau)
{
    for (int ligne = 0 ; ligne < TAILLE ; ligne++){
        for (int colonne = 0 ; colonne < TAILLE ; colonne++){
            if(plateau[ligne][colonne] == PLAYER_SUR_CIBLE){
                printf("%c", PLAYER);
            }
            else if (plateau[ligne][colonne] ==CAISSE_SUR_CIBLE){
                printf("%c", CAISSE);
            }
            else {
                printf("%c", plateau[ligne][colonne]);
            }
            
        }
        printf("\n");
    }
}

/**
*
* @brief Démarrage d'une partie
* @param plateau de type t_plateau, Entrée : tableau du plateau.
* @param nomDuFichier de type chaîne de caractères Entrée/Sortie
*   nom du fichier niveau.
*
*/
void demarrer_partie(t_plateau plateau, char nomDuFichier[30])
{
    afficher_encadre("SOKOBAN v1");

    printf("Entrez le nom du fichier de jeu : ");
    scanf("%s", nomDuFichier);
    
    afficher_entete(nomDuFichier, 0);
    charger_partie(plateau, nomDuFichier);

    afficher_plateau(plateau);
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
    for (int ligne = 0 ; ligne < TAILLE ; ligne++){
        for (int colonne = 0 ; colonne < TAILLE ; colonne++){
            if(plateau[ligne][colonne] == PLAYER ||
                plateau[ligne][colonne] == PLAYER_SUR_CIBLE){
                (*posX) = ligne;
                (*posY) = colonne;
                // quitter pour ne pas continuer le reste du tableau
                return;
            }
        }
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
* @param deplacements de type entier, Entrée/Sortie : nombre de déplacements.
*/
void gestion_deplacement_caisse(t_plateau plateau, char *positionSuivante,
    char *positionApresSuivante, int *deplacements, int playerX, int playerY){

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
    if(plateau[playerX][playerY] == PLAYER_SUR_CIBLE){
        plateau[playerX][playerY] = CIBLE;
    }
    else
    {
        plateau[playerX][playerY] = VIDE;
    }

    (*deplacements) += 1;
}

/**
*
* @brief Faire se déplacer le joueur
* @param plateau de type t_plateau, Entrée : tableau du plateau.
* @param posX de type entier, Entrée : position X du joueur.
* @param posY de type entier, Entrée : position Y du joueur.
* @param deplacements de type entier, Entrée/Sortie : nombre de déplacements.
*/
void deplacer(t_plateau plateau, int posX, int posY, int *deplacements)
{
    int playerX, playerY;
    position_joueur(plateau, &playerX, &playerY);
    char *positionSuivante = &plateau[playerX+posX][playerY+posY];
    char *positionApresSuivante = &plateau[playerX+posX+posX][playerY+posY+posY];

    // Si on est pas contre un bord et qu'on ne sort pas du plateau
    if(*positionSuivante != MUR && playerX+posX < TAILLE && playerY+posY < TAILLE
        && playerX+posX >= 0 && playerY+posY >= 0)
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
            if(plateau[playerX][playerY] == PLAYER_SUR_CIBLE){
                plateau[playerX][playerY] = CIBLE;
            }
            else
            {
                plateau[playerX][playerY] = VIDE;
            }

            (*deplacements) += 1;
        }
        // Sinon on tente de pousser une caisse mais on vérifie que la case derrière
        // n'est pas un mur ou une autre caisse
        else if(
            *positionApresSuivante != MUR &&
            *positionApresSuivante != CAISSE &&
            *positionApresSuivante != CAISSE_SUR_CIBLE
        ){
            gestion_deplacement_caisse(plateau, positionSuivante,
                positionApresSuivante, deplacements, playerX, playerY);
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
void gerer_sauvegarde(t_plateau plateau){
    char sauvePartie = 'n';
    char nomDuFichier[30];

    printf("Voulez-vous enregistrer la partie (o/n) : ");
    scanf("%c", &sauvePartie);

    if(sauvePartie == 'o'){

        printf("Dans quel fichier voulez vous sauvegarder la partie : ");
        scanf("%s", nomDuFichier);

        enregistrer_partie(plateau, nomDuFichier);
        printf("Partie sauvegardée !\n\n");
    }

    afficher_encadre("Vous avez abandonner :/");
}

/**
*
* @brief Demander et gérer le fait de recommencer un niveau
* @param plateau de type t_plateau, Entrée : plateau de jeu.
* @param deplacements de type entier, Entrée/Sortie : nombre de déplacements.
* @param nomDuFichier de type caractère, Entrée : Fichier de jeu.
*/
void gerer_redemarrage(t_plateau plateau, char nomDuFichier[30],
    int *deplacements, int *tentatives){

    char touche = '\0';

    printf("Voulez vous recommencer (o/n) : ");
    scanf("%c", &touche);

    if(touche == 'o'){
        // On recharge le fichier 
        charger_partie(plateau, nomDuFichier);
        *deplacements = 0;
        (*tentatives)++;
    }
}

/**
*
* @brief Redistribuer les actions en fonction des touches pressées
* @param plateau de type t_plateau, Entrée : plateau de jeu.
* @param touche de type caractère, Entrée : touche pressée.
* @param deplacements de type entier, Entrée/Sortie : nombre de déplacements.
* @param nomDuFichier de type caractère, Entrée : Fichier de jeu.
* @param isFinished de type booléen, Entrée : La partie est elle terminée.
*/
void gerer_touches(t_plateau plateau, char touche, int *deplacements,
     int *tentatives, char nomDuFichier[30], bool *isFinished){
    switch(touche)
    {
        case MOVE_UP:
            deplacer(plateau, -1, 0, deplacements);
            break;
        case MOVE_LEFT:
            deplacer(plateau, 0, -1, deplacements);
            break;
        case MOVE_DOWN:
            deplacer(plateau, 1, 0, deplacements);
            break;
        case MOVE_RIGHT:
            deplacer(plateau, 0, 1, deplacements);
            break;
        case GIVE_UP:
            *isFinished = true;
            gerer_sauvegarde(plateau);
            break;
        case RESTART:
            gerer_redemarrage(plateau, nomDuFichier, deplacements, tentatives);
            break;
    }
}