/********************************************************************************************************
 * @file client.c
 * @author MMR, FTAFFIN
 * @brief 
 * @version 0.1
 * @date 2022-11-29
 * 
 * @copyright Copyright (c) 2022
 * 
 *********************************************************************************************************/
#include <stdio.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <ncurses.h>
#include "time.h"
#include "GuitarGhetto.h"
/*******************************************************************************************************/
//constantes

//TODO: pour une prochaine versions on pourrait avec un temps différents entre chaque notes
//TODO temps qu'on pourrait observer dans les partitions
#define TEMPS_PAR_NOTE 350 //en millisecondes
#define DELTA_T_PERFECT 100 //ms
#define DELTA_T_GOOD 200 //ms
#define DELTA_T_BAD 300 //msg

#define SCORE_PERFECT 500
#define SCORE_GOOD 200
#define SCORE_BAD 50
#define SCORE_WORSE -10

#define SCREEN_WIDTH 200
#define SCREEN_HEIGHT 100

/*******************************************************************************************************/
//empreintes fonctions

void CHECK(int code, char * toprint);
void clientDeroute(int, siginfo_t*, void*);
void *routineAffichage(void *);
void *routineEnvoieScore(void *);
void *routineReceptionScore(void *);

/*******************************************************************************************************/
// variables globales

char partitionEnCours[TAILLE_PARTITION];
int monScore;
int flagPartition;
int msqidServeur;
int nbJoueur;
preGameLetter_t preGame;
pid_t pidServeur;
contentScore1J_t scoreTousJoueurs[4];
/*******************************************************************************************************/

/**
 * @brief 
 * 
 * @return int 
 */
int main(){
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = clientDeroute;
    key_t cleServeur;
    partitionLettre_t partitionAJouer;
    char tmp[20], keyPressed;
    pthread_t threadAffichage, threadEnvoieScore, threadReceptionScore;
    time_t debutNote, tempsReaction, finNote;

    printf("bienvenue à vous !!\n merci d'entrer votre code de partie !\n");
    fgets(tmp, 7, stdin);
    while(tmp[strlen(tmp)] == ' ' || tmp[strlen(tmp)] == '\n') tmp[strlen(tmp) - 1] = '\0';
    pidServeur = atoi(tmp);
    //printf("atoi : %d", pidServeur);

    //Clear les signaux
    CHECK(sigemptyset(&sa.sa_mask), "[Child] Erreur sigemptyset\n");

    //Création du masque de signaux
    CHECK(sigprocmask(SIG_SETMASK, &sa.sa_mask, NULL), "[Child] Erreur sigprocmask\n");

    CHECK(sigaction(SIGUSR1, &sa, NULL), "Erreur client ne peut pas ajouter SIGUSR1");
    CHECK(sigaction(SIGUSR2, &sa, NULL), "Erreur client ne peut pas ajouter SIGUSR2");
    CHECK(sigaction(SIGALRM, &sa, NULL), "Erreur client ne peut pas ajouter SIGALRM");

    //préparation avant la fin de partie
    kill(pidServeur, SIGUSR1);
    sleep(2);
    cleServeur = ftok(FIC_BAL, getpid());
    msqidServeur = msgget(cleServeur, 0666);
    //printf("%d\n", cleServeur);
    //printf("%d\n", msqidServeur);
    printf("Errno : %d\n", errno);
    msgrcv(msqidServeur, &partitionAJouer, sizeof(partitionAJouer), MTYPE_ENVOI_PARTITION, 0);
    printf("Errno : %d\n", errno);
    strcpy(partitionEnCours, partitionListe[partitionAJouer.content]);
    printf("client n°%d, la partition n°%d à été chargée\n", getpid(), partitionAJouer.content);

    do{
        msgrcv(msqidServeur, &preGame, sizeof(preGame), MTYPE_PRE_PARTIE | MTYPE_DEBUT_PARTIE, 0);
        printf("client n°%d, temps avant début de partie : %d\n", getpid(), preGame.content.secondeRestant);
        printf("client n°%d, joueurs dans le hub : %d\n", getpid(), preGame.content.nbJoueur);
    }while(preGame.content.secondeRestant > 0);

    if(preGame.content.nbJoueur < 2){
      printf("[client] %d : pas assez de joueurs, fin du programme\n", getpid());
    }

    nbJoueur = preGame.content.nbJoueur;

    pthread_create(&threadEnvoieScore, NULL, routineEnvoieScore, NULL);
    pthread_create(&threadReceptionScore, NULL, routineReceptionScore, NULL);
    //fin de préparation début de partie

    flagPartition = 0;
    //partie en cours
    pthread_create(&threadAffichage, NULL, routineAffichage, NULL);

    //le thread rejoins lorsque la partie est terminée
    pthread_join(threadReceptionScore, NULL);
    pthread_join(threadAffichage, NULL);
    pthread_join(threadEnvoieScore, NULL);
    

    return 0;
}

/**
 * @brief permet la reception des signaux et de leurs affecter une réponse adéquat 
 * 
 * @param sig le signal reçu
 * @param sa les informations relative au signal reçu
 * @param context 
 */
