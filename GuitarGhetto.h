/*********************************************************************************************************/
//defines
#define MAX_JOUEUR_DEFAUT 4
#define TEMPS_DEBUT_PARTIE_DEFAUT 120
#define FIC_BAL "./BALREP"
#define TAILLE_PARTITION 250

#define MTYPE_ENVOI_PARTITION 11
#define MTYPE_DEBUT_PARTIE 12
#define MTYPE_MAJSCORE_1J 1
#define MTYPE_MAJSCORE_ALL 13
#define MTYPE_FIN_PARTITION 2
#define MTYPE_FIN_PARTIE 14
#define MTYPE_PRE_PARTIE 15
/*********************************************************************************************************/
//strcuture de message

//partition
typedef struct partitionLettre{
    long mtype;
    char content[TAILLE_PARTITION];
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
typedef contentPreGame{
    int secondeRestant;
    int nbJoueur;
    int pidJoueur[4];
}

//temps restant et nb de joueurs présent
typedef preGameLetter{
    long mtype;
    contentPreGame_t content;
}