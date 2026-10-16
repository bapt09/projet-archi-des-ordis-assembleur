#ifndef ASSEMBLEUR_H
#define ASSEMBLEUR_H

#include "commun.h"

// Retourne 0 si succès, -1 si échec
int assembler(char *nom_fichier, Instruction *programme, int *nb_instructions);

// Fonction utilitaire pour afficher le résultat
void afficher_prog(Instruction *programme, int nb_instructions);

#endif
