# Portage L476RG -> F401RE

## Branchements sur le Base Shield Grove
| Module            | Port Grove | Broches STM32           |
|-------------------|------------|-------------------------|
| RTC DS1307        | I2C        | PB8 (SCL) / PB9 (SDA)   |
| BME680            | I2C        | PB8 / PB9               |
| LED RGB chainable | D6         | PB10 (CLK) / PA8 (DATA) |
| Capteur lumiere   | A2         | PA4 (ADC1_IN4)          |
| GPS Air530Z       | D2  (modifie, etait A0) | PA10 = USART1_RX |
| Double bouton     | D8  (modifie, etait D2) | PA9 / PC7        |
| Shield SD         | SPI        | D11/D12/D13 + CS D4 (PB5) |

## Fichiers modifies
- platformio.ini : board nucleo_f401re, upload stlink, monitor 115200
- stm32l4xx_hal.h -> stm32f4xx_hal.h (setup.c, board_pins.h, sd_logger.c)
- setup.c : horloge HSI 16 MHz, I2C en ClockSpeed (F4), USART2 sans options L4, GPS sur USART1
- board_pins.h : canal ADC 4, boutons sur D8, GPS sur D2
- grove_light.c : init ADC version F4
- grove_gps_air530z.c : USART1 en reception seule (pas d'UART4 sur F401)
- main.c : programme de test de tous les composants

Remarque : la LED LD2 (PA5) partage la broche SCK du SPI, elle clignotera pendant les acces SD. C'est normal.
