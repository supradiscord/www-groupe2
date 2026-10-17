#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "setup.h"
#include "bme280.h"

extern I2C_HandleTypeDef hi2c1;

/*
 * TEST CAPTEUR BME280 - Nucleo F401RE
 * 1) Liste les appareils presents sur le bus I2C
 * 2) Toutes les 2 s : temperature, humidite, pression
 */

static void scan_i2c(void)
{
    printf("Scan I2C : ");
    int found = 0;
    for (uint8_t a = 1; a < 128; a++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(a << 1), 2, 10) == HAL_OK)
        {
            printf("0x%02X ", a);
            found++;
        }
    }
    printf(found ? "\r\n" : "aucun appareil (verifie SCL/SDA/VCC/GND)\r\n");
}

int main(void)
{
    Global_Init();
    HAL_Delay(200);

    printf("\r\n=== TEST BME280 ===\r\n");

    while (1)
    {
        scan_i2c();

        if (BME280_Init())
        {
            printf("BME280    : OK (adresse 0x%02X)\r\n", BME280_GetAddress());

            Env_Data env;
            if (BME280_ReadEnvironment(&env))
                printf("Temp : %.1f C | Hum : %.1f %% | Pres : %.1f hPa\r\n",
                       env.temperature_c, env.humidity_percent, env.pressure_hpa);
        }
        else
        {
            printf("BME280    : ERREUR (non trouve en 0x76 ni 0x77)\r\n");
        }

        printf("\r\n");
        HAL_Delay(2000);
    }
}