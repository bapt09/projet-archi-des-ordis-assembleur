#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "assembleur.h"
#include "commun.h"


//Bienvenu dans le code de l'assembleur, bon courage pour ne pas vous perdre dans les méandres de ces 500 lignes de code...

// commandes disponibles définies
const char *Commande[] = {
    "ADD", "SUB", "MUL", "DIV", "AND", "OR", "XOR", "SHR",
    "LDB", "LDH", "LDW", "STB", "STH", "STW",
    "IN", "OUT",
    "JMP", "JZS", "JZC", "JCS", "JCC", "JNS", "JNC",
    "RND", "HLT",
    NULL
};

// structure d'un labelw
typedef struct {
    char nom[50];
    int adr;
    int use;
} label_t;


label_t tab_labels[100];
int nb_lab = 0;


// outils perso

// est ce un chiffre
int est_chiffre(char c) {
    return (c >= '0' && c <= '9'); // Basique
}

// lettre chiffre ou underscore
int est_alpha(char c) {
    if (c >= '0' && c <= '9') return 1;
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= 'a' && c <= 'z') return 1;
    if (c == '_') return 1;
    return 0; // Pas un caractere valide pour un nom
}

// minuscules vers majuscules
char maj(char c) {
    if (c >= 'a' && c <= 'z') return c - 32; // Petite astuce ASCII
    return c;
}

// separateurs espace tab entree
int est_sep(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}


// verifs types

// registre R0 R31
int est_reg(char *s) {
    if (!s) return 0;
    // R ou r
    if (s[0] != 'R' && s[0] != 'r') return 0; // Faut que ca commence par R sinon c'est mort
    // chiffre apres
    if (!est_chiffre(s[1])) return 0;

    // reste est numerique
    for (int i = 1; s[i]; i++) {
        if (!est_chiffre(s[i])) return 0;
    }

    int n = atoi(s + 1);
    return (n >= 0 && n <= 31); // On verifie que ca depasse pas les 32 registres
}

// cas R0 pour warning
int est_r0(char *s) {
    if (!s) return 0;
    return ((s[0] == 'R' || s[0] == 'r') && s[1] == '0' && !s[2]); // Juste pour eviter d'ecrire dans R0 pour rien
}

// valeur immediate dièse
int est_imm(char *s) {
    return (s && s[0] == '#'); // Faut le diese
}

// memoire avec parentheses
int est_mem(char *s) {
    if (!s) return 0;
    int len = strlen(s);

    if (len < 3 || s[0] != '(' || s[len-1] != ')') return 0; // Pas de parentheses pas de chocolat

    char buf[20];
    int j = 0;
    // recup interieur
    for(int i = 1; i < len - 1 && j < 19; i++) buf[j++] = s[i];
    buf[j] = '\0';

    return est_reg(buf); // Faut que ce soit un registre a l'interieur
}

// syntaxe nom label
int nom_ok(char *s) {
    if (!s || !strlen(s)) return 0;
    if (est_chiffre(s[0])) return 0; // pas de chiffre debut, regle de base

    for(int i=0; s[i]; i++) {
        if (!est_alpha(s[i])) return 0;
    }
    return 1;
}

// commande dans la liste
int cmd_ok(char *s) {
    for (int i = 0; Commande[i]; i++) {
        if (strcmp(s, Commande[i]) == 0) return 1; // On regarde si la commande existe vraiment
    }
    return 0;
}


//instructions logique

int verif_math(Instruction *ins, int l) {
    if (ins->nb_args != 3) {
        printf(rouge "Ligne %d: %s veut 3 args\n" normal, l, ins->opcode); // Erreur bete, pas assez d'argument
        return 0;
    }
    // test registre
    if (!est_reg(ins->args[0])) {
        printf(rouge "Ligne %d: arg 1 pas registre\n" normal, l);
        return 0;
    }
    if (est_r0(ins->args[0])) {
        printf(jaune "Ligne %d: ecriture R0 inutile\n" normal, l); // Warning sympa pour prevenir
    }
    // src1 registre
    if (!est_reg(ins->args[1])) {
        printf(rouge "Ligne %d: arg 2 pas registre\n" normal, l);
        return 0;
    }
    // src2 pas memoire
    if (est_mem(ins->args[2])) {
        printf(rouge "Ligne %d: pointeur interdit ici\n" normal, l); // On melange pas tout, pas de pointeur
        return 0;
    }
    return 1;
}
//Instructions mémoire
int verif_trans(Instruction *ins, int l) {
    char *op = ins->opcode;

    // LDB
    if (op[0] == 'L') {
        if (!est_reg(ins->args[0])) {
            printf(rouge "Ligne %d: dest doit etre registre\n" normal, l);
            return 0;
        }
        if (est_imm(ins->args[1])) {
            printf(rouge "Ligne %d: load immediat interdit\n" normal, l); // Ca n'a pas de sens
            return 0;
        }
        if (!est_mem(ins->args[1])) {
            printf(rouge "Ligne %d: adresse sans parentheses\n" normal, l); // Oubli classique...
            return 0;
        }
    }
    // STB
    else if (op[0] == 'S') {
        if (!est_mem(ins->args[0])) {
            printf(rouge "Ligne %d: dest doit etre adresse\n" normal, l);
            return 0;
        }
        if (!est_reg(ins->args[1])) {
            printf(rouge "Ligne %d: source doit etre registre\n" normal, l);
            return 0;
        }
    }
    return 1;
}

