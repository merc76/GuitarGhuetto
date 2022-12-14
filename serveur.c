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
#include <unistd.h>
#include <stdlib.h>
#include "GuitarGhetto.h"

//prototypes
void serveurDeroute(int, siginfo_t*, void*);
void CHECK(int code, char * toprint);

//global var
int nbJoueurs = 0, tempsDebutPartieS, nbJoueurMax;
pid_t pidJoueur[MAX_JOUEUR_DEFAUT];


/**
 * @brief 
 * 
 * @param argc nombre d'argument
 * @param argv les arguments
 * @return int 
 */
int main(int argc, char * argv[]){
    scoreAll_t test;
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = serveurDeroute;

    //on apllique les valeurs par défauts
    tempsDebutPartieS = TEMPS_DEBUT_PARTIE_DEFAUT;
    nbJoueurMax = MAX_JOUEUR_DEFAUT;

// si le nombre d'argument est egal a 3 alors on recupere les valeurs
    if(argc == 3){
        nbJoueurMax = atoi(argv[1]);
        tempsDebutPartieS = atoi(argv[2]);
        // si le nombre de joueur est inferieur a 2 ou superieur a 4 alors on affiche un message d'erreur
        if(nbJoueurMax > 4 || nbJoueurMax < 2){
            printf("[serveur] nombre de joueur incorrect\n");
            printf(" 2 <= joueurMax < 4 (inclues)\n");
            exit(EXIT_FAILURE);
        }
        // si le temps de debut de partie est inferieur a 10 ou superieur a 60 alors on affiche un message d'erreur
        if(tempsDebutPartieS > 280 || tempsDebutPartieS < 60){
            printf("[serveur] temps avant début de partie inccorect\n");
            printf(" 60sec < temps début < 280sec (inclues)\n");
            exit(EXIT_FAILURE);
        }
    }

    //Clear les signaux
    CHECK(sigemptyset(&sa.sa_mask), "[serveur] Erreur sigemptyset\n");

    //Création du masque de signaux
    CHECK(sigprocmask(SIG_SETMASK, &sa.sa_mask, NULL), "[serveur] Erreur sigprocmask\n");

    CHECK(sigaction(SIGUSR1, &sa, NULL), "[serveur] Erreur client ne peut pas ajouter SIGUSR1");
    CHECK(sigaction(SIGALRM, &sa, NULL), "[serveur] Erreur client ne peut pas ajouter SIGALRM");

    printf("[serveur] La partie est créer, vous pouvez rejoindre avec le code : %d.\n", getpid());
    printf("[serveur] la partie commencera dans %d secondes\n", tempsDebutPartieS);
    alarm(1);



    while(1){}

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


/**
 * @brief Modifie le comportement des signaux reçus
 * 
 * @param sig le signal reçu
 * @param sa struct siginfo reçu
 * @param context 
 */
void serveurDeroute(int sig, siginfo_t *sa, void *context){
    switch (sig) {
    case SIGALRM:
        //si je me suis envoyé une alarme (toutes les secondes)
        if(sa->si_pid = getpid()){
            //on décremente le temps restant et on prépare le prochain envoie
            if(--tempsDebutPartieS > 1) alarm(1);
            //toutes les 5 secondes
            if((tempsDebutPartieS % 5) == 0){
                printf("[serveur] la partie commencera dans %d secondes\n", tempsDebutPartieS);
                printf("[serveur] %d joueurs présents dans la partie \n", nbJoueurs);
            } 
        }
        break;

    case SIGUSR1:
        printf("[serveur] le joueur n°%d essaye de se connecter\n", sa->si_pid);
        //temps restant insuffisant ou nbjoueurs max atteint
        if(tempsDebutPartieS < 10 || nbJoueurs == nbJoueurMax){
            printf("[serveur] le joueur n°%d se voit refuser la connexion\n", sa->si_pid);
            kill(sa->si_pid, SIGUSR2);
        }       
        else{
            printf("[serveur] le joueur n°%d se voit approuver la connexion\n", sa->si_pid);
            kill(sa->si_pid, SIGUSR1);
            pidJoueur[nbJoueurs++] = sa->si_pid;
        }
        break;
    default:
        break;
    }
}

void CHECK(int code, char * toprint){
    if(code < 0){
        printf("%s \n", toprint);
        exit(EXIT_FAILURE);
    }
}