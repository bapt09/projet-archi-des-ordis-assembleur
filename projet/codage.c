#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "commun.h"
#include "codage.h"

// Les opcodes pour savoir qui est qui
// On definit tout ca en dur pour pas se tromper
//Beaucoup de commentaires dans ce code...

// op maths
#define OP_OR  0
#define OP_AND 1
#define OP_XOR 2
#define OP_ADD 3
#define OP_SUB 4
#define OP_MUL 5
#define OP_DIV 6
#define OP_SHR 7

// Memoire
#define OP_LDB 12
#define OP_LDH 13
#define OP_LDW 14
#define OP_STB 15
#define OP_STH 16
#define OP_STW 17

// Sauts
#define OP_JMP 21
#define OP_JZS 22
#define OP_JZC 23
#define OP_JCS 24
#define OP_JCC 25
#define OP_JNS 26
#define OP_JNC 27

// Divers
#define OP_IN  28
#define OP_OUT 29
#define OP_RND 30
#define OP_HLT 31

// Petite fonction pour convertir le texte en chiffre
int trouver(char *txt) {
    // securite : On nettoie le texte au cas ou un \r ou \n traine
    char propre[20];
    strncpy(propre, txt, 19);
    propre[strcspn(propre, "\r\n")] = 0;

    // On teste toutes les possibilites une par une, c'est bourrin mais ca marche au moins...
    if (strcmp(propre, "ADD") == 0) return OP_ADD;
    if (strcmp(propre, "SUB") == 0) return OP_SUB;
    if (strcmp(propre, "MUL") == 0) return OP_MUL;
    if (strcmp(propre, "DIV") == 0) return OP_DIV;
    if (strcmp(propre, "AND") == 0) return OP_AND;
    if (strcmp(propre, "OR") == 0)  return OP_OR;
    if (strcmp(propre, "XOR") == 0) return OP_XOR;
    if (strcmp(propre, "SHR") == 0) return OP_SHR;

    // Instructions Memoire
    if (strcmp(propre, "LDB") == 0) return OP_LDB;
    if (strcmp(propre, "LDH") == 0) return OP_LDH;
    if (strcmp(propre, "LDW") == 0) return OP_LDW;
    if (strcmp(propre, "STB") == 0) return OP_STB;
    if (strcmp(propre, "STH") == 0) return OP_STH;
    if (strcmp(propre, "STW") == 0) return OP_STW;

    // les sauts
    if (strcmp(propre, "JMP") == 0) return OP_JMP;
    if (strcmp(propre, "JZS") == 0) return OP_JZS;
    if (strcmp(propre, "JZC") == 0) return OP_JZC;
    if (strcmp(propre, "JCS") == 0) return OP_JCS;
    if (strcmp(propre, "JCC") == 0) return OP_JCC;
    if (strcmp(propre, "JNS") == 0) return OP_JNS;
    if (strcmp(propre, "JNC") == 0) return OP_JNC;

    // le reste
    if (strcmp(propre, "IN") == 0)  return OP_IN;
    if (strcmp(propre, "OUT") == 0) return OP_OUT;
    if (strcmp(propre, "RND") == 0) return OP_RND;
    if (strcmp(propre, "HLT") == 0) return OP_HLT;

    return -1; // pas trouve
}

// Recupere le numero du registre depuis la chaine "R12" ou "(R12)"
int registre(char *arg) {
    // Si c'est un pointeur type (R1), on saute la parenthese pour lire R1
    if (arg[0] == '(') {
        return registre(arg + 1); // recursif c'est plus propre
    }

    // Si ca commence bien par R, on convertit la suite
    if (arg[0] == 'R' || arg[0] == 'r') {
        return atoi(arg + 1);
    }
    return 0; // Par defaut 0 si y'a un probleme
}

