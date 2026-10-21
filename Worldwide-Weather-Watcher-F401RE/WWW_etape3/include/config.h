#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/*
 * Mode demonstration : 1 = LOG_INTERVAL de 1 min (demo)
 *                      0 = LOG_INTERVAL de 10 min (valeur du sujet)
 * -> remettre a 0 apres la demonstration.
 */
#define DEMO_MODE 1

/* Version du programme et numero de lot (commande VERSION) */
#define FW_VERSION  "1.0.0"
#define LOT_NUMBER  "WWW-G2-2026-001"

/* Periode d'affichage en mode maintenance (consultation en direct) */
#define MAINTENANCE_PERIOD_MS 5000U

/* Parametres du systeme (valeurs par defaut du sujet).
 * Etape 3 : modifiables par la console serie et sauvegardes en flash. */
typedef struct
{
    uint16_t log_interval_min;  /* LOG_INTERVAL   : minutes entre 2 mesures   */
    uint16_t file_max_size;     /* FILE_MAX_SIZE  : taille max d'un fichier   */
    uint16_t timeout_s;         /* TIMEOUT        : abandon d'un capteur (s)  */

    uint8_t  lumin;             /* LUMIN          : 1 actif / 0 inactif       */
    uint16_t lumin_low;         /* LUMIN_LOW      : en dessous = faible       */
    uint16_t lumin_high;        /* LUMIN_HIGH     : au dessus = forte         */

    uint8_t  temp_air;          /* TEMP_AIR       : 1 actif / 0 inactif       */
    int16_t  min_temp_air;      /* MIN_TEMP_AIR   : seuil bas (C)             */
    int16_t  max_temp_air;      /* MAX_TEMP_AIR   : seuil haut (C)            */

    uint8_t  hygr;              /* HYGR           : 1 actif / 0 inactif       */
    int16_t  hygr_mint;         /* HYGR_MINT      : temp. min prise en compte */
    int16_t  hygr_maxt;         /* HYGR_MAXT      : temp. max prise en compte */

    uint8_t  pressure;          /* PRESSURE       : 1 actif / 0 inactif       */
    uint16_t pressure_min;      /* PRESSURE_MIN   : seuil bas (hPa)           */
    uint16_t pressure_max;      /* PRESSURE_MAX   : seuil haut (hPa)          */
} Params;

extern Params g_params;

void Params_SetDefaults(void);

#endif
