#include "acquisition.h"
#include "config.h"
#include "modes.h"
#include "status_led.h"
#include "gps_async.h"
#include "bme280.h"
#include "grove_light.h"
#include "grove_rtc_ds1307.h"
#include "sd_logger.h"
#include "logger.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

typedef enum { ACQ_IDLE, ACQ_WAIT_GPS } AcqState;

static AcqState state = ACQ_IDLE;
static SystemMode last_mode = (SystemMode)-1;
static uint32_t next_due = 0;
static uint32_t start_tick = 0;
static uint32_t gga_at_start = 0;
static uint32_t eco_count = 0;

static uint8_t bme_fail = 0;   /* mesures consecutives en echec */
static uint8_t gps_fail = 0;

/* Mesure en cours : texte de chaque champ */
static char f_date[32], f_temp[16], f_hum[16], f_pres[16], f_lum[24], f_gps[40];

static uint32_t period_ms(SystemMode m)
{
    uint32_t base = (uint32_t)g_params.log_interval_min * 60000UL;
    switch (m)
    {
        case MODE_ECO:         return 2U * base;
        case MODE_MAINTENANCE: return MAINTENANCE_PERIOD_MS;
        default:               return base;
    }
}

void Acq_Init(void)
{
    BME280_Init();
}

/* ---------- capteurs immediats (RTC, BME280, lumiere) ---------- */

static void read_date(void)
{
    RTC_DateTime dt;
    if (GroveRTC_GetDateTime(&dt))
        snprintf(f_date, sizeof(f_date), "%04u-%02u-%02u %02u:%02u:%02u",
                 dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds);
    else
        strcpy(f_date, "NA");
}

static void read_bme(void)
{
    Env_Data env;
    bool ok = BME280_ReadEnvironment(&env);
    if (!ok) ok = BME280_Init() && BME280_ReadEnvironment(&env);   /* 2e essai */

    if (!ok)
    {
        if (bme_fail < 255) bme_fail++;
        if (bme_fail >= 2) StatusLed_RaiseError(ERR_SENSOR_ACCESS);  /* 2 echecs -> erreur */
        strcpy(f_temp, g_params.temp_air ? "NA" : "OFF");
        strcpy(f_hum,  g_params.hygr     ? "NA" : "OFF");
        strcpy(f_pres, g_params.pressure ? "NA" : "OFF");
        return;
    }

    bme_fail = 0;
    StatusLed_ClearError(ERR_SENSOR_ACCESS);

    bool incoherent = false;
    float t = env.temperature_c, h = env.humidity_percent, p = env.pressure_hpa;

    /* Temperature de l'air */
    if (!g_params.temp_air) strcpy(f_temp, "OFF");
    else
    {
        snprintf(f_temp, sizeof(f_temp), "%.1f", t);
        if (t < g_params.min_temp_air || t > g_params.max_temp_air) incoherent = true;
    }

    /* Hygrometrie : ignoree hors de [HYGR_MINT, HYGR_MAXT] */
    if (!g_params.hygr) strcpy(f_hum, "OFF");
    else if (t < g_params.hygr_mint || t > g_params.hygr_maxt) strcpy(f_hum, "NA");
    else snprintf(f_hum, sizeof(f_hum), "%.1f", h);

    /* Pression atmospherique */
    if (!g_params.pressure) strcpy(f_pres, "OFF");
    else
    {
        snprintf(f_pres, sizeof(f_pres), "%.1f", p);
        if (p < g_params.pressure_min || p > g_params.pressure_max) incoherent = true;
    }

    if (incoherent) StatusLed_RaiseError(ERR_SENSOR_INCOHERENT);
    else            StatusLed_ClearError(ERR_SENSOR_INCOHERENT);
}

static void read_light(void)
{
    if (!g_params.lumin) { strcpy(f_lum, "OFF"); return; }

    uint16_t raw = GroveLight_ReadRaw();
    if (raw == 0xFFFF) { strcpy(f_lum, "NA"); return; }

    uint16_t v = raw / 4U;   /* 12 bits (0-4095) -> 10 bits (0-1023) */
    const char *level = v < g_params.lumin_low ? "faible"
                      : v > g_params.lumin_high ? "forte" : "moyenne";
    snprintf(f_lum, sizeof(f_lum), "%u(%s)", v, level);
}

/* ---------- fin de mesure : construction et sortie de la ligne ---------- */

static void finish(SystemMode m)
{
    char line[160];
    snprintf(line, sizeof(line), "%s;T=%s;H=%s;P=%s;L=%s;GPS=%s",
             f_date, f_temp, f_hum, f_pres, f_lum, f_gps);

    if (m == MODE_MAINTENANCE)
    {
        printf("[LIVE] %s\r\n", line);          /* maintenance : pas d'ecriture SD */
    }
    else
    {
        printf("[LOG]  %s\r\n", line);
        if (Logger_Write(f_date, line))
            printf("[SD]   ligne enregistree\r\n");
    }

    uint32_t now = HAL_GetTick();
    uint32_t due = start_tick + period_ms(m);
    next_due = ((int32_t)(due - now) > 0) ? due : now;
    state = ACQ_IDLE;
}

static void take_gps(void)
{
    GPS_Data g = GpsAsync_LastGga();
    gps_fail = 0;
    StatusLed_ClearError(ERR_GPS);
    if (g.valid)
        snprintf(f_gps, sizeof(f_gps), "%.5f,%.5f", g.latitude, g.longitude);
    else
        strcpy(f_gps, "NA");      /* trames recues mais pas de position */
}

void Acq_Update(void)
{
    GpsAsync_Poll();

    SystemMode m = Modes_Get();
    uint32_t now = HAL_GetTick();

    /* Changement de mode : on repart de zero, mesure immediate */
    if (m != last_mode)
    {
        /* Maintenance : la carte SD est demontee, on peut la retirer sans risque */
        if (m == MODE_MAINTENANCE)
        {
            SDLogger_Unmount();
            printf("Carte SD demontee : retrait possible en toute securite\r\n");
        }
        else if (last_mode == MODE_MAINTENANCE)
        {
            printf("Carte SD : reprise de l'enregistrement\r\n");
        }
        last_mode = m;
        state = ACQ_IDLE;
        next_due = now;
    }

    if (m == MODE_CONFIG) return;   /* acquisition desactivee */

    if (state == ACQ_IDLE && (int32_t)(now - next_due) >= 0)
    {
        start_tick = now;
        read_date();
        read_bme();
        read_light();

        /* Mode eco : GPS une mesure sur deux */
        bool need_gps = true;
        if (m == MODE_ECO) need_gps = (eco_count++ % 2U) == 0;

        if (!need_gps)
        {
            strcpy(f_gps, "SKIP");
            finish(m);
        }
        else
        {
            gga_at_start = GpsAsync_GgaCount();
            state = ACQ_WAIT_GPS;   /* on attend une trame fraiche, sans bloquer */
        }
    }
    else if (state == ACQ_WAIT_GPS)
    {
        if (GpsAsync_GgaCount() != gga_at_start)
        {
            take_gps();
            finish(m);
        }
        else if (now - start_tick >= (uint32_t)g_params.timeout_s * 1000UL)
        {
            /* TIMEOUT : aucune trame GPS -> NA, erreur apres 2 fois */
            strcpy(f_gps, "NA");
            if (gps_fail < 255) gps_fail++;
            if (gps_fail >= 2) StatusLed_RaiseError(ERR_GPS);
            finish(m);
        }
    }
}
