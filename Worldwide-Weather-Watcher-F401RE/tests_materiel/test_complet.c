#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "setup.h"
#include "cesi_types.h"
#include "grove_rgb.h"
#include "grove_button.h"
#include "grove_light.h"
#include "grove_rtc_ds1307.h"
#include "grove_gps_air530z.h"
#include "bme280.h"
#include "sd_logger.h"

/*
 * TEST DU MONTAGE COMPLET - Nucleo F401RE
 * Bouton vert (D8) -> LED verte / Bouton rouge (D9) -> LED rouge
 * Toutes les 2 s : date, temperature/humidite/pression, lumiere, GPS
 */

static void print_status(const char *name, bool ok)
{
    printf("%-10s : %s\r\n", name, ok ? "OK" : "ERREUR");
}


/*
 * Remise a l'heure automatique de la RTC.
 * Si l'horloge est arretee (les secondes n'avancent pas), on la regle
 * avec la date et l'heure de COMPILATION du programme (__DATE__ / __TIME__).
 */
static uint8_t day_of_week(int y, int m, int d)   /* 1 = lundi ... 7 = dimanche */
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    int w = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;  /* 0 = dimanche */
    return (uint8_t)(w == 0 ? 7 : w);
}

static void rtc_set_from_build_time(void)
{
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *date = __DATE__;   /* ex : "Oct 17 2026" */
    const char *time = __TIME__;   /* ex : "21:40:12"    */

    int month = 1;
    for (int i = 0; i < 12; i++)
        if (date[0] == months[i * 3] && date[1] == months[i * 3 + 1] && date[2] == months[i * 3 + 2])
            month = i + 1;

    int day  = (date[4] == ' ' ? 0 : date[4] - '0') * 10 + (date[5] - '0');
    int year = (date[7] - '0') * 1000 + (date[8] - '0') * 100 + (date[9] - '0') * 10 + (date[10] - '0');

    RTC_DateTime now = {0};
    now.hours     = (uint8_t)((time[0] - '0') * 10 + (time[1] - '0'));
    now.minutes   = (uint8_t)((time[3] - '0') * 10 + (time[4] - '0'));
    now.seconds   = (uint8_t)((time[6] - '0') * 10 + (time[7] - '0'));
    now.day       = (uint8_t)day;
    now.month     = (uint8_t)month;
    now.year      = (uint16_t)year;
    now.dayOfWeek = day_of_week(year, month, day);

    GroveRTC_SetDateTime(&now);
}

static void rtc_check_running(void)
{
    RTC_DateTime a, b;
    if (!GroveRTC_GetDateTime(&a)) return;
    HAL_Delay(1100);
    if (!GroveRTC_GetDateTime(&b)) return;

    if (a.seconds == b.seconds)
    {
        printf("RTC       : horloge arretee -> remise a l'heure de compilation\r\n");
        rtc_set_from_build_time();
    }
}

int main(void)
{
    Global_Init();
    HAL_Delay(200);

    printf("\r\n=== WWW - test montage complet ===\r\n");

    GroveRGB_Init();
    GroveRGB_SetColor(0, 0, 255);        /* bleu = demarrage */

    GroveButton_Init();
    GroveLight_Init();
    GroveGPS_Init();

    bool rtc_ok = GroveRTC_Init();
    if (rtc_ok) rtc_check_running();
    bool bme_ok = BME280_Init();
    bool sd_ok  = SDLogger_Init();

    print_status("RTC", rtc_ok);
    print_status("BME280", bme_ok);
    print_status("Carte SD", sd_ok);

    GroveRGB_SetColor(0, 255, 0);        /* vert = pret */

    uint32_t last = 0;

    while (1)
    {
        if (GroveButton1_IsPressed())      GroveRGB_SetColor(0, 255, 0);
        else if (GroveButton2_IsPressed()) GroveRGB_SetColor(255, 0, 0);

        if (HAL_GetTick() - last < 2000)
            continue;
        last = HAL_GetTick();

        printf("\r\n--- mesure ---\r\n");

        RTC_DateTime dt;
        if (rtc_ok && GroveRTC_GetDateTime(&dt))
            printf("Date      : %02u/%02u/%04u %02u:%02u:%02u\r\n",
                   dt.day, dt.month, dt.year, dt.hours, dt.minutes, dt.seconds);
        else
            printf("Date      : NA\r\n");

        Env_Data env;
        if (bme_ok && BME280_ReadEnvironment(&env))
            printf("Temp      : %.1f C | Hum : %.1f %% | Pres : %.1f hPa\r\n",
                   env.temperature_c, env.humidity_percent, env.pressure_hpa);
        else
            printf("Temp      : NA\r\n");

        printf("Lumiere   : %u / 4095\r\n", GroveLight_ReadRaw());

        GPS_Data gps = {0};
        if (GroveGPS_ReadGGA(&gps, 1200))
        {
            if (gps.valid)
                printf("GPS       : %.5f, %.5f (%u sat)\r\n",
                       gps.latitude, gps.longitude, gps.satellites);
            else
                printf("GPS       : trames OK, pas encore de fix\r\n");
        }
        else
        {
            printf("GPS       : aucune trame recue\r\n");
        }
    }
}