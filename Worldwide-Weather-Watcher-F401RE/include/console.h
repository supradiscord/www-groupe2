#ifndef CONSOLE_H
#define CONSOLE_H

/*
 * Console de configuration sur le port serie (USART2, 115200 bauds).
 * Commandes acceptees en mode CONFIGURATION :
 *   PARAMETRE=valeur   (LOG_INTERVAL=5, LUMIN_LOW=200, ...)
 *   CLOCK=HH:MM:SS     DATE=MM,JJ,AAAA     DAY=MON..SUN
 *   VERSION   RESET   PARAMS   HELP
 */
void Console_Init(void);
void Console_Update(void);   /* a appeler dans la boucle principale */

#endif
