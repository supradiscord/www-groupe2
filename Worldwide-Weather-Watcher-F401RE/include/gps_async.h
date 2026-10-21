#ifndef GPS_ASYNC_H
#define GPS_ASYNC_H

#include <stdbool.h>
#include <stdint.h>
#include "cesi_types.h"

/*
 * Reception GPS en arriere-plan (interruption UART) :
 * les octets arrivent dans un buffer circulaire, GpsAsync_Poll() decoupe
 * les trames et garde la derniere trame GGA recue.
 */
void GpsAsync_Start(void);          /* apres GroveGPS_Init() */
void GpsAsync_Poll(void);           /* a appeler souvent dans la boucle */
uint32_t GpsAsync_GgaCount(void);   /* nombre de trames GGA recues */
GPS_Data GpsAsync_LastGga(void);    /* derniere trame GGA decodee */

#endif
