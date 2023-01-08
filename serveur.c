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
void detruireQueues();
void envoyerPartition(int msqid, int partition);
void envoyerLettrePrePartie();
void * lireScoreRoutine();
void * envoyerScoreRoutine();
/*******************************************************************************************************/
//global var
int nbJoueurs, tempsDebutPartieS, nbJoueurMax;
joueurMetaData_t metaJoueurs[4];//les données sur tous les joueurs
int partitionChoisie;
int finPartie;//pour annonceer au thread envoyer score d'arrêter
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
    pthread_t threadLireScore, threadEnvoiScore;

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

    while(tempsDebutPartieS > 0){
    }

    pthread_create(&threadLireScore, NULL, lireScoreRoutine, NULL);
    pthread_create(&threadEnvoiScore, NULL, envoyerScoreRoutine, NULL);
    pthread_join(threadLireScore, NULL);
    pthread_join(threadEnvoiScore, NULL);
    //si on arrive ici c'est que tous les joueurs ont finis leurs partie
    printf("Voici les score finaux : \n\n");
    for(int i =0; i< nbJoueurs; i++){
        printf("Joueur n°%d, pid : %d, score : %d\n",i, metaJoueurs[i].pid, metaJoueurs[i].score);
    }
    sleep(10);
    detruireQueues();

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
            if(--tempsDebutPartieS >= 0){
                alarm(1);
                envoyerLettrePrePartie();
            }
            //toutes les 5 secondes
            if((tempsDebutPartieS % 5) == 0){
                printf("[serveur] la partie commencera dans %d secondes\n", tempsDebutPartieS);
                printf("[serveur] %d joueurs présents dans la partie \n", nbJoueurs);
            }
            if(tempsDebutPartieS == 0 && nbJoueurs < 2){
                printf("[serveur] pas assez de joueurs, fin du programme\n");
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
        kill(sa->si_pid, SIGUSR1);
        creerQueue(sa->si_pid);
        printf("on envoie partition à %d \n",metaJoueurs[nbJoueurs - 1].pid);
        //usleep(500);
        envoyerPartition(nbJoueurs - 1 , partitionChoisie);
        break;
    default:
        break;
    }
}

void CHECK(int code, char * toprint){
    if(code < 0){
        printf("%s \n", toprint);
        //exit(EXIT_FAILURE);
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
    CHECK(metaJoueurs[nbJoueurs].cle = ftok(FIC_BAL, pid), "problème ftok\n");
    CHECK(metaJoueurs[nbJoueurs].msqid = msgget(metaJoueurs[nbJoueurs].cle, IPC_CREAT | IPC_EXCL | 0666), "problème msgget");
    nbJoueurs++;
    printf("fin creationQueue\n");
    return;
}

/**
 * @brief detruit les ressource allouées à une BAL
 * 
 * @param pid le pid du client associé à cette BAL
 */
void detruireQueues(){
    int i;

    for(i=nbJoueurs - 1 ; i >= 0; i--){
        CHECK(msgctl(metaJoueurs[i].msqid, IPC_RMID, NULL), "erreur suppression queues\n");
    }
    printf("toutes les files sont supprimées \n");
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
    printf("clé : %d\n", metaJoueurs[playerNb].cle);
    msgsnd(metaJoueurs[playerNb].msqid, &msg, sizeof(msg), 0);
    printf("Errno : %d\n", errno);
    printf("[serveur] partition n°%d envoyée au client n°%d\n", partition, metaJoueurs[playerNb].pid);
}

/**
 * @brief lis les score de tous les joueurs en boucle
 * quand les 4 score ont été mo
 * 
 */
void * lireScoreRoutine(){
    int nbClient = 0, nbFinPartition=0, i;
    score1J_t score;

    //on lis jusqu'a ce que tous les jours aient terminés leur partition
    do{
        for(nbClient=0; nbClient < nbJoueurs; nbClient++){
            msgrcv(metaJoueurs[nbClient].msqid, &score, sizeof(score), MTYPE_MAJSCORE_1J, IPC_NOWAIT);
            msgrcv(metaJoueurs[nbClient].msqid, &score, sizeof(score), MTYPE_FIN_PARTITION, IPC_NOWAIT);
            metaJoueurs[nbClient].score = score.content.score;
            printf("score joueur n° %d Receptionned : %d \n", metaJoueurs[nbClient].pid, metaJoueurs[nbClient].score);
            if(score.mtype == MTYPE_FIN_PARTITION){
                nbFinPartition ++;
                printf("joueurs N°%d, a terminé sa partition\n", score.content.pidJoueur);
            }
            sleep(1);
        }    
    }while(nbFinPartition < nbJoueurs);
    finPartie = 1;
    printf("tous les joueurs on finit leur partition\n");
    pthread_exit;
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
    message.content.secondeRestant = tempsDebutPartieS;

    for(i=0; i < nbJoueurs; i++){
        message.content.pidJoueur[i] = metaJoueurs[i].pid;
    }
    for(i=0; i < nbJoueurs; i++){
        CHECK(msgsnd(metaJoueurs[i].msqid, &message, sizeof(preGameLetter_t), 0 ), "problème msgsnd");
    }
    //printf("prepartie envoyé à tous les joueurs\n");
    return;
}

/**
 * @brief enovie à tous les joueurs les scores de leurs adversaires puis dors 1 seconde
 * 
 * @return void* 
 */
void * envoyerScoreRoutine(){
    scoreAll_t scores;
    int i;

    while(!finPartie){
        //on contruit le message
        scores.mtype = MTYPE_MAJSCORE_ALL;
        for(i=0; i < nbJoueurs; i++){
            scores.content[i].pidJoueur = metaJoueurs[i].pid;
            scores.content[i].score = metaJoueurs[i].score;
        }
        //on envoie à tous les joueurs
        for(i=0; i < nbJoueurs; i++){
            msgsnd(metaJoueurs[i].msqid, &scores, sizeof(scores), 0);
            printf("scoresAll j%d enovyé\n", i);
        }
        sleep(1);
    }
    //on contruit le message de fin de partie
    scores.mtype = MTYPE_FIN_PARTIE;
    for(i=0; i < nbJoueurs; i++){
        scores.content[i].pidJoueur = metaJoueurs[i].pid;
        scores.content[i].score = metaJoueurs[i].score;
    }
    //on envoie à tous les joueurs
    printf("fin partie enovyé\n");
    for(i=0; i < nbJoueurs; i++) msgsnd(metaJoueurs[i].msqid, &scores, sizeof(scores), 0);

    pthread_exit;
}