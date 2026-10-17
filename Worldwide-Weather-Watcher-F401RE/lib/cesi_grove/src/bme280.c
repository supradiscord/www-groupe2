#include "bme280.h"
#include "board_pins.h"

extern I2C_HandleTypeDef GROVE_I2C_HANDLE;

#define BME280_CHIP_ID   0x60
#define REG_ID           0xD0
#define REG_RESET        0xE0
#define REG_CTRL_HUM     0xF2
#define REG_CTRL_MEAS    0xF4
#define REG_CONFIG       0xF5
#define REG_DATA         0xF7

static uint16_t addr = 0;   /* adresse I2C decalee (<< 1) */

/* Coefficients de calibration lus dans le capteur */
static uint16_t T1; static int16_t T2, T3;
static uint16_t P1; static int16_t P2, P3, P4, P5, P6, P7, P8, P9;
static uint8_t  H1, H3; static int16_t H2, H4, H5; static int8_t H6;

static bool read_regs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(&GROVE_I2C_HANDLE, addr, reg, I2C_MEMADD_SIZE_8BIT,
                            buf, len, 100) == HAL_OK;
}

static bool write_reg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&GROVE_I2C_HANDLE, addr, reg, I2C_MEMADD_SIZE_8BIT,
                             &value, 1, 100) == HAL_OK;
}

static bool try_address(uint8_t a7)
{
    uint8_t id = 0;
    addr = (uint16_t)(a7 << 1);
    if (!read_regs(REG_ID, &id, 1)) return false;
    return id == BME280_CHIP_ID;
}

uint8_t BME280_GetAddress(void)
{
    return (uint8_t)(addr >> 1);
}

bool BME280_Init(void)
{
    /* L'adresse depend de la broche SDO : 0x76 (GND) ou 0x77 (VCC) */
    if (!try_address(0x76) && !try_address(0x77))
    {
        addr = 0;
        return false;
    }

    write_reg(REG_RESET, 0xB6);
    HAL_Delay(5);

    uint8_t c[26];
    if (!read_regs(0x88, c, 26)) return false;

    T1 = (uint16_t)(c[1] << 8 | c[0]);
    T2 = (int16_t)(c[3] << 8 | c[2]);
    T3 = (int16_t)(c[5] << 8 | c[4]);
    P1 = (uint16_t)(c[7] << 8 | c[6]);
    P2 = (int16_t)(c[9] << 8 | c[8]);
    P3 = (int16_t)(c[11] << 8 | c[10]);
    P4 = (int16_t)(c[13] << 8 | c[12]);
    P5 = (int16_t)(c[15] << 8 | c[14]);
    P6 = (int16_t)(c[17] << 8 | c[16]);
    P7 = (int16_t)(c[19] << 8 | c[18]);
    P8 = (int16_t)(c[21] << 8 | c[20]);
    P9 = (int16_t)(c[23] << 8 | c[22]);
    H1 = c[25];

    uint8_t h[7];
    if (!read_regs(0xE1, h, 7)) return false;
    H2 = (int16_t)(h[1] << 8 | h[0]);
    H3 = h[2];
    H4 = (int16_t)((int8_t)h[3] * 16 | (h[4] & 0x0F));
    H5 = (int16_t)((int8_t)h[5] * 16 | (h[4] >> 4));
    H6 = (int8_t)h[6];

    /* Humidite x1, puis temp x1 + pression x1 en mode normal, mesure toutes les 1 s */
    write_reg(REG_CTRL_HUM, 0x01);
    write_reg(REG_CONFIG, 0xA0);
    write_reg(REG_CTRL_MEAS, 0x27);
    HAL_Delay(50);
    return true;
}

bool BME280_ReadEnvironment(Env_Data *data)
{
    if (data == 0 || addr == 0) return false;

    uint8_t d[8];
    if (!read_regs(REG_DATA, d, 8)) return false;

    int32_t adc_P = (int32_t)((d[0] << 12) | (d[1] << 4) | (d[2] >> 4));
    int32_t adc_T = (int32_t)((d[3] << 12) | (d[4] << 4) | (d[5] >> 4));
    int32_t adc_H = (int32_t)((d[6] << 8) | d[7]);

    /* Formules de compensation officielles Bosch (datasheet BME280) */
    double v1 = ((double)adc_T / 16384.0 - (double)T1 / 1024.0) * (double)T2;
    double v2 = ((double)adc_T / 131072.0 - (double)T1 / 8192.0);
    v2 = v2 * v2 * (double)T3;
    double t_fine = v1 + v2;
    double temp = t_fine / 5120.0;

    v1 = t_fine / 2.0 - 64000.0;
    v2 = v1 * v1 * (double)P6 / 32768.0;
    v2 = v2 + v1 * (double)P5 * 2.0;
    v2 = v2 / 4.0 + (double)P4 * 65536.0;
    v1 = ((double)P3 * v1 * v1 / 524288.0 + (double)P2 * v1) / 524288.0;
    v1 = (1.0 + v1 / 32768.0) * (double)P1;
    double press = 0.0;
    if (v1 != 0.0)
    {
        press = 1048576.0 - (double)adc_P;
        press = (press - v2 / 4096.0) * 6250.0 / v1;
        v1 = (double)P9 * press * press / 2147483648.0;
        v2 = press * (double)P8 / 32768.0;
        press = press + (v1 + v2 + (double)P7) / 16.0;
    }

    double hum = t_fine - 76800.0;
    hum = ((double)adc_H - ((double)H4 * 64.0 + (double)H5 / 16384.0 * hum)) *
          ((double)H2 / 65536.0 * (1.0 + (double)H6 / 67108864.0 * hum *
          (1.0 + (double)H3 / 67108864.0 * hum)));
    hum = hum * (1.0 - (double)H1 * hum / 524288.0);
    if (hum > 100.0) hum = 100.0;
    if (hum < 0.0) hum = 0.0;

    data->temperature_c = (float)temp;
    data->pressure_hpa = (float)(press / 100.0);
    data->humidity_percent = (float)hum;
    data->gas_resistance_ohm = 0.0f;   /* pas de capteur de gaz sur le BME280 */
    return true;
}