int verif_saut(Instruction *ins, int l) {
    if (ins->nb_args != 1) {
        printf(rouge "Ligne %d: saut 1 arg\n" normal, l);
        return 0;
    }
    if (est_imm(ins->args[0])) {
        printf(rouge "Ligne %d: saut immediat non\n" normal, l); // Faut viser une etiquette
        return 0;
    }
    if (est_mem(ins->args[0])) {
        printf(rouge "Ligne %d: pas de parentheses\n" normal, l);
        return 0;
    }
    return 1;
}

// Vérification bonne entrée
int verif_tout(Instruction *ins, int l) {
    char *op = ins->opcode;

// opérateur logique
    // On utilise == 0 car strcmp renvoie 0 si les chaines sont identiques
    if (strcmp(op, "ADD") == 0 || strcmp(op, "SUB") == 0 || strcmp(op, "MUL") == 0 ||
        strcmp(op, "DIV") == 0 || strcmp(op, "AND") == 0 || strcmp(op, "OR")  == 0 ||
        strcmp(op, "XOR") == 0 || strcmp(op, "SHR") == 0 || strcmp(op, "RND") == 0) {
        return verif_math(ins, l);
    }

    // memoire
    if (strcmp(op, "LDB") == 0 || strcmp(op, "LDH") == 0 || strcmp(op, "LDW") == 0 ||
        strcmp(op, "STB") == 0 || strcmp(op, "STH") == 0 || strcmp(op, "STW") == 0) {
        return verif_trans(ins, l);
    }

    // sauts
    //comme tous les sauts commencent par J, on teste juste la premiere lettre
    if (op[0] == 'J') return verif_saut(ins, l);

    // io (In/Out, si on l'a pas...)
    if (strcmp(op, "IN") == 0 || strcmp(op, "OUT") == 0) {
        if (ins->nb_args != 1) {
            printf(rouge "Ligne %d: io veut 1 arg\n" normal, l);
            return 0;
        }
        if (!est_reg(ins->args[0])) {
            printf(rouge "Ligne %d: faut un registre\n" normal, l);
            return 0;
        }
        return 1;
    }

    // stop
    if (!strcmp(op, "HLT")) {
        if (ins->nb_args > 0) printf(jaune "Ligne %d: args HLT ignores\n" normal, l); // On previent quand meme, mais c'est pas ouf
        return 1;
    }
    return 1;
}


// Analyse et découpage

int lit_arg(char *s) {
    // valeur
    if (s[0] == '#') {
        if (!s[1]) return 1; // format

        char *ptr;
        long v = strtol(s + 1, &ptr, 0); // tentative de gestion de l'hexadécimal, pratique strtol

        if (*ptr != '\0') return 1; // chars invalides

        // overflow 16 bits (Pas sûr d'avoir bien compris la phrase "La machine est comprend une mémoire de 64ko...)
        // Mais bon on securise quand meme
        if (v < -32768 || v > 65535) return 3;
        return 0;
    }

    // registre
    if (s[0] == 'R' || s[0] == 'r') {
        for (int k = 1; s[k] != '\0'; k++) { //vérifie si chiffre après lettre
            // erreur sinon
            if (est_chiffre(s[k]) == 0) {
                return 1;
            }
        }
        int registre_id = atoi(s + 1); //pour enlever le premier R
        if (registre_id < 0 || registre_id > 31) { //32 registres
            return 2; //registre n'existe
        }

        return 0; //registre valide
    }
    // pointeur
    if (s[0] == '(') {
        if (!est_mem(s)) return 1;
        return 0;
    }
    return 0;
}

// analyse ligne brute
// il analyse ligne par ligne avec gestion des commentaires simples (//, ;) et bloques (/* */)
// La variable com permet de savoir si on est actuellement dans un commentaire juste c'était plus rapide com...
int lit_ligne(char *s, Instruction *ins, int *com) {
    // Fonction complexe de decoupage lexical
    // La logique a ete ecrite progressivement puis ajustee pour gerer tous les cas du sujet
    //Mais c'est aune des fonctions les plus importante...
    int i = 0, len = strlen(s), ok = 0;

    // normal
    strcpy(ins->label, ""); strcpy(ins->opcode, ""); ins->nb_args = 0; //On met tout à 0, label = "", opcode=""

    while (i < len) { //La gestion des commentaires est pas très optimisé, 2 boucles while mais ca fait le taff
        // gestion comentaire
        if (*com) {
            // fin bloc
            if (s[i] == '*' && s[i+1] == '/') { *com = 0; i += 2; }
            else i++;
            continue;
        }
        // debut bloc
        if (s[i] == '/' && s[i+1] == '*') { *com = 1; i += 2; continue; }
        // ligne simple
        if ((s[i] == '/' && s[i+1] == '/') || s[i] == ';') break;

        // separateurs
        if (est_sep(s[i]) || s[i] == ',') { i++; continue; }

        // mot
        char buf[120];
        int j = 0;
        int par = 0; // dans parentheses

        //correctif pour accepter les labels du type label : avec un espace
        //On normalise pour eviter des erreurs de syntaxe inutiles
        for (int i = 1; s[i] != '\0'; i++) {
            if (s[i] == ':' && s[i-1] == ' ') {
                    s[i-1] = ':';
                    s[i] = ' ';
                }
        }

        //J'ai trouvé une partie de cette fonction sur github, mais que j'ai adapté pour mieux géré les différents types de commentaires et aussi parce qu'elle marchait pas trop...
        while (i < len) {
            // stop si separateur hors parentheses
            if (!par) {
                if ((s[i]=='/' && (s[i+1]=='/' || s[i+1]=='*')) || s[i]==';') break;
                if (s[i] == ':' || est_sep(s[i]) || s[i] == ',') break;
            }

            if (s[i] == '(') {
                par = 1;
                buf[j++] = s[i++];
            }
            else if (s[i] == ')') {
                par = 0;
                buf[j++] = s[i++];
            }
            else {
                // skip espace dans par
                if (par && est_sep(s[i])) i++;
                else buf[j++] = s[i++];
            }
            if (j >= 119) break; // Securite buffer
        }
        buf[j] = '\0';

        if (j > 0) {
            ok = 1;

            // label fini par deux points
            if (s[i] == ':') {
                i++;
                if (!nom_ok(buf)) {
                    printf(rouge "Label '%s' invalide\n" normal, buf);
                    return -1;
                }
                strcpy(ins->label, buf);
                continue;
            }
            // opcode (code opérateur)
            else if (strlen(ins->opcode) == 0) {
                // majuscule
                for(int k=0; buf[k]; k++) buf[k] = maj(buf[k]); // On normalise tout en majuscule

                if (cmd_ok(buf)) strcpy(ins->opcode, buf);
                else {
                    printf(rouge "Commande '%s' inconnue\n" normal, buf); // faute de frappe
                    return -1;
                }
            }
            // argument
            else {
                if (ins->nb_args < 4) {//différents types d'erreur
                    int err = lit_arg(buf);
                    if (err == 1) { printf(rouge "Format arg '%s' ko\n" normal, buf); return -1; }
                    else if (err == 2) { printf(rouge "Reg '%s' ko\n" normal, buf); return -1; }
                    else if (err == 3) { printf(rouge "Overflow '%s'\n" normal, buf); return -1; }

                    // tout en decimal
                    if (buf[0] == '#') {
                        long v = strtol(buf+1, NULL, 0);
                        sprintf(ins->args[ins->nb_args], "#%ld", v);
                    } else {
                        strcpy(ins->args[ins->nb_args], buf);
                    }
                    ins->nb_args++;
                } else {
                    printf(rouge "Trop d'args\n" normal); // Faut pas abuser non plus
                    return -1;
                }
            }
        }
        else if (s[i] == ':') i++;
    }
    return ok;
}


// gestion des labels


