#include <stdio.h>
#include <stdlib.h>
#include "commun.h"
#include "assembleur.h"
#include "codage.h"
#include "cpu.h"
//Bienvenue dans ce petit programme
//Il est divisé en deux parties distincts : l'assemblage/compilation et l'execution
//Ce fichier est le chef d'orchestre du programme, il récupère le fichier donné dans la console, puis l'envoie au fichier compilation
//Celui ci vérifie si tout les arguments sont correctes, puis il stock tout dans une structure Instruction, qui est envoyé au fichier codage.c
//C'est lui qui code le fichier hexa.txt en hexadécimal, ensuite le main appelle le fichier executer, qui réouvre hexa.txt pour executer les instructions
//Plus de détail dans la documentation...

int main(int argc, char *argv[]) {
    // verification que le fichier soit bien donné
    if (argc < 2) {
        printf(rouge "Erreur fichier manquant\n" normal);
        return 1;
    }

    // tableau instructions servant à stocker les lignes du programme assembleur donné en entré
    Instruction prog[16384];
    int n = 0;

    // l'assemblage
    printf(bleu " 1. COMPILATION \n" normal);

    // lecture et analyse fichier txt
    if (assembler(argv[1], prog, &n) != 0) {
        printf(rouge "Erreur assemblage. Stop.\n" normal);
        return 1;
    }
    //afficher_prog(prog, n); //Cette ligne est a rajouter si on veut avoir une idée de ce qu'il se passe, en fait elle est pas très utile mais bon...

    // creation fichier hexa.txt
    ecrire(prog, n);


    //Partie de l'execution
    printf(bleu "\n 2. EXECUTION \n" normal);

    init_machine();

    charger_programme("hexa.txt");

    executer();

    return 0;
}
