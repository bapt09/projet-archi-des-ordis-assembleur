#include <stdio.h>
#include <stdlib.h>
#include "commun.h"

#define TAILLE_MEM 65536

// borne pour la taille d'entré
#define INT_MAX 2147483647
#define INT_MIN -2147483648

//Initialisation

// La memoire
unsigned char memoire[TAILLE_MEM];

// Les registres de R0 a R31
int reg[32];

// Le Compteur
unsigned int pc;

// Les Drapeaux
unsigned char Z; // Zero
unsigned char C; // Carry
unsigned char N; // Negatif

// fonction qui peuvent servir

// gérer les nombre négatif sur 16bit
int extension_signe(short v) {
    return (int)v;
}

//On utilise un long long pour detecter les depassements 32 bits
//Attention ici le carry est interprete comme overflow signe c'est un parti pris pour le projet, il y a d'autres méthodes
void maj_flags(long long res) {
    //zero
    if (res == 0) {
        Z = 1;
    }
    else {
        Z = 0;
    }
    //Negatif
    if (res < 0) {
        N = 1;
    }
    else {
        N = 0;
    }
    //Carry
    // si le resultat sort des bornes d'un int 32 bits
    // Ce n'est pas un carry materiel exact, mais ca suffit pour la machine du sujet
    if (res > INT_MAX || res < INT_MIN) {
        C = 1;
    }
    else {
        C = 0;
    }
}

// remise a zero
void init_machine() { //on reinitialise tout
    int i;
    //on efface la memoire
    for (i = 0; i < TAILLE_MEM; i++) {
        memoire[i] = 0;
    }
    for (i = 0; i < 32; i++) {
        reg[i] = 0;
    }
    pc = 0;
    Z = 0;
    C = 0;
    N = 0;
}

// Chargement du fichier hexa.txt
void charger_programme(const char *nom) {
    FILE *f = fopen(nom, "r");

    if (f == NULL) {
        printf(rouge "Erreur : Impossible d'ouvrir %s\n" normal, nom);
        exit(1);
    }

    char ligne[64];
    unsigned int addr = 0;
    unsigned int code;

    // Lecture ligne par ligne
while (fgets(ligne, sizeof(ligne), f)) {
        // On essaie de lire un nombre hexa sur la ligne
        //on reconstruit
        if (sscanf(ligne, "%x", &code) == 1) {
            memoire[addr++] = (code >> 24) & 0xFF; // Le morceau tout a gauche, opcode + registre 1 début
            memoire[addr++] = (code >> 16) & 0xFF; //morceau suivant, fin registre 1 plus registre 2 et flag
            memoire[addr++] = (code >> 8)  & 0xFF; // Celui d'apres
            memoire[addr] = code & 0xFF;         // Le dernier morceau les deux blocs pour la valeur immédiates
            addr++;
        }
    }
    fclose(f);
}

//el processor

