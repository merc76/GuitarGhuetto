//pour eviter d'utiliser 2 fois le même header durant la compilation
#ifndef GUITARGHETTO_H
#define GUITARGHETTO_H
/********************************************************************************************************
 * @file GuitarGhetto.h
 * @author MMR, FTAFFIN
 * @brief 
 * @version 0.1
 * @date 2022-11-29
 * 
 * @copyright Copyright (c) 2022
 * 
 *********************************************************************************************************/
/*********************************************************************************************************/
//defines
#define MAX_JOUEUR_DEFAUT 4
#define TEMPS_DEBUT_PARTIE_DEFAUT 120
#define FIC_BAL "./BALREP"
#define TAILLE_PARTITION 250
#define NB_PARTITION 3

#define MTYPE_ENVOI_PARTITION 11
#define MTYPE_DEBUT_PARTIE 12
#define MTYPE_MAJSCORE_1J 1
#define MTYPE_MAJSCORE_ALL 13
#define MTYPE_FIN_PARTITION 2
#define MTYPE_FIN_PARTIE 14
#define MTYPE_PRE_PARTIE 15

/*********************************************************************************************************/
//variables globales

const char partitionListe[NB_PARTITION][TAILLE_PARTITION] = {
    "-----------------------Z-Z---S-Q--D-S-----Z-Z-D-S----------ZQS-SZ--QS-----------------------",
    "-----------------------Z-Z---S-Q--D-S-----Z-Z-D-S----------ZQS-SZ--QS-----------------------",
    "-----------------------Z-Z---S-Q--D-S-----Z-Z-D-S----------ZQS-SZ--QS-----------------------"
};
/*********************************************************************************************************/
//strcutures de messages

//partition
typedef struct partitionLettre{
    long mtype;
    int content;
}partitionLettre_t;

//score un joueur
typedef struct contentScore1J{
    int pidJoueur;
    int score;
}contentScore1J_t;

//MAJ score un joueur
typedef struct score1J{
    long mtype;
    contentScore1J_t content;
}score1J_t;

//MAJ score tous les joueurs
typedef struct scoreAll{
    long mtype;
    contentScore1J_t content[4];
}scoreAll_t;

//temps restant et nb de joueurs présent (contenu)
typedef struct contentPreGame{
    int secondeRestant;
    int nbJoueur;
    int pidJoueur[4];
}contentPreGame_t;
//temps restant et nb de joueurs présent
typedef struct preGameLetter{
    long mtype;
    contentPreGame_t content;
}preGameLetter_t;

#endif