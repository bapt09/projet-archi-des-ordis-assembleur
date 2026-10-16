#ifndef COMMUN_H
#define COMMUN_H

//Couleur pour la console, je suis allé chercher ce code sur internet, je connaissais pas ces commandes
#define rouge     "\033[1;31m"
#define vert   "\033[1;32m"
#define jaune  "\033[1;33m"
#define bleu    "\033[1;37m"
#define normal   "\033[0m"

//structure des instructions

typedef struct {
    char label[20];   // Nom du label (ex: "boucle")
    char opcode[10];  // Nom de la commande (ex: "ADD", "OUT")
    int nb_args;      // Nombre d'arguments
    char args[4][20]; // Les arguments (ex: "R1", "#5")
} Instruction;

#endif
