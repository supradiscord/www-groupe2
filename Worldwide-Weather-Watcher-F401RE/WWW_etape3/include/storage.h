#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>

/*
 * Sauvegarde des parametres en memoire FLASH (la F401RE n'a pas d'EEPROM).
 * On utilise le dernier secteur de la flash (secteur 7, 128 Ko a 0x08060000),
 * loin du programme qui occupe le debut de la flash.
 */
bool Storage_Load(void);   /* vrai si des parametres valides ont ete lus */
bool Storage_Save(void);   /* ecrit g_params en flash */

#endif
