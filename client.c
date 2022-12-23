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
preGameLetter_t preGame;
pid_t pidServeur;
int flagPartition;
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
    int msqidServeur;
    partitionLettre_t partitionAJouer;
    char tmp[20], keyPressed;
    pthread_t threadAffichage, threadEnvoieScore, threadReceptionScore;
    time_t debutNote, tempsReaction, finNote;

    printf("bienvenue à vous !!\n merci d'entrer votre code de partie !\n");
    fgets(tmp, 6, stdin);
    
    pidServeur = atoi(tmp);

    //Clear les signaux
    CHECK(sigemptyset(&sa.sa_mask), "[Child] Erreur sigemptyset\n");

    //Création du masque de signaux
    CHECK(sigprocmask(SIG_SETMASK, &sa.sa_mask, NULL), "[Child] Erreur sigprocmask\n");

    CHECK(sigaction(SIGUSR1, &sa, NULL), "Erreur client ne peut pas ajouter SIGUSR1");
    CHECK(sigaction(SIGUSR2, &sa, NULL), "Erreur client ne peut pas ajouter SIGUSR2");
    CHECK(sigaction(SIGALRM, &sa, NULL), "Erreur client ne peut pas ajouter SIGALRM");

    //préparation avant la fin de partie
    kill(pidServeur, SIGUSR1);

    //on tampone pour laisser le temps au serveur de créer la BAL
    sleep(1);
    cleServeur = ftok(FIC_BAL, getpid());
    msqidServeur = msgget(cleServeur, 0666);
    msgrcv(msqidServeur, &partitionAJouer, sizeof(partitionLettre_t), MTYPE_ENVOI_PARTITION, 0664);
    strcpy(partitionEnCours, partitionListe[partitionAJouer.content]);
    printf("client n°%d, la partition n°%d à été chargée\n", getpid(), partitionAJouer.content);

    //on créer les 3 autres threads
    pthread_create(&threadAffichage, NULL, routineAffichage, NULL);
    pthread_create(&threadEnvoieScore, NULL, routineEnvoieScore, NULL);
    pthread_create(&threadReceptionScore, NULL, routineReceptionScore, NULL);

    do{
        msgrcv(msqidServeur, &preGame, sizeof(preGameLetter_t), MTYPE_PRE_PARTIE | MTYPE_DEBUT_PARTIE, 0664);
        alarm(1);
    }while(preGame.mtype != MTYPE_DEBUT_PARTIE);
    //fin de préparation début de partie

    initscr();
    //cbreak();
    //noecho();
    //nodelay(stdscr, TRUE);
    //scrollok(stdscr, TRUE);

    flagPartition = 0;
    //partie en cours
    while(partitionEnCours[flagPartition] != '\0'){
        time(&debutNote);
        keyPressed = getch();
        time(&finNote);
        printw("clé pressé : %c, temps réaction %f", keyPressed, difftime(debutNote, finNote));
        flagPartition++;
    }

    //préparation fin de partie
    pthread_join(threadAffichage, NULL);
    pthread_join(threadEnvoieScore, NULL);
    pthread_join(threadReceptionScore, NULL);
    while (1)
    {
    }
   
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
        printf("client n°%d, temps avant début de partie : %d\n", getpid(), preGame.content.secondeRestant);
        printf("client n°%d, joueurs dans le hub : %d\n", getpid(), preGame.content.nbJoueur);

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
    //TODO
    pthread_exit;
}

/**
 * @brief permet la reception des scores des autres joueurs en boucles
 * 
 * @param noth nothing
 */
void *routineReceptionScore(void * noth){
    //TODO
    pthread_exit;
}

/**
 * @brief affiche une interface plus agréable pour le joueur
 * 
 * @param noth 
 * @return void* 
 */
void *routineAffichage(void * noth){
// Initialiser la bibliothèque ncursed et créer une fenêtre de jeu
  initscr();
  cbreak();
  noecho();
  curs_set(FALSE);

  int x = SCREEN_WIDTH / 2;
  int y = 0;
  int lineY = SCREEN_HEIGHT - 1;

  char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int numLetters = sizeof(letters) - 1;

  // Sélectionner aléatoirement une lettre et une vitesse de chute
  char letter = letters[rand() % numLetters];
  int speed = 100;

  int score = 0;

  // Dessiner chaque tiret de la ligne de tirets
  for (int i = 0; i < SCREEN_WIDTH; i++) {
    mvaddch(lineY, i, '-');
  }

  // Boucle infinie pour mettre à jour la position de la lettre et de la ligne de tirets
  while (1) {
    // Afficher le score actuel de l'utilisateur à l'écran
    mvprintw(0, 0, "Score : %d", score);
    // Afficher la lettre à l'écran
    mvaddch(y, x, letter);
    refresh();

    // Attendre un peu avant de mettre à jour la position de la lettre et de la ligne de tirets
    napms(speed);

    // Effacer la lettre de l'écran
    mvaddch(y, x, ' ');

    // Mettre à jour la position de la lettre et de la ligne de tirets
    y++;
    lineY++;

    // Si la lettre atteint le haut du terminal, générer une nouvelle lettre aléatoirement
    if (y < 0) {
      y = 0;
      lineY = SCREEN_HEIGHT - 1;
      letter = letters[rand() % numLetters];
    }
    // Si la lettre atteint le bas de l'écran, lire l'entrée du clavier de l'utilisateur pendant 1 seconde
    if (y >= SCREEN_HEIGHT) {
      char ch = 0;
      timeout(1000);
      ch = getch();

      // Si la touche pressée correspond à la lettre affichée, augmenter le score de 2 et afficher "PERFECT" pendant 4 secondes
      if (ch == letter) {
        score += 2;
        mvprintw(2, 0, "PERFECT");
        refresh();
        napms(4000);
        mvprintw(2, 0, "       ");
      }
      // Si la touche pressée ne correspond pas à la lettre affichée, réinitialiser le score à 0
      else {
        score = 0;
      }

      // Générer une nouvelle lettre aléatoirement
      y = 0;
      lineY = SCREEN_HEIGHT - 1;
      letter = letters[rand() % numLetters];
    }
  }

  // Nettoyer la bibliothèque ncursed et fermer la fenêtre de jeu
  endwin();

    pthread_exit;
}

