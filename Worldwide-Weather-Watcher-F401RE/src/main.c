#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "setup.h"
#include "grove_rtc_ds1307.h"
#include "grove_light.h"
#include "grove_gps_air530z.h"
#include "config.h"
#include "buttons.h"
#include "status_led.h"
#include "modes.h"
#include "gps_async.h"
#include "acquisition.h"

/*
 * WORLDWIDE WEATHER WATCHER - Etape 2
 * Modes + boutons (interruptions) + LED d'etat + acquisition des capteurs.
 * Etape 3 : console de configuration / Etape 4 : ecriture carte SD.
 */

int main(void)
{
    Global_Init();
    HAL_Delay(100);

    printf("\r\n=== Worldwide Weather Watcher ===\r\n");

    Params_SetDefaults();
    printf("LOG_INTERVAL = %u min%s\r\n", g_params.log_interval_min,
           DEMO_MODE ? " (mode demo)" : "");

    StatusLed_Init();
    Buttons_Init();

    GroveLight_Init();
    GroveGPS_Init();
    GpsAsync_Start();
    Acq_Init();

    /* Mode de demarrage : bouton rouge appuye -> configuration */
    Modes_Init(Buttons_IsRedPressed());

    uint32_t last_rtc_check = 0;

    while (1)
    {
        Buttons_Update();
        Modes_Update();
        Acq_Update();

        /* Verification de l'horloge RTC toutes les 2 s */
        if (HAL_GetTick() - last_rtc_check >= 2000)
        {
            last_rtc_check = HAL_GetTick();
            RTC_DateTime dt;
            bool rtc_ok = GroveRTC_Init() && GroveRTC_GetDateTime(&dt);

            if (!rtc_ok && StatusLed_GetError() != ERR_RTC)
            {
                printf("ERREUR : horloge RTC inaccessible\r\n");
                StatusLed_RaiseError(ERR_RTC);
            }
            else if (rtc_ok && StatusLed_GetError() == ERR_RTC)
            {
                printf("Horloge RTC de nouveau accessible\r\n");
                StatusLed_ClearError(ERR_RTC);
            }
        }

        StatusLed_Update();
    }
}
