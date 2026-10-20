#ifndef APP_TYPES_H
#define APP_TYPES_H

/* Les 4 modes de fonctionnement de la station */
typedef enum
{
    MODE_STANDARD = 0,
    MODE_CONFIG,
    MODE_MAINTENANCE,
    MODE_ECO
} SystemMode;

/* Les erreurs signalees par la LED (par ordre de priorite croissante) */
typedef enum
{
    ERR_NONE = 0,
    ERR_SENSOR_INCOHERENT,  /* rouge / vert  (vert 2x plus long)  */
    ERR_SENSOR_ACCESS,      /* rouge / vert  (duree identique)    */
    ERR_GPS,                /* rouge / jaune                      */
    ERR_SD_FULL,            /* rouge / blanc (duree identique)    */
    ERR_SD_ACCESS,          /* rouge / blanc (blanc 2x plus long) */
    ERR_RTC                 /* rouge / bleu                       */
} SystemError;

#endif
