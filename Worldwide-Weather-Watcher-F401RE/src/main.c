#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "setup.h"
#include "cesi_types.h"
#include "grove_rgb.h"
#include "grove_button.h"
#include "grove_light.h"
#include "grove_rtc_ds1307.h"
#include "grove_gps_air530z.h"
#include "grove_bme680.h"
#include "sd_logger.h"

/*
 * PROGRAMME DE TEST MATERIEL (Nucleo F401RE)
 * Ouvre le Serial Monitor de PlatformIO a 115200 bauds.
 *  - Au demarrage : chaque module est initialise et on affiche OK / ERREUR
 *  - Toutes les 2 s : affichage RTC, BME680, luminosite, GPS
 *  - Bouton 1 (rouge) : LED RGB en rouge / Bouton 2 (vert) : LED RGB en vert
 */

static void print_status(const char *name, bool ok)
{
    printf("%-10s : %s\r\n", name, ok ? "OK" : "ERREUR");
}

int main(void)
{
    Global_Init();
    HAL_Delay(200);

    printf("\r\n=== WWW - test materiel F401RE ===\r\n");

    GroveRGB_Init();
    GroveRGB_SetColor(0, 0, 255);          /* bleu = demarrage */

    GroveButton_Init();
    GroveLight_Init();
    GroveGPS_Init();

    bool rtc_ok = GroveRTC_Init();
    bool bme_ok = GroveBME680_Init();
    bool sd_ok  = SDLogger_Init();

    print_status("RTC", rtc_ok);
    print_status("BME680", bme_ok);
    print_status("Carte SD", sd_ok);

    GroveRGB_SetColor(0, 255, 0);          /* vert = pret */

    uint32_t last = 0;

    while (1)
    {
        /* Boutons : reaction immediate */
        if (GroveButton1_IsPressed())      GroveRGB_SetColor(255, 0, 0);
        else if (GroveButton2_IsPressed()) GroveRGB_SetColor(0, 255, 0);

        if (HAL_GetTick() - last < 2000)
            continue;
        last = HAL_GetTick();

        printf("\r\n--- mesure ---\r\n");

        RTC_DateTime dt;
        if (rtc_ok && GroveRTC_GetDateTime(&dt))
            printf("Date      : %02u/%02u/%04u %02u:%02u:%02u\r\n",
                   dt.day, dt.month, dt.year, dt.hours, dt.minutes, dt.seconds);

        Env_Data env;
        if (bme_ok && GroveBME680_ReadEnvironment(&env))
            printf("Temp      : %.1f C | Hum : %.1f %% | Pres : %.1f hPa\r\n",
                   env.temperature_c, env.humidity_percent, env.pressure_hpa);

        printf("Lumiere   : %u / 4095\r\n", GroveLight_ReadRaw());

        GPS_Data gps = {0};
        if (GroveGPS_ReadGGA(&gps, 1200))
        {
            if (gps.valid)
                printf("GPS       : %.5f, %.5f (%u sat)\r\n",
                       gps.latitude, gps.longitude, gps.satellites);
            else
                printf("GPS       : pas de fix (va pres d'une fenetre)\r\n");
        }
        else
        {
            printf("GPS       : aucune trame recue (verifie le port D2)\r\n");
        }
    }
}
