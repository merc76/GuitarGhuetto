/*********************************************************************************************************/
//FICHIER serveur.c
//auteur MMR, FTAFFIN
//version 0.1
/*********************************************************************************************************/
/*********************************************************************************************************/
//includes
#include <stdio.h>
#include <sys/msg.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

/*********************************************************************************************************/
//defines
#define MAX_JOUEUR_DEFAUT 4
#define TEMPS_DEBUT_PARTIE_DEFAUT 120
#define FIC_BAL "./BALREP"

int main(int argc, char argv[]){
    int nbJoueurMax = MAX_JOUEUR_DEFAUT;
    int tempsDebutPartieS = TEMPS_DEBUT_PARTIE_DEFAUT;

    if(argc == 3){
        if(nbJoueurMax = atoi(argv[1]) > 4 || nbJoueurMax < 2){
            printf("nombre de joueur incorrect\n");
            printf(" 2 < joueurMax < 4 (inclues)\n");
            exit(EXIT_FAILURE);
        }
        if(tempsDebutPartieS = argv[2] > 280 || tempsDebutPartieS < 60){
            printf("temps avant début de partie inccorect\n");
            printf(" 60sec < temps début < 280sec (inclues)\n");
            exit(EXIT_FAILURE);
        }
    }

    

    return 0;
}