#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MEM_SIZE 65536

// fonctions principales
void init_machine();
void charger_programme(const char *nom);
void executer();

#endif
