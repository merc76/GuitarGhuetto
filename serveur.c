/********************************************************************************************************
 * @file serveur.c
 * @author MMR, FTAFFIN
 * @brief
 * @version 0.1
 * @date 2022-11-29
 *
 * @copyright Copyright (c) 2022
 *
 *********************************************************************************************************/

//includes
#include <stdio.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include "GuitarGhetto.h"

void clientDeroute(int, siginfo_t*, void*);

/**
 * @brief
 *
 * @param argc nombre d'argument
 * @param argv les arguments
 * @return int
 */
int main(int argc, char * argv[]){
    int nbJoueurMax = MAX_JOUEUR_DEFAUT;
    int tempsDebutPartieS = TEMPS_DEBUT_PARTIE_DEFAUT;
    scoreAll_t test;

// si le nombre d'argument est egal a 3 alors on recupere les valeurs
    if(argc == 3){
        nbJoueurMax = atoi(argv[1]);
        tempsDebutPartieS = atoi(argv[2]);
        // si le nombre de joueur est inferieur a 2 ou superieur a 4 alors on affiche un message d'erreur
        if(nbJoueurMax > 4 || nbJoueurMax < 2){
            printf("nombre de joueur incorrect\n");
            printf(" 2 < joueurMax < 4 (inclues)\n");
            exit(EXIT_FAILURE);
        }
        // si le temps de debut de partie est inferieur a 10 ou superieur a 60 alors on affiche un message d'erreur
        if(tempsDebutPartieS > 280 || tempsDebutPartieS < 60){
            printf("temps avant début de partie inccorect\n");
            printf(" 60sec < temps début < 280sec (inclues)\n");
            exit(EXIT_FAILURE);
        }

    // deroute le signal SIGUSR1 pour recuperer les PID client et les stockers dans un tableau
    for (int i = 0; i < nbJoueurMax; i++) {
        struct sigaction action;
        action.sa_sigaction = derouteServeur;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_SIGINFO;
        sigaction(SIGUSR1, &action, NULL);
        pause();
        pidJoueur[i] = signalPid;
        signalPid = -1;
    }

    }
    return 0;
}


/*
    test.mtype = 10;
    test.content[0].pidJoueur = 5000;
    test.content[0].score = 12;
    test.content[1].pidJoueur = 5001;
    test.content[1].score = 16;

    printf("mtype : %ld\n", test.mtype);
    printf("scorej1 : %d\n", test.content[0].score);
    printf("pidj1 : %d\n", test.content[0].pidJoueur);
    printf("scorej2 : %d\n", test.content[1].score);
    printf("pifj2 : %d\n", test.content[1].pidJoueur);
    printf("test : %d\n", test.content[2].pidJoueur);
*/
