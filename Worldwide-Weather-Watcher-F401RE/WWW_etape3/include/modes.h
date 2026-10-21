#ifndef MODES_H
#define MODES_H

#include <stdbool.h>
#include "app_types.h"

#define CONFIG_TIMEOUT_MS  (30UL * 60UL * 1000UL)  /* 30 min sans activite */

void Modes_Init(bool red_pressed_at_boot);
void Modes_Update(void);              /* a appeler dans la boucle principale */
void Modes_NotifyActivity(void);      /* activite en mode config (commande serie) */
SystemMode Modes_Get(void);
const char *Modes_Name(SystemMode m);

#endif
