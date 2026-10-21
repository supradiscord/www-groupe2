#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "app_types.h"

void StatusLed_Init(void);
void StatusLed_SetMode(SystemMode mode);
void StatusLed_RaiseError(SystemError err);   /* active une erreur          */
void StatusLed_ClearError(SystemError err);   /* desactive une erreur       */
SystemError StatusLed_GetError(void);         /* erreur affichee (prioritaire) */
void StatusLed_Update(void);                  /* a appeler dans la boucle   */

#endif