void executer() {
    int run = 1;
    // Limite de cycles pour eviter les boucles infinies en cas de bug
    // Valeur arbitraire mais suffisante pour les tests
    int max_cycles = 50000;
    printf("Debut de l'execution...\n");
    while (run == 1 && max_cycles > 0) {

        // vérif si PC est dans la mémoire sinon erreur dangereuse
        if (pc + 4 > TAILLE_MEM) {
            printf(rouge "Erreur : PC hors memoire (%d)\n" normal, pc);
            break;
        }
        // Lecture d'instruction, unsigned sinon nombre négatif => erreur, on récupère bit un par un
        unsigned char b1 = memoire[pc];
        unsigned char b2 = memoire[pc+1];
        unsigned char b3 = memoire[pc+2];
        unsigned char b4 = memoire[pc+3];
        //reconstitution de l'instruction sur 32 bits
        //chaque octet est lu separement pour eviter les problemes de signe sinon ça bug
        unsigned int instr = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
        // On avance le PC de 4 octets
        pc += 4;
        // on décode
        // on récupère les champs de l'instruction
        int opcode = (instr >> 27) & 0x1F; // code operation
        int rd     = (instr >> 22) & 0x1F; // registre destination
        int rn     = (instr >> 17) & 0x1F; // registre source
        int imm    = (instr >> 16) & 1;    // flag immediat
        int val_16 = instr & 0xFFFF;       // valeur brute
        // Preparation du deuxieme operande
        int op2=0;
        if (imm == 1) {
            // Si c'est immediat, on etend le signe
            op2 = extension_signe(val_16);
        }
        else {
            // Sinon c'est un registre
            int num_reg = val_16 & 0x1F;
            op2 = reg[num_reg];
        }
        long long res = 0; // Utilise pour detecter les overflow, cun registre est sur 32bit donc l'adition de deux registres pour éviter l'overflow on prend large
        int ecrire_reg = 1; // Par defaut on ecrit le resultat
        // execution, grosse partie du code, switch plus beau que if du coup y en a plein
        switch (opcode) {
            //maths
            case 0: // or
                res = reg[rn] | op2; // ou binaire
                break;
            case 1: // and
                res = reg[rn] & op2; //et binaire
                break;
            case 2: // xor
                res = reg[rn] ^ op2; // xor binaire
                break;
            case 3: // ADD
                res = (long long)reg[rn] + op2;
                break;
            case 4: // sub
                res = (long long)reg[rn] - op2;
                break;
            case 5: // MUL
                res = (long long)reg[rn] * op2;
                break;
            case 6: // div
                if (op2 != 0) {
                    res = reg[rn] / op2;
                } else {
                    printf(rouge "Erreur: Division par 0 !\n" normal);
                    run = 0; // Arret d'urgence, complètement interdit
                    ecrire_reg = 0;
                }
                break;
            case 7: // decalage
                if (op2 >= 0) {
                    // Decalage a droite
                    // Le cast en unsigned permet un decalage logique a droite
                    // Sans ca, un int negatif ferait un decalage arithmetique
                    res = (unsigned int)reg[rn] >> op2; //En vrai le unsigned int est un peu tordu mais c'est une astuce vraiment efficace et necessaire pour décaler
                }
                else {
                    // Decalage a gauche
                    res = (unsigned int)reg[rn] << (-op2);
                }
                break;
            //lecture de mémoire
            case 12: // ldb
                {
                    unsigned int adr = reg[rn] + op2;
                    // Verification bornes
                    if (adr >= TAILLE_MEM) {
                        printf(rouge "Erreur de segmentation LDB (%d)\n" normal, adr);
                        run = 0;
                        ecrire_reg = 0;
                    }
                    else {
                        res = memoire[adr];
                    }
                }
                break;
            case 13: // ldh
                {
                    unsigned int adr = reg[rn] + op2;
                    if (adr >= TAILLE_MEM - 1) {
                        printf(rouge "Erreur de segmentation LDH (%d)\n" normal, adr);
                        run = 0;
                        ecrire_reg = 0;
                    }
                    else {
                        // On lit 2 octets
                        res = (memoire[adr] << 8) | memoire[adr+1];
                    }
                }
                break;
            case 14: // ldw
                {
                    unsigned int adr = reg[rn] + op2;
                    if (adr >= TAILLE_MEM - 3) {
                        printf(rouge "Erreur de segmentation LDW (%d)\n" normal, adr);
                        run = 0;
                        ecrire_reg = 0;
                    } else {
                        // On lit 4 octets
                        res = (memoire[adr] << 24) |
                              (memoire[adr+1] << 16) |
                              (memoire[adr+2] << 8) |
                              memoire[adr+3];
                    }
                }
                break;
            //mémoire d'ecriture
            case 15: // STB
                {
                    unsigned int adr = reg[rd] + op2;
                    if (adr >= TAILLE_MEM) {
                        printf(rouge "Erreur de segmentation STB (%d)\n" normal, adr);
                        run = 0;
                    }
                    else {
                        memoire[adr] = reg[rn] & 0xFF;
                    }
                    ecrire_reg = 0; // Pas d'ecriture registre
                }
                break;
            case 16: // STH
                {
                    unsigned int adr = reg[rd] + op2;
                    if (adr >= TAILLE_MEM - 1) {
                        printf(rouge "Erreur de segmentation STH (%d)\n" normal, adr);
                        run = 0;
                    }
                    else {
                        memoire[adr]   = (reg[rn] >> 8) & 0xFF;
                        memoire[adr+1] = reg[rn] & 0xFF;
                    }
                    ecrire_reg = 0;
                }
                break;
            case 17: // STW
                {
                    unsigned int adr = reg[rd] + op2;
                    if (adr >= TAILLE_MEM - 3) {
                        printf(rouge "Erreur de segmentation STW (%d)\n" normal, adr);
                        run = 0;
                    }
                    else {
                        memoire[adr]   = (reg[rn] >> 24) & 0xFF;
                        memoire[adr+1] = (reg[rn] >> 16) & 0xFF;
                        memoire[adr+2] = (reg[rn] >> 8) & 0xFF;
                        memoire[adr+3] = reg[rn] & 0xFF;
                    }
                    ecrire_reg = 0;
                }
                break;
            // sauts
            case 21: // JMP
                pc = op2;
                ecrire_reg = 0;
                break;
            case 22: // JZS
                if (Z == 1) pc = op2;
                ecrire_reg = 0;
                break;
            case 23: // JZC
                if (Z == 0) pc = op2;
                ecrire_reg = 0;
                break;
            case 24: // JCS
                if (C == 1) pc = op2;
                ecrire_reg = 0;
                break;
            case 25: // JCC
                if (C == 0) pc = op2;
                ecrire_reg = 0;
                break;
            case 26: // JNS
                if (N == 1) pc = op2;
                ecrire_reg = 0;
                break;
            case 27: // JNC
                if (N == 0) pc = op2;
                ecrire_reg = 0;
                break;
            // in/out
            case 28: // IN
                printf("Entrez R%d : ", rd);
                int valeur_lue;
                int i = scanf("%i", &valeur_lue);//%i pour integer, c'est hyper pratique pour convertir un nombre en entré directement
                if (i != 1){ //Quand on fait scanf(...) ça renvoie 1 si tout est correcte, 0 sinon on peut donx tester si l'entrée est valide
                    printf(rouge "Erreur d'entree\n"normal);
                    break;
                }
                res = valeur_lue;
                break;
            case 29: // OUT
                printf(">>> OUT R%d = %d\n", rd, reg[rd]);
                res = reg[rd]; // Par convention on laisse la valeur
                break;
            case 30: // RND
                if (op2 > reg[rn]) {
                    // Formule pour essayer de créer de l'aléatoire : min + rand() % (max - min + 1)
                    res = reg[rn]+rand()%(op2-reg[rn]+1);
                }
                else {
                    res = reg[rn];
                }
                break;
            case 31: // HLT
                printf("Fin du programme (HLT).\n");
                run = 0;
                ecrire_reg = 0;
                break;

            default:
                // Instruction inconnue : on ignore
                ecrire_reg = 0;
                break;
        }
        // ecriture du resultat
        if (run == 1 && ecrire_reg == 1) {

            // On ne touche jamais a R0
            // Meme si une ecriture accidentelle passait
            if (rd != 0) {
                reg[rd] = (int)res;
            }
            // flag
            maj_flags(res);
        }
        // pas utile mais pour etre vraiment sur que r0 = 0
        reg[0] = 0;
        max_cycles--;
    }
}
