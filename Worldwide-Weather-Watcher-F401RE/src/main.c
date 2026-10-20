#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "setup.h"
#include "grove_rtc_ds1307.h"
#include "buttons.h"
#include "status_led.h"
#include "modes.h"

/*
 * WORLDWIDE WEATHER WATCHER - Etape 1
 * Modes de fonctionnement + boutons (interruptions) + LED d'etat.
 * L'acquisition des capteurs sera ajoutee a l'etape 2.
 */

int main(void)
{
    Global_Init();
    HAL_Delay(100);

    printf("\r\n=== Worldwide Weather Watcher ===\r\n");

    StatusLed_Init();
    Buttons_Init();

    /* Mode de demarrage : bouton rouge appuye -> configuration */
    Modes_Init(Buttons_IsRedPressed());

    uint32_t last_rtc_check = 0;

    while (1)
    {
        Buttons_Update();
        Modes_Update();

        /* Verification de l'horloge RTC toutes les 2 s */
        if (HAL_GetTick() - last_rtc_check >= 2000)
        {
            last_rtc_check = HAL_GetTick();
            RTC_DateTime dt;
            bool rtc_ok = GroveRTC_Init() && GroveRTC_GetDateTime(&dt);

            if (!rtc_ok && StatusLed_GetError() != ERR_RTC)
            {
                printf("ERREUR : horloge RTC inaccessible\r\n");
                StatusLed_SetError(ERR_RTC);
            }
            else if (rtc_ok && StatusLed_GetError() == ERR_RTC)
            {
                printf("Horloge RTC de nouveau accessible\r\n");
                StatusLed_SetError(ERR_NONE);
            }
        }

        StatusLed_Update();
    }
}
