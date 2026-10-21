#include "modes.h"
#include "buttons.h"
#include "status_led.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

/*
 * Machine a etats des modes de fonctionnement.
 *
 *  Demarrage sans bouton        -> STANDARD
 *  Demarrage bouton rouge appuye -> CONFIG
 *
 *  STANDARD    --rouge 5 s--> MAINTENANCE (memorise STANDARD)
 *  STANDARD    --vert 5 s---> ECO
 *  ECO         --rouge 5 s--> MAINTENANCE (memorise ECO)
 *  ECO         --vert 5 s---> STANDARD     (choix de conception, voir doc)
 *  MAINTENANCE --rouge 5 s--> mode precedent
 *  CONFIG      --30 min sans activite (commande ou appui)--> STANDARD
 */

static SystemMode mode = MODE_STANDARD;
static SystemMode previous = MODE_STANDARD;   /* pour sortir de maintenance */
static uint32_t last_activity = 0;

const char *Modes_Name(SystemMode m)
{
    switch (m)
    {
        case MODE_STANDARD:    return "STANDARD";
        case MODE_CONFIG:      return "CONFIGURATION";
        case MODE_MAINTENANCE: return "MAINTENANCE";
        case MODE_ECO:         return "ECONOMIQUE";
        default:               return "?";
    }
}

static void enter(SystemMode m)
{
    mode = m;
    StatusLed_SetMode(m);
    printf("\r\n>>> Mode %s\r\n", Modes_Name(m));
}

void Modes_Init(bool red_pressed_at_boot)
{
    last_activity = HAL_GetTick();
    Buttons_TakeActivity();
    enter(red_pressed_at_boot ? MODE_CONFIG : MODE_STANDARD);
}

void Modes_NotifyActivity(void)
{
    last_activity = HAL_GetTick();
}

SystemMode Modes_Get(void)
{
    return mode;
}

void Modes_Update(void)
{
    ButtonEvent e = Buttons_GetEvent();

    switch (mode)
    {
        case MODE_STANDARD:
            if (e == BTN_EVT_RED_LONG)        { previous = MODE_STANDARD; enter(MODE_MAINTENANCE); }
            else if (e == BTN_EVT_GREEN_LONG) { enter(MODE_ECO); }
            break;

        case MODE_ECO:
            if (e == BTN_EVT_RED_LONG)        { previous = MODE_ECO; enter(MODE_MAINTENANCE); }
            else if (e == BTN_EVT_GREEN_LONG) { enter(MODE_STANDARD); }
            break;

        case MODE_MAINTENANCE:
            if (e == BTN_EVT_RED_LONG)        { enter(previous); }
            break;

        case MODE_CONFIG:
            if (Buttons_TakeActivity()) last_activity = HAL_GetTick();  /* un appui = activite */
            if (HAL_GetTick() - last_activity >= CONFIG_TIMEOUT_MS)
            {
                printf("Config : 30 min sans activite\r\n");
                enter(MODE_STANDARD);
            }
            break;
    }
}