void clientDeroute(int sig, siginfo_t *sa, void *context){
    switch (sig) {
    case SIGALRM:
        break;

    case SIGUSR1:
        printf("le serveur à accepté votre connexion\n");
        break;

    case SIGUSR2:
        printf("le serveur à rejeté votre demande de connexion\n");
        exit(EXIT_FAILURE);
        break;
    default:
        break;
    }
}

/**
 * @brief vérifie si le retour d'une fonction est un succès ou échec
 *      dans le cas d'un échec renvoie la phrase toprint
 * 
 * @param code le code de retour
 * @param toprint la phrase à affiché en cas d'échec
 */
void CHECK(int code, char * toprint){
    if(code < 0){
        printf("%s \n", toprint);
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief permet d'envoyer son score personnel toutes les x secondes
 * 
 * @param noth nothing
 */
void *routineEnvoieScore(void* noth){
    score1J_t scoreMsg;

    while(1){
        scoreMsg.mtype = MTYPE_MAJSCORE_1J;
        scoreMsg.content.pidJoueur = getpid();
        scoreMsg.content.score = monScore;
        msgsnd(msqidServeur, &scoreMsg, sizeof(scoreMsg), 0);
        printf("[routine score envoie] %d : score enoyé au serveur\n", getpid());
        sleep(3);
    }
}

/**
 * @brief permet la reception des scores des autres joueurs en boucles
 * 
 * @param noth nothing
 */
void *routineReceptionScore(void * noth){
    scoreAll_t scores;
    int i;

    do{
        msgrcv(msqidServeur, &scores, sizeof(scores), MTYPE_MAJSCORE_ALL | MTYPE_FIN_PARTIE, 0);
        for(i =0; i < nbJoueur; i ++){
            if(scores.content[i].pidJoueur != getpid()) scoreTousJoueurs[i] = scores.content[i];
        }
        printf("[routine score reception] %d : score reçu depuis le serveur\n", getpid());
    }while(scores.mtype != MTYPE_FIN_PARTIE);
    pthread_exit;
}

/**
 * @brief affiche une interface plus agréable pour le joueur
 * 
 * @param noth 
 * @return void* 
 */
void *routineAffichage(void * noth){

char saisies[sizeof partitionEnCours]; // tableau pour stocker les touches saisies par l'utilisateur 
 
  int monScore = 0; // monScore du joueur  
  int i;  
  
  initscr(); // initialise l'interface ncursed  
  noecho(); // désactive l'affichage des caractères saisis par l'utilisateur  
  curs_set(0); // cache le curseur  
  
  for (i = 0; i < strlen(partitionEnCours); i++) {  
    // affiche le monScore et la partition en cours à partir de la ligne du haut  
    mvprintw(0, 0, "monScore: %d", monScore);  
    mvprintw(1, 30, "%c", partitionEnCours[i]);  
    mvhline(LINES - 1, 0, '-', COLS); // ajoute une ligne de tiret en bas de l'écran  
    refresh(); // met à jour l'affichage  
  
    int j;  
    for (j = 1; j < LINES; j++) {  
      // déplace la lettre vers le bas de l'écran  
      mvprintw(j - 1, 30, " ");  
      mvprintw(j, 30, "%c", partitionEnCours[i]);  
      refresh(); // met à jour l'affichage  
      nanosleep((const struct timespec[]){{0, TEMPS_PAR_NOTE * 1000000L}}, NULL); // attend DELAY ms  
    }  
 
    // initialise la variable debutNote avec l'heure 
    // initialise la variable debutNote avec l'heure actuelle 
    time_t debutNote; 
    time(&debutNote); 
  
    // attend que l'utilisateur appuie sur une touche ou que le délai de 300 ms soit écoulé  
    timeout(TEMPS_PAR_NOTE); // définit un délai de 300 ms avant que getch() ne renvoie ERR  
    char c = getch();  
  
    // enregistre l'heure actuelle dans la variable finNote 
    time_t finNote; 
    time(&finNote); 
 
    // calcule le temps de réaction en utilisant la différence entre debutNote et finNote 
    double tempsReaction = difftime(debutNote, finNote); 
  
    // vérifie si la touche appuyée est la bonne  
    if (c == partitionEnCours[i]) {  
      // touche correcte, incrémente le monScore en fonction du délai  
      if (tempsReaction <= DELTA_T_PERFECT) {  
        monScore += SCORE_PERFECT;  
        mvprintw(LINES - 1, 0, "PERFECT!");  
      } else if (tempsReaction <= DELTA_T_GOOD) {
        monScore += SCORE_GOOD;  
        mvprintw(LINES - 1, 0, "GOOD!");  
      }  else if (tempsReaction <= DELTA_T_BAD) {  
        monScore += SCORE_BAD;  
        mvprintw(LINES - 1, 0, "BAD...");  
      }  else {  
        // pas de touche appuyée dans le délai de 300 ms 
        monScore += SCORE_WORSE;  
        mvprintw(LINES - 1, 0, "MISS...");  
      }}
  
    // met à jour l'affichage et attend avant de passer au caractère suivant  
    refresh();  
    nanosleep((const struct timespec[]){{0, TEMPS_PAR_NOTE * 1000000L}}, NULL); 
    }  
  
  endwin(); // termine l'interface ncursed  
    pthread_exit;
}