// Recupere la valeur numerique apres le #
// Gere aussi bien le decimal que l'hexa (petite fierté du code)
int valeur(char *arg) {
    if (arg[0] == '#') {
        // strtol est pratique car il detecte tout seul la base
        return (int)strtol(arg + 1, NULL, 0);
    }
    // Cas ou le label a deja ete remplace par un nombre brut
    return (int)strtol(arg, NULL, 0);
    //Complètement inutile btw parce qu'on le fait déjà dans le fichier assembleur.c
    //My bad, c'est pas faux dans assembleur on converti les étiquettes en chaine de caractère par exemple boucle = "12" là on converti en Int
    //Du coup je me réponds mais je corrige pas mon commentaire...
}

// C'est ici qu'on fabrique le mot de 32 bits final
unsigned int encoder(Instruction inst) {
    int op = trouver(inst.opcode);

    // Si l'instruction est inconnue ou vide, l'op vaut -1 pour éviter les incompréhension
    // Si on laisse -1, ca fait 11111 (31) => HLT et du coup c'est complètement faux, on a eu beaucoup de problème dessus...
    // Donc si c'est inconnu, on renvoie 0 pour ne pas planter l'execution
    if (op == -1) {
        return 0;
    }

    // Variables pour les champs binaires, initialisees a 0
    int dest = 0;
    int src = 0;
    int flag = 0; // Flag pour dire si on utilise une valeur brute
    int val = 0;  // La valeur ou le 2eme registre

    // cas 1 Instructions classiques (ADD, SUB,etc)
    // format OP Dest, Src1, Src2/Imm
    if ((op >= 0 && op <= 7) || op == OP_RND) {
        dest = registre(inst.args[0]);
        src = registre(inst.args[1]);
        char *tmp = inst.args[2];

        // on regarde si le dernier argument est un nombre (#...)
        if (tmp[0] == '#' || (tmp[0] >= '0' && tmp[0] <= '9') || tmp[0] == '-') {
            flag = 1;
            val = valeur(tmp);
        } else {
            flag = 0;
            val = registre(tmp);
        }
    }

    // cas 2 Memoire (LD / ST)
    // on met toujours l'adresse et le registre dans les champs Dest et Src1 pour faire simple
    else if (op >= 12 && op <= 17) {
        dest = registre(inst.args[0]);
        src = registre(inst.args[1]);
        flag = 0;
        val = 0;
    }

    // Cas 3 Les sauts (JMP...)
    else if (op >= 21 && op <= 27) {
        char *cible = inst.args[0];

        // est-ce qu'on saute vers une adresse dans un registre ou une valeur fixe ?
        if (cible[0] == 'R' || cible[0] == 'r') {
            flag = 0;
            val = registre(cible);
        } else {
            flag = 1;
            val = valeur(cible);
        }
    }

    // Cas 4 Entrees / Sorties
    else if (op == OP_IN || op == OP_OUT) {
        dest = registre(inst.args[0]);
    }

    // Cas 5 HLT (rien a faire, c'est que des 0)

    // Construction de l'entier final avec des decalages de bits
    unsigned int res = 0;
    //l'opérateur << sert à tout décaler à gauche pour écrire le code que le cpu va manger
    res |= (op & 0x1F) << 27;  // Opcode sur 5 bits tout en haut
    res |= (dest & 0x1F) << 22; // Registre destination
    res |= (src & 0x1F) << 17;  // Registre source 1
    res |= (flag & 0x1) << 16;  // Le petit flag immediat
    res |= (val & 0xFFFF);      // La valeur (masquee pour gerer les nombres negatifs)

    return res;
}

void ecrire(Instruction *prog, int n) {
    FILE *f = fopen("hexa.txt", "w");
    if (f == NULL) {
        printf(rouge "Erreur : Impossible de creer le fichier hexa.txt !\n" normal);
        return;
    }

    printf(vert "Ecriture du fichier hexa.txt...\n" normal);

    for (int i = 0; i < n; i++) {
        unsigned int code = encoder(prog[i]);
        // On ecrit en hexadecimal sur 8 chiffres (32 bits)
        fprintf(f, "%08X\n", code);
    }

    fclose(f);
    printf(vert "Fichier genere avec succes.\n" normal);
}