int traiter_labels(Instruction *prog, int n) {
    int err = 0, nb_lab = 0;

    // passe 1 reperage, première lecture du fichier
    for (int i = 0; i < n; i++) {
        if (strlen(prog[i].label) > 0) {
            // doublons, problème ?
            for(int k=0; k<nb_lab; k++) {
                if(!strcmp(tab_labels[k].nom, prog[i].label)) {
                    printf(rouge "Label '%s' deja la\n" normal, prog[i].label); // Doublon detecte donc pas de problème
                    err++;
                }
            }
            strcpy(tab_labels[nb_lab].nom, prog[i].label);
            tab_labels[nb_lab].adr = i * 4;
            tab_labels[nb_lab].use = 0;
            nb_lab++;
        }
    }

    // passe 2 remplacement
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < prog[i].nb_args; j++) {
            char *arg = prog[i].args[j];
            // pas registre pas valeur pas mémoire => label
            if (!est_reg(arg) && !est_imm(arg) && !est_mem(arg)) {
                //recherche linéaire dans la table des symboles
                int tt = 0;
                for (int k = 0; k < nb_lab; k++) {
                    if (!strcmp(arg, tab_labels[k].nom)) {
                        sprintf(prog[i].args[j], "#%d", tab_labels[k].adr); // Hop on remplace par l'adresse
                        tab_labels[k].use = 1;
                        tt = 1; break;
                    }
                }
                if (!tt) {
                    printf(rouge "Label '%s' pas trouve\n" normal, arg); // Etiquette fantome
                    err++;
                }
            }
        }
    }

    // labels inutiles
    for(int k=0; k < nb_lab; k++) {
        if(!tab_labels[k].use)
            printf(jaune "Warning: label '%s' sert a rien\n" normal, tab_labels[k].nom); //Warning, on s'est dit qu'il fallait en avoir comme en C :)
    }
    return err;
}


// Fonction principal du programme

int assembler(char *nom, Instruction *out, int *n) {
    FILE* f = fopen(nom, "r");
    if (!f) {
        printf(rouge "Erreur lecture %s\n" normal, nom); // Fichier introuvable
        return -1;
    }

    char line[255];
    int com = 0, err = 0, l = 0;
    *n = 0;

    printf(vert "Phase 1 analyse\n" normal);

    while (fgets(line, 255, f)) {
        l++;

        // petit nettoyage pour virer les retours à la ligne
        line[strcspn(line, "\r")] = 0;
        line[strcspn(line, "\n")] = 0;

        Instruction tmp;

        int res = lit_ligne(line, &tmp, &com); // On analyse la ligne courante

        if (res == -1) {
            printf(rouge "Syntaxe ligne %d\n" normal, l);
            err++;
        }
        else if (res == 1) {
            if (!verif_tout(&tmp, l)) err++; // On verifie la logique des phrases, deuxième vérification
            else {
                out[*n] = tmp;
                (*n)++;
            }
        }
    }
    fclose(f);

    if (err > 0) {
        printf(rouge "Stop: %d erreurs\n" normal, err);
        return -1;
    }

    printf(vert "Phase 2 labels\n" normal);

    if (traiter_labels(out, *n) > 0) return -1;

    // fin, HLT obligatoire
    if (*n == 0 || strcmp(out[*n - 1].opcode, "HLT") != 0) { //Si le fichier contient rien=> pas de hlt, warnin, si il est pas vide on regarde la dernière instruction
        printf(jaune "Warning: pas de HLT\n" normal);
    }

    // taille, petit point qu'on peut améliorer, en fait on prend pas le programme dans le disque dur du coup on a moins de stockage
    if (*n > 16384) {
        printf(rouge "Erreur: prog trop gros\n" normal); // Memoire saturee
        return -1;
    }

    return 0;
}



//Cette fonction est complétement inutile, je l'utilisais pour voir si ma compilation marchait mais je me suis dit que c'était sympa de la laisser
void afficher_prog(Instruction *prog, int n) {
    if (nb_lab > 0) {
        printf(bleu "\nLabels:\n" normal);
        for(int k=0; k<nb_lab; k++)
            printf(" %s -> %d\n", tab_labels[k].nom, tab_labels[k].adr); // Petit recap des labels
    }
    printf(vert "\nCode:\n" normal);
    for(int i = 0; i < n; i++){
        char s[100] = "";
        for(int k=0; k < prog[i].nb_args; k++) {
            strcat(s, prog[i].args[k]);
            if (k < prog[i].nb_args - 1) strcat(s, ", ");
        }
        printf("%-10s | %-6s | %s\n", prog[i].label, prog[i].opcode, s);
    }
}
