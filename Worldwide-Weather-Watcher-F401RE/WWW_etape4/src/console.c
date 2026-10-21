#include "console.h"
#include "config.h"
#include "storage.h"
#include "modes.h"
#include "grove_rtc_ds1307.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

extern UART_HandleTypeDef huart2;   /* port serie du ST-Link (setup.c) */

/* ---------- reception par interruption (buffer circulaire) ---------- */

#define RB_SIZE 128U
static volatile uint8_t  rb[RB_SIZE];
static volatile uint16_t rb_head = 0, rb_tail = 0;

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE))
    {
        uint8_t c = (uint8_t)USART2->DR;     /* lire DR efface aussi l'overrun */
        uint16_t next = (uint16_t)((rb_head + 1U) % RB_SIZE);
        if (next != rb_tail) { rb[rb_head] = c; rb_head = next; }
    }
}

void Console_Init(void)
{
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/* ---------- table des parametres ---------- */

typedef enum { T_U8, T_U16, T_I16 } ParamType;

typedef struct
{
    const char *name;
    void *ptr;
    ParamType type;
    int32_t min, max;
} ParamDef;

static const ParamDef params[] = {
    {"LOG_INTERVAL",  &g_params.log_interval_min, T_U16, 1,   1440},
    {"FILE_MAX_SIZE", &g_params.file_max_size,    T_U16, 256, 65535},
    {"TIMEOUT",       &g_params.timeout_s,        T_U16, 1,   300},
    {"LUMIN",         &g_params.lumin,            T_U8,  0,   1},
    {"LUMIN_LOW",     &g_params.lumin_low,        T_U16, 0,   1023},
    {"LUMIN_HIGH",    &g_params.lumin_high,       T_U16, 0,   1023},
    {"TEMP_AIR",      &g_params.temp_air,         T_U8,  0,   1},
    {"MIN_TEMP_AIR",  &g_params.min_temp_air,     T_I16, -40, 85},
    {"MAX_TEMP_AIR",  &g_params.max_temp_air,     T_I16, -40, 85},
    {"HYGR",          &g_params.hygr,             T_U8,  0,   1},
    {"HYGR_MINT",     &g_params.hygr_mint,        T_I16, -40, 85},
    {"HYGR_MAXT",     &g_params.hygr_maxt,        T_I16, -40, 85},
    {"PRESSURE",      &g_params.pressure,         T_U8,  0,   1},
    {"PRESSURE_MIN",  &g_params.pressure_min,     T_U16, 300, 1100},
    {"PRESSURE_MAX",  &g_params.pressure_max,     T_U16, 300, 1100},
};
#define NB_PARAMS (sizeof(params) / sizeof(params[0]))

static int32_t get_value(const ParamDef *p)
{
    switch (p->type)
    {
        case T_U8:  return *(uint8_t *)p->ptr;
        case T_U16: return *(uint16_t *)p->ptr;
        default:    return *(int16_t *)p->ptr;
    }
}

static void set_value(const ParamDef *p, int32_t v)
{
    switch (p->type)
    {
        case T_U8:  *(uint8_t *)p->ptr  = (uint8_t)v;  break;
        case T_U16: *(uint16_t *)p->ptr = (uint16_t)v; break;
        default:    *(int16_t *)p->ptr  = (int16_t)v;  break;
    }
}

/* ---------- utilitaires ---------- */

static bool parse_int(const char *s, int32_t *out)
{
    if (*s == '\0') return false;
    char *end;
    long v = strtol(s, &end, 10);
    if (*end != '\0') return false;
    *out = (int32_t)v;
    return true;
}

static void save_and_report(void)
{
    if (Storage_Save()) printf("  -> sauvegarde en flash OK\r\n");
    else                printf("  -> ERREUR de sauvegarde en flash\r\n");
}

static void print_params(void)
{
    printf("Parametres actuels :\r\n");
    for (unsigned i = 0; i < NB_PARAMS; i++)
        printf("  %-14s = %ld\r\n", params[i].name, (long)get_value(&params[i]));
}

static void print_help(void)
{
    printf("Commandes : PARAMETRE=valeur | CLOCK=HH:MM:SS | DATE=MM,JJ,AAAA\r\n");
    printf("            DAY=MON..SUN | VERSION | RESET | PARAMS | HELP\r\n");
}

/* ---------- commandes horloge ---------- */

static void cmd_clock(const char *v)
{
    int h, m, s;
    if (sscanf(v, "%d:%d:%d", &h, &m, &s) != 3 || h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59)
    {
        printf("ERREUR : format CLOCK=HH:MM:SS (0-23:0-59:0-59)\r\n");
        return;
    }
    RTC_DateTime dt;
    if (!GroveRTC_GetDateTime(&dt)) { printf("ERREUR : RTC inaccessible\r\n"); return; }
    dt.hours = (uint8_t)h; dt.minutes = (uint8_t)m; dt.seconds = (uint8_t)s;
    if (GroveRTC_SetDateTime(&dt)) printf("OK : heure reglee a %02d:%02d:%02d\r\n", h, m, s);
    else                           printf("ERREUR : ecriture RTC\r\n");
}

static void cmd_date(const char *v)
{
    int mo, d, y;
    if (sscanf(v, "%d,%d,%d", &mo, &d, &y) != 3 || mo < 1 || mo > 12 || d < 1 || d > 31 || y < 2000 || y > 2099)
    {
        printf("ERREUR : format DATE=MOIS,JOUR,ANNEE (1-12,1-31,2000-2099)\r\n");
        return;
    }
    RTC_DateTime dt;
    if (!GroveRTC_GetDateTime(&dt)) { printf("ERREUR : RTC inaccessible\r\n"); return; }
    dt.month = (uint8_t)mo; dt.day = (uint8_t)d; dt.year = (uint16_t)y;
    if (GroveRTC_SetDateTime(&dt)) printf("OK : date reglee au %02d/%02d/%04d\r\n", d, mo, y);
    else                           printf("ERREUR : ecriture RTC\r\n");
}

static void cmd_day(const char *v)
{
    static const char *days[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
    for (int i = 0; i < 7; i++)
    {
        if (strcmp(v, days[i]) == 0)
        {
            RTC_DateTime dt;
            if (!GroveRTC_GetDateTime(&dt)) { printf("ERREUR : RTC inaccessible\r\n"); return; }
            dt.dayOfWeek = (uint8_t)(i + 1);
            if (GroveRTC_SetDateTime(&dt)) printf("OK : jour = %s\r\n", days[i]);
            else                           printf("ERREUR : ecriture RTC\r\n");
            return;
        }
    }
    printf("ERREUR : DAY=MON, TUE, WED, THU, FRI, SAT ou SUN\r\n");
}

/* ---------- traitement d'une ligne ---------- */

static void process(char *cmd)
{
    for (char *c = cmd; *c; c++) *c = (char)toupper((unsigned char)*c);   /* majuscules */

    if (Modes_Get() != MODE_CONFIG)
    {
        printf("Commandes disponibles uniquement en mode CONFIGURATION\r\n");
        printf("(redemarrer en tenant le bouton rouge)\r\n");
        return;
    }
    Modes_NotifyActivity();   /* une commande = activite (delai de 30 min) */

    if (strcmp(cmd, "VERSION") == 0)
    {
        printf("Worldwide Weather Watcher v%s - lot %s\r\n", FW_VERSION, LOT_NUMBER);
        return;
    }
    if (strcmp(cmd, "RESET") == 0)
    {
        Params_SetDefaults();
        printf("OK : parametres remis aux valeurs par defaut\r\n");
        save_and_report();
        return;
    }
    if (strcmp(cmd, "PARAMS") == 0) { print_params(); return; }
    if (strcmp(cmd, "HELP") == 0)   { print_help();   return; }

    char *eq = strchr(cmd, '=');
    if (eq == 0) { printf("ERREUR : commande inconnue '%s' (HELP pour l'aide)\r\n", cmd); return; }
    *eq = '\0';
    const char *name = cmd, *value = eq + 1;

    if (strcmp(name, "CLOCK") == 0) { cmd_clock(value); return; }
    if (strcmp(name, "DATE") == 0)  { cmd_date(value);  return; }
    if (strcmp(name, "DAY") == 0)   { cmd_day(value);   return; }
    if (strcmp(name, "LOG_INTERVALL") == 0) name = "LOG_INTERVAL";   /* orthographe du sujet */

    for (unsigned i = 0; i < NB_PARAMS; i++)
    {
        if (strcmp(name, params[i].name) != 0) continue;

        int32_t v;
        if (!parse_int(value, &v)) { printf("ERREUR : '%s' n'est pas un nombre entier\r\n", value); return; }
        if (v < params[i].min || v > params[i].max)
        {
            printf("ERREUR : valeur hors limites (%ld a %ld)\r\n", (long)params[i].min, (long)params[i].max);
            return;
        }
        set_value(&params[i], v);
        printf("OK : %s = %ld\r\n", params[i].name, (long)v);
        save_and_report();
        return;
    }
    printf("ERREUR : parametre inconnu '%s' (PARAMS pour la liste)\r\n", name);
}

/* ---------- lecture des caracteres recus ---------- */

static char line[64];
static uint8_t len = 0;

void Console_Update(void)
{
    while (rb_tail != rb_head)
    {
        char c = (char)rb[rb_tail];
        rb_tail = (uint16_t)((rb_tail + 1U) % RB_SIZE);

        if (c == '\r' || c == '\n')
        {
            if (len > 0)
            {
                line[len] = '\0';
                printf("\r\n");          /* la reponse commence sur une nouvelle ligne */
                process(line);
                len = 0;
            }
        }
        else if (c == ' ')
        {
            /* on ignore les espaces : "LOG_INTERVAL = 5" fonctionne aussi */
        }
        else if (len < sizeof(line) - 1)
        {
            line[len++] = c;
        }
    }
}
