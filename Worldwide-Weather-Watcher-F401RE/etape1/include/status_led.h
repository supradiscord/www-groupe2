#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "app_types.h"

void StatusLed_Init(void);
void StatusLed_SetMode(SystemMode mode);
void StatusLed_SetError(SystemError err);   /* ERR_NONE pour effacer */
SystemError StatusLed_GetError(void);
void StatusLed_Update(void);                /* a appeler dans la boucle principale */

#endif
