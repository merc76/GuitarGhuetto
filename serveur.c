/********************************************************************************************************
 * @file serveur.c
 * @author MMR, FTA
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
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "time.h"
#include "GuitarGhetto.h"
/*******************************************************************************************************/
//struct META
typedef struct joueurMetaData{
    pid_t pid;
    int score;
    key_t cle;
    int msqid;
    //int pseudo
}joueurMetaData_t;
/*******************************************************************************************************/
//prototypes
void serveurDeroute(int, siginfo_t*, void*);
void CHECK(int code, char * toprint);
void creerQueue(pid_t pid);
void detruireQueues(pid_t pid);
void envoyerPartition(int msqid, int partition);
void envoyerLettrePrePartie();
void lireScoreRoutine();
/*******************************************************************************************************/
//global var
int nbJoueurs, tempsDebutPartieS, nbJoueurMax;
joueurMetaData_t metaJoueurs[4];
int partitionChoisie;
/*******************************************************************************************************/
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
    nbJoueurs = 0;

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
    //ajout des deroute
    CHECK(sigaction(SIGUSR1, &sa, NULL), "[serveur] Erreur client ne peut pas ajouter SIGUSR1");
    CHECK(sigaction(SIGALRM, &sa, NULL), "[serveur] Erreur client ne peut pas ajouter SIGALRM");

    srand(time(NULL));
    partitionChoisie = rand() % NB_PARTITION;
    printf("[serveur] La partie est créer, vous pouvez rejoindre avec le code : %d.\n", getpid());
    printf("[serveur] la partie commencera dans %d secondes\n", tempsDebutPartieS);
    alarm(1);

    while(1){
        
    }

    //TODO : supprimer les BAL

    return 0;
}

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
            if(--tempsDebutPartieS > 1){
                alarm(1);
                //envoyerLettrePrePartie();
            }
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
            return;
        }       
        printf("[serveur] le joueur n°%d se voit approuver la connexion\n", sa->si_pid);
        //detruireQueues(sa->si_pid);
        kill(sa->si_pid, SIGUSR1);
        creerQueue(sa->si_pid);
        printf("on envoie partition à %d \n",metaJoueurs[nbJoueurs - 1].pid);
        envoyerPartition(nbJoueurs - 1 , partitionChoisie);
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

/**
 * @brief Créer une boite à lettre avec un token se basant sur un fichier commun
 *      et pour entier le pid du client avec qui on communique
 * 
 * @param pid le pid du client avec qui on va communiquer
 */
void creerQueue(pid_t pid){
    printf("creationQueue\n");
    metaJoueurs[nbJoueurs].pid = pid;
    metaJoueurs[nbJoueurs].cle = ftok(FIC_BAL, pid);
    metaJoueurs[nbJoueurs].msqid = msgget(metaJoueurs[nbJoueurs].cle, IPC_CREAT | IPC_EXCL | 0666);
    nbJoueurs++;
    printf("fin creationQueue\n");
    return;
}

/**
 * @brief detruit les ressource allouées à une BAL
 * 
 * @param pid le pid du client associé à cette BAL
 */
void detruireQueues(pid_t pid){
    int i;
    char scoreS[12];

    for(i=0; i<nbJoueurs; i++){
        sprintf(scoreS, "%d", metaJoueurs[i].score);
        CHECK(msgctl(metaJoueurs[i].msqid, IPC_RMID, NULL), strcat("erreur ne peut pas supprimer la queue n°", scoreS));
    }
}

/**
 * @brief envoie la partition choisie au client dont le pid est pid
 * 
 * @param pid 
 */
void envoyerPartition(int playerNb, int partition){
    int client, i;
    partitionLettre_t msg;

    msg.mtype = MTYPE_ENVOI_PARTITION;
    msg.content = partition;
    msg.content = partition;
    printf("%d\n", metaJoueurs[playerNb].cle);
    msgsnd(metaJoueurs[playerNb].msqid, &msg, sizeof(msg), 0);
    printf("Errno : %d\n", errno);
    printf("[serveur] partition n°%d envoyée au client n°%d\n", partition, metaJoueurs[playerNb].pid);
}

/**
 * @brief lis les score de tous les joueurs en boucle
 * quand les 4 score ont été mo
 * 
 */
void LireScoreRoutine(){
    int nbClient;
    score1J_t score;

    for(nbClient = 0; nbClient < nbJoueurs; nbClient = ((nbClient + 1) % nbJoueurs)){
        msgrcv(metaJoueurs[nbClient].msqid, &score, sizeof(score1J_t), MTYPE_MAJSCORE_1J, IPC_NOWAIT | 0666);
        metaJoueurs[nbClient].score = score.content.score;
    }
}

/**
 * @brief envoie les données de début de partie à tous les joueurs
 * 
 */
void envoyerLettrePrePartie(){
    int i;
    preGameLetter_t message;

    message.mtype = MTYPE_PRE_PARTIE;
    message.content.nbJoueur = nbJoueurs;

    for(i=0; i < nbJoueurs; i++){
        message.content.pidJoueur[i] = metaJoueurs[i].pid;
    }
    for(i=0; i < nbJoueurs; i++){
        msgsnd(metaJoueurs[i].msqid, &message, sizeof(preGameLetter_t), IPC_NOWAIT );
    }
    return;
}
