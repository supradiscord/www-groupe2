#include "status_led.h"
#include "grove_rgb.h"
#include "stm32f4xx_hal.h"

/*
 * LED d'etat (LED RGB chainable).
 * - Pas d'erreur : couleur continue selon le mode.
 * - Erreur       : clignotement a 1 Hz entre le rouge et une 2e couleur.
 * Fonctionnement non bloquant : StatusLed_Update() est appelee en boucle
 * et n'envoie une couleur a la LED que lorsqu'elle change.
 */

typedef struct { uint8_t r, g, b; } Color;

static const Color RED    = {255,   0,   0};
static const Color GREEN  = {  0, 255,   0};
static const Color YELLOW = {255, 180,   0};
static const Color ORANGE = {255,  60,   0};
static const Color BLUE   = {  0,   0, 255};
static const Color WHITE  = {255, 255, 255};

static SystemMode cur_mode = MODE_STANDARD;
static uint8_t    err_flags = 0;   /* un bit par erreur active */
static Color shown = {1, 2, 3};   /* valeur impossible : force le 1er envoi */

static void show(Color c)
{
    if (c.r == shown.r && c.g == shown.g && c.b == shown.b) return;
    shown = c;
    GroveRGB_SetColor(c.r, c.g, c.b);
}

static Color mode_color(SystemMode m)
{
    switch (m)
    {
        case MODE_CONFIG:      return YELLOW;
        case MODE_MAINTENANCE: return ORANGE;
        case MODE_ECO:         return BLUE;
        case MODE_STANDARD:
        default:               return GREEN;
    }
}

void StatusLed_Init(void)
{
    GroveRGB_Init();
}

void StatusLed_SetMode(SystemMode mode)    { cur_mode = mode; }
void StatusLed_RaiseError(SystemError err) { if (err != ERR_NONE) err_flags |= (uint8_t)(1U << err); }
void StatusLed_ClearError(SystemError err) { if (err != ERR_NONE) err_flags &= (uint8_t)~(1U << err); }

/* Plusieurs erreurs a la fois : on affiche la plus prioritaire (ERR_RTC en tete) */
SystemError StatusLed_GetError(void)
{
    for (int e = ERR_RTC; e > ERR_NONE; e--)
        if (err_flags & (1U << e)) return (SystemError)e;
    return ERR_NONE;
}

void StatusLed_Update(void)
{
    SystemError cur_err = StatusLed_GetError();
    if (cur_err == ERR_NONE)
    {
        show(mode_color(cur_mode));
        return;
    }

    /* Periode de 1 s : rouge pendant red_ms, puis la 2e couleur */
    Color other = BLUE;
    uint32_t red_ms = 500;   /* duree identique pour les 2 couleurs */

    switch (cur_err)
    {
        case ERR_RTC:               other = BLUE;   break;
        case ERR_GPS:               other = YELLOW; break;
        case ERR_SENSOR_ACCESS:     other = GREEN;  break;
        case ERR_SENSOR_INCOHERENT: other = GREEN;  red_ms = 333; break; /* vert 2x plus long  */
        case ERR_SD_FULL:           other = WHITE;  break;
        case ERR_SD_ACCESS:         other = WHITE;  red_ms = 333; break; /* blanc 2x plus long */
        default: break;
    }

    show((HAL_GetTick() % 1000U) < red_ms ? RED : other);
}
