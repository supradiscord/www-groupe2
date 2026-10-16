#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f4xx_hal.h"

/*
 * Grove Chainable RGB LED :
 * - Port Grove D6 du shield
 * - Signal1=D6=PB10 → CIN (horloge P9813)
 * - Signal2=D7=PA8  → DIN (données P9813)
 */
#define RGB_DATA_PORT   GPIOA
#define RGB_DATA_PIN    GPIO_PIN_8

#define RGB_CLK_PORT    GPIOB
#define RGB_CLK_PIN     GPIO_PIN_10

/*
 * Grove Light Sensor v1.3
 * Port Grove A2 du shield (PA4 = ADC1_IN4 sur F401RE ; c'etait IN9 sur L476RG).
 */
#define LIGHT_ADC_CHANNEL  ADC_CHANNEL_4
#define LIGHT_GPIO_PORT    GPIOA
#define LIGHT_GPIO_PIN     GPIO_PIN_4

/*
 * Grove Dual Button v1.0
 * Port Grove D8 du shield (F401RE) : D8 = PA9, D9 = PC7.
 * (Sur L476RG il etait sur D2, mais D2 = PA10 sert maintenant au GPS.)
 */
#define BUTTON1_PORT GPIOA
#define BUTTON1_PIN  GPIO_PIN_9

#define BUTTON2_PORT GPIOC
#define BUTTON2_PIN  GPIO_PIN_7

/*
 * Grove GPS Air530Z
 * La F401RE n'a PAS d'UART4. Le GPS passe donc sur le port Grove D2 :
 * Signal1 = D2 = PA10 = USART1_RX (AF7) <- TX du GPS.
 * On ne fait que lire les trames NMEA, donc RX seul suffit.
 */
#define GPS_UART_INSTANCE USART1
#define GPS_RX_PORT       GPIOA
#define GPS_RX_PIN        GPIO_PIN_10

/*
 * Grove RTC v1.2 DS1307 + BME680
 * Tous deux sur I2C.
 */
#define GROVE_I2C_HANDLE hi2c1

/*
 * SD Card Shield Seeed v4.3
 * SPI Arduino : D11/D12/D13 et CS sur D4.
 */
#define SD_CS_PORT GPIOB
#define SD_CS_PIN  GPIO_PIN_5

#define SD_SPI_HANDLE hspi1

#endif