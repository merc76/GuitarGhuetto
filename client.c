/********************************************************************************************************
 * @file client.c
 * @author MMR, FTAFFIN
 * @brief 
 * @version 1.0
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
#define TEMPS_PAR_NOTE 500 //en millisecondes doit être multiple de (100)
#define DELTA_T_PERFECT 250 //ms
#define DELTA_T_GOOD 300 //ms
#define DELTA_T_BAD 400 //ms

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
void attribuerNote(int tempsReaction, char cle);
void printScoreBoard();
void flushgetch();
/*******************************************************************************************************/
// variables globales

char partitionEnCours[TAILLE_PARTITION];
char scoreLastNote[30]; //le commentaire sur la dernière note (GOOD, PERFECT, BAD ...)
int monScore;
int flagPartition;//ou en est on de la partition
int msqidServeur;//le msqid pour échanger avec le serveur
int nbJoueur;
int finPartie;
preGameLetter_t preGame;//une variable pour lire les message d'avant début de partie
pid_t pidServeur;
contentScore1J_t scoreTousJoueurs[4];// les pids et scores des joueurs 
/*******************************************************************************************************/

void printAllScore(){
    printf("mon Score : %d\n", monScore);
    for(int i =0; i < nbJoueur; i++){
        if(scoreTousJoueurs[i].pidJoueur != 0) printf("score de %d : %d\n", scoreTousJoueurs[i].pidJoueur, scoreTousJoueurs[i].score);
    }
}

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
    struct timespec debutNote, finNote;
    float tempsReaction;

    //initialisation
    finPartie = 0;
    monScore = 0;
    scoreLastNote[0] = '\0';

    //TODO : ajouter gestion de pseudo
    //TODO : fix stack smashing at ending 
    //TODO ; ajouter des fichier logs et système de classement pas point et par pseudo
    //TODO : replace alarm with "sleep" to avoid interferences between the signal and the
    //message queues that also uses signals 

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
    //printf("Errno : %d\n", errno);
    msgrcv(msqidServeur, &partitionAJouer, sizeof(partitionAJouer), MTYPE_ENVOI_PARTITION, 0);
    //printf("Errno : %d\n", errno);
    strcpy(partitionEnCours, partitionListe[partitionAJouer.content]);
    //printf("client n°%d, la partition n°%s à été chargée\n", getpid(), partitionEnCours);

    do{
        msgrcv(msqidServeur, &preGame, sizeof(preGame), MTYPE_PRE_PARTIE, 0);
        printf("client n°%d, temps avant début de partie : %d\n", getpid(), preGame.content.secondeRestant);
        printf("client n°%d, joueurs dans le hub : %d\n", getpid(), preGame.content.nbJoueur);
    }while(preGame.content.secondeRestant > 0);

    if(preGame.content.nbJoueur < 2){
      printf("[client] %d : pas assez de joueurs, fin du programme\n", getpid());
      finPartie = 1;
      return 0;
    }

    nbJoueur = preGame.content.nbJoueur;

    pthread_create(&threadEnvoieScore, NULL, routineEnvoieScore, NULL);
    pthread_create(&threadReceptionScore, NULL, routineReceptionScore, NULL);
    //fin de préparation début de partie
    pthread_create(&threadAffichage, NULL, routineAffichage, NULL);

    pthread_join(threadEnvoieScore, NULL);
    pthread_join(threadReceptionScore, NULL);
    pthread_join(threadAffichage, NULL);
    
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

    while(flagPartition < TAILLE_PARTITION){
        scoreMsg.mtype = MTYPE_MAJSCORE_1J;
        scoreMsg.content.pidJoueur = getpid();
        scoreMsg.content.score = monScore;
        msgsnd(msqidServeur, &scoreMsg, sizeof(scoreMsg), 0);
        //printf("[routine score envoie] %d : score enoyé au serveur : %d\n", getpid(), scoreMsg.content.score );
        sleep(3);
    }
    scoreMsg.mtype = MTYPE_FIN_PARTITION;
    scoreMsg.content.pidJoueur = getpid();
    scoreMsg.content.score = monScore;
    msgsnd(msqidServeur, &scoreMsg, sizeof(scoreMsg), 0);
    pthread_exit;
}

/**
 * @brief permet la reception des scores des autres joueurs en boucles
 * 
 * @param noth nothing
 */
void *routineReceptionScore(void * noth){
    scoreAll_t scores;
    int i;

    //TODO: fix scoreboard bug
    //TODO : améliorer le chronométrage

    do{
        //printf("[routine score reception] %d : attente score\n", getpid());
        msgrcv(msqidServeur, &scores, sizeof(scores), MTYPE_MAJSCORE_ALL, IPC_NOWAIT);
        msgrcv(msqidServeur, &scores, sizeof(scores), MTYPE_FIN_PARTIE, IPC_NOWAIT);
        for(i =0; i < nbJoueur; i++){
            if(scores.content[i].pidJoueur != getpid()){
                scoreTousJoueurs[i].pidJoueur = scores.content[i].pidJoueur;
                scoreTousJoueurs[i].score = scores.content[i].score;
            }
        }
        //printf("[routine score reception] %d : score reçu depuis le serveur\n", getpid());
        //printAllScore();
        sleep(1);
    }while(scores.mtype != MTYPE_FIN_PARTIE);
    finPartie = 1;
    printf("La partie est terminée voici les scores \n");
    printAllScore();
    pthread_exit;
}

/**
 * @brief recupere un temps en milliseconde et attribut au joueur un score
 * 
 * @param tempsReaction le temps en milliseconde
 * @param cle la clé pressée
 */
void attribuerNote(int tempsReaction, char cle){
    if(cle != partitionEnCours[flagPartition]){
        monScore += SCORE_WORSE;
        strcpy(scoreLastNote, "WRONG  ");
        return;
    }
    if(tempsReaction <= DELTA_T_PERFECT){
        monScore += SCORE_PERFECT;
        strcpy(scoreLastNote, "PERFECT");
        return;
    }
    if(tempsReaction <= DELTA_T_GOOD ){
        monScore += SCORE_GOOD;
        strcpy(scoreLastNote, "GOOD   ");
        return;
    } 
    if(tempsReaction <= DELTA_T_BAD){
        monScore += SCORE_BAD;
        strcpy(scoreLastNote, "BAD    ");
        return;
    }
    else{
        monScore += SCORE_WORSE;
        strcpy(scoreLastNote, "MISS   ");
        return;
    }
}


/**
 * @brief affiche une interface plus agréable pour le joueur
 * 
 * @param noth 
 * @return void* 
 */
void *routineAffichage(void * noth){

    char saisies[sizeof partitionEnCours]; // tableau pour stocker les touches saisies par l'utilisateur 
    char keyPressed;
    float tempsReaction;
    struct timespec debutNote, finNote, aRetirer;
    int monScore = 0; // monScore du joueur  
    char charToPrint =' ';
    int i;  

    flagPartition = 0;

    initscr(); // initialise l'interface ncursed  
    cbreak(); // désactive l'affichage des caractères saisis par l'utilisateur
    //halfdelay(TEMPS_PAR_NOTE / 100);
    curs_set(0); // cache le curseur
    //on clean  
    erase();

    while(flagPartition < TAILLE_PARTITION){
        for(i = 0; (i < 10 && (flagPartition + i) < TAILLE_PARTITION); i++){
            if(partitionEnCours[flagPartition + i] != '-'){
                charToPrint = partitionEnCours[flagPartition + i];
            }
            else charToPrint = ' ';
            mvprintw(LINES - 1 - i, 30, "%c ", charToPrint);
        }
        refresh();
        //on demande la sasie du caractère
        if(partitionEnCours[flagPartition] != '-'){
            timespec_get(&debutNote, TIME_UTC);
            timeout(DELTA_T_BAD - 20);
            keyPressed = getch();
            //scanw("%c", &keyPressed);
            timespec_get(&finNote, TIME_UTC);
            tempsReaction = abs((float)(finNote.tv_nsec - debutNote.tv_nsec) / 1000000) ;
            attribuerNote(tempsReaction, keyPressed);
            mvprintw(5, 0,"cle presse : %c, temps reaction %.2f, flag : %d     ", keyPressed, tempsReaction, flagPartition);
        }
        printScoreBoard();
        mvprintw(LINES - 1, 5, "%s", scoreLastNote);
        mvprintw(2, 5, "completed : %f%", ((float)flagPartition / TAILLE_PARTITION)* 100); 
        timespec_get(&aRetirer, TIME_UTC);
        usleep(TEMPS_PAR_NOTE * 1000);
        flagPartition++;
        //flushgetch();
        keyPressed = ' ';
        //erase();
        mvprintw(LINES - 1, 26, "%s", "---"); // ajoute une ligne de tiret en bas de l'écran 
        mvprintw(LINES - 1, 32, "%s", "---"); // ajoute une ligne de tiret en bas de l'écran 
        refresh();

        //TODO : sleep the right amount of time instaed of TEMPS_PAR_NOTE here
        //usleep((350 - (debutNote.tv_nsec - aRetirer.tv_nsec)) * 1000);
    }

    endwin(); // termine l'interface ncursed  
    pthread_exit;
}

/**
 * @brief imprime le classement sur l'écran
 * 
 */
void printScoreBoard(){
    int i;

    mvprintw(1, COLS - 30, "SCOREBOARD");
    mvprintw(2, COLS - 30, "mon score : %5d", monScore);
    for(i = 0; i < nbJoueur; i++){
        if(scoreTousJoueurs[i].pidJoueur && scoreTousJoueurs[i].pidJoueur != getpid()){
            mvprintw(3+i, COLS - 30, " %d : %5d", scoreTousJoueurs[i].pidJoueur, scoreTousJoueurs[i].score);
        }
    }
    return;
}

/**
 * @brief on nettoie la sortie si on a plusieurs char dans le buffer
 * 
 */
void flushgetch() {
    int c;
    do{
        c = getch();
    }
    while ((c != '\n') && (c!= ERR));

    return; 
}