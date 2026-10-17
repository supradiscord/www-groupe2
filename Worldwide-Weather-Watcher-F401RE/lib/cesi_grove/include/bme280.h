#ifndef BME280_H
#define BME280_H

#include <stdbool.h>
#include <stdint.h>
#include "cesi_types.h"

/* Capteur BME280 (temperature, humidite, pression) en I2C */
bool BME280_Init(void);
uint8_t BME280_GetAddress(void);   /* 0x76 ou 0x77 (0 si non trouve) */
bool BME280_ReadEnvironment(Env_Data *data);

#endif