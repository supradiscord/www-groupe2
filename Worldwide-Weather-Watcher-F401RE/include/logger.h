#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>

/*
 * Enregistrement des mesures sur la carte SD.
 * Fichier du jour : AAMMJJ_0.LOG (ex : 261021_0.LOG pour le 21/10/2026).
 * Quand il depasse FILE_MAX_SIZE, il est archive en AAMMJJ_1.LOG, _2, ...
 * et l'ecriture reprend dans un AAMMJJ_0.LOG vide.
 */
void Logger_Init(void);                               /* monte la carte au demarrage */
bool Logger_Write(const char *date, const char *line); /* date = "AAAA-MM-JJ hh:mm:ss" */

#endif
