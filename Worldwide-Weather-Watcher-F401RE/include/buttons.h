#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>

#define BUTTON_LONG_PRESS_MS  5000U   /* appui long = 5 secondes */

typedef enum
{
    BTN_EVT_NONE = 0,
    BTN_EVT_RED_LONG,     /* bouton rouge maintenu 5 s */
    BTN_EVT_GREEN_LONG    /* bouton vert maintenu 5 s  */
} ButtonEvent;

void Buttons_Init(void);
void Buttons_Update(void);          /* a appeler dans la boucle principale */
ButtonEvent Buttons_GetEvent(void); /* renvoie (et consomme) le dernier evenement */
bool Buttons_IsRedPressed(void);
bool Buttons_IsGreenPressed(void);

#endif
