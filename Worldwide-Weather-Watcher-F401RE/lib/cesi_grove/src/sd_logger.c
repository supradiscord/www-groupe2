#include "sd_logger.h"
#include "board_pins.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "ff.h"

/* 1 = affiche les messages de diagnostic [SD diag], 0 = silencieux */
#define SD_DIAG 0
#if SD_DIAG
#define SD_DIAG_PRINTF printf
#else
#define SD_DIAG_PRINTF(...) ((void)0)
#endif

#define SD_CMD0     0
#define SD_CMD1     1
#define SD_CMD8     8
#define SD_CMD55    55
#define SD_CMD58    58
#define SD_ACMD41   41

#define SD_R1_IDLE_STATE      0x01U
#define SD_R1_ILLEGAL_COMMAND 0x04U

#define SD_INIT_TIMEOUT_MS 1000U
#define SD_POWERUP_DELAY_MS 50U

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    const char *label;
} SDChipSelect;

extern SPI_HandleTypeDef SD_SPI_HANDLE;

static SDLoggerCardType g_card_type = SDLOGGER_CARD_UNKNOWN;
static SDLoggerInitStatus g_init_status = SDLOGGER_INIT_ERR_CMD0;
static uint8_t g_active_cs_index = 0;

static const SDChipSelect g_cs_candidates[] = {
    {SD_CS_PORT, SD_CS_PIN, "D4/PB5"}
};

#define SD_CS_CANDIDATE_COUNT ((uint8_t)(sizeof(g_cs_candidates) / sizeof(g_cs_candidates[0])))

static uint8_t SD_SPI_Transfer(uint8_t tx)
{
    uint8_t rx = 0xFF;
    (void)HAL_SPI_TransmitReceive(&SD_SPI_HANDLE, &tx, &rx, 1, 100);
    return rx;
}

static void SD_Deselect(void)
{
    HAL_GPIO_WritePin(g_cs_candidates[g_active_cs_index].port,
                      g_cs_candidates[g_active_cs_index].pin,
                      GPIO_PIN_SET);
    (void)SD_SPI_Transfer(0xFF);
}

static void SD_Select(void)
{
    HAL_GPIO_WritePin(g_cs_candidates[g_active_cs_index].port,
                      g_cs_candidates[g_active_cs_index].pin,
                      GPIO_PIN_RESET);
}

static void SD_SetActiveChipSelect(uint8_t index)
{
    uint8_t i;

    g_active_cs_index = index;
    for (i = 0; i < SD_CS_CANDIDATE_COUNT; ++i)
    {
        HAL_GPIO_WritePin(g_cs_candidates[i].port, g_cs_candidates[i].pin, GPIO_PIN_SET);
    }
}

/* Vitesse SPI : lente (~62 kHz) pour l'initialisation, rapide (1 MHz) ensuite */
static void SD_SetSpeed(uint32_t prescaler)
{
    SD_SPI_HANDLE.Instance->CR1 &= ~SPI_CR1_SPE;
    SD_SPI_HANDLE.Instance->CR1 = (SD_SPI_HANDLE.Instance->CR1 & ~SPI_CR1_BR) | prescaler;
    SD_SPI_HANDLE.Instance->CR1 |= SPI_CR1_SPE;
}

static bool SD_WaitReady(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        if (SD_SPI_Transfer(0xFF) == 0xFF)
        {
            return true;
        }
    }

    return false;
}

static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t response = 0xFF;
    uint8_t frame[6];
    uint8_t i;

    SD_Deselect();
    SD_Select();

    if (cmd != SD_CMD0)
    {
        if (!SD_WaitReady(50U))
        {
            SD_Deselect();
            return 0xFF;
        }
    }

    frame[0] = (uint8_t)(0x40U | cmd);
    frame[1] = (uint8_t)(arg >> 24);
    frame[2] = (uint8_t)(arg >> 16);
    frame[3] = (uint8_t)(arg >> 8);
    frame[4] = (uint8_t)(arg);
    frame[5] = crc;

    for (i = 0; i < 6; ++i)
    {
        (void)SD_SPI_Transfer(frame[i]);
    }

    for (i = 0; i < 10; ++i)
    {
        response = SD_SPI_Transfer(0xFF);
        if ((response & 0x80U) == 0U)
        {
            return response;
        }
    }

    return response;
}

bool SDLogger_Init(void)
{
    uint8_t cs_index;
    uint8_t i;
    uint8_t r1;
    uint8_t r7[4];
    uint8_t ocr[4];
    uint32_t start;
    uint8_t cmd0_try;
    uint8_t idle_samples[8];
    uint8_t cmd0_samples[8];
    uint8_t sample_count;
    (void)idle_samples;
    (void)cmd0_samples;

    g_card_type = SDLOGGER_CARD_UNKNOWN;
    g_init_status = SDLOGGER_INIT_ERR_CMD0;
    SD_SetSpeed(SPI_BAUDRATEPRESCALER_256);

    for (cs_index = 0; cs_index < SD_CS_CANDIDATE_COUNT; ++cs_index)
    {
        SD_SetActiveChipSelect(cs_index);
        SD_Deselect();

        SD_DIAG_PRINTF("[SD diag] try CS %s\r\n", g_cs_candidates[cs_index].label);

        HAL_Delay(SD_POWERUP_DELAY_MS);

        /* Send at least 74 clocks with CS high as mandated by SD SPI mode. */
        for (i = 0; i < 10; ++i)
        {
            (void)SD_SPI_Transfer(0xFF);
        }

        for (i = 0; i < 8; ++i)
        {
            idle_samples[i] = SD_SPI_Transfer(0xFF);
        }
        SD_DIAG_PRINTF("[SD diag] MISO idle bytes: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
               idle_samples[0], idle_samples[1], idle_samples[2], idle_samples[3],
               idle_samples[4], idle_samples[5], idle_samples[6], idle_samples[7]);

        r1 = 0xFFU;
        sample_count = 0U;
        for (cmd0_try = 0; cmd0_try < 32U; ++cmd0_try)
        {
            r1 = SD_SendCommand(SD_CMD0, 0x00000000UL, 0x95U);
            SD_Deselect();

            if (sample_count < 8U)
            {
                cmd0_samples[sample_count] = r1;
                ++sample_count;
            }

            if (r1 == SD_R1_IDLE_STATE)
            {
                break;
            }

            HAL_Delay(2U);
        }

        if (sample_count > 0U)
        {
            SD_DIAG_PRINTF("[SD diag] CMD0 R1 first bytes:");
            for (i = 0; i < sample_count; ++i)
            {
                SD_DIAG_PRINTF(" %02X", cmd0_samples[i]);
            }
            SD_DIAG_PRINTF("\r\n");
        }

        if (r1 != SD_R1_IDLE_STATE)
        {
            g_init_status = SDLOGGER_INIT_ERR_CMD0;
            SD_DIAG_PRINTF("[SD diag] CMD0 failed on %s\r\n", g_cs_candidates[cs_index].label);
            continue;
        }

        r1 = SD_SendCommand(SD_CMD8, 0x000001AAUL, 0x87U);
        if (r1 == SD_R1_IDLE_STATE)
        {
            for (i = 0; i < 4; ++i)
            {
                r7[i] = SD_SPI_Transfer(0xFF);
            }
            SD_Deselect();

            if ((r7[2] != 0x01U) || (r7[3] != 0xAAU))
            {
                g_init_status = SDLOGGER_INIT_ERR_CMD8_PATTERN;
                continue;
            }

            start = HAL_GetTick();
            do
            {
                r1 = SD_SendCommand(SD_CMD55, 0x00000000UL, 0x01U);
                SD_Deselect();
                if (r1 > 0x01U)
                {
                    g_init_status = SDLOGGER_INIT_ERR_ACMD41_TIMEOUT;
                    break;
                }

                r1 = SD_SendCommand(SD_ACMD41, 0x40000000UL, 0x01U);
                SD_Deselect();
                if (r1 == 0x00U)
                {
                    break;
                }
            }
            while ((HAL_GetTick() - start) < SD_INIT_TIMEOUT_MS);

            if (r1 != 0x00U)
            {
                g_init_status = SDLOGGER_INIT_ERR_ACMD41_TIMEOUT;
                continue;
            }

            r1 = SD_SendCommand(SD_CMD58, 0x00000000UL, 0x01U);
            if (r1 != 0x00U)
            {
                SD_Deselect();
                g_init_status = SDLOGGER_INIT_ERR_CMD58;
                continue;
            }

            for (i = 0; i < 4; ++i)
            {
                ocr[i] = SD_SPI_Transfer(0xFF);
            }
            SD_Deselect();

            g_card_type = ((ocr[0] & 0x40U) != 0U) ? SDLOGGER_CARD_SDHC : SDLOGGER_CARD_SDSC;
            g_init_status = SDLOGGER_INIT_OK;
            SD_SetSpeed(SPI_BAUDRATEPRESCALER_16);
            return true;
        }

        if ((r1 & SD_R1_ILLEGAL_COMMAND) == 0U)
        {
            SD_Deselect();
            g_init_status = SDLOGGER_INIT_ERR_CMD8_PATTERN;
            continue;
        }

        SD_Deselect();

        start = HAL_GetTick();
        do
        {
            r1 = SD_SendCommand(SD_CMD1, 0x00000000UL, 0x01U);
            SD_Deselect();
            if (r1 == 0x00U)
            {
                g_card_type = SDLOGGER_CARD_SDSC;
                g_init_status = SDLOGGER_INIT_OK;
                SD_SetSpeed(SPI_BAUDRATEPRESCALER_16);
                return true;
            }
        }
        while ((HAL_GetTick() - start) < SD_INIT_TIMEOUT_MS);

        g_init_status = SDLOGGER_INIT_ERR_CMD1_TIMEOUT;
    }

    return false;
}

SDLoggerCardType SDLogger_GetCardType(void)
{
    return g_card_type;
}

SDLoggerInitStatus SDLogger_GetLastInitStatus(void)
{
    return g_init_status;
}

const char *SDLogger_GetActiveCsLabel(void)
{
    return g_cs_candidates[g_active_cs_index].label;
}

/* ---------- lecture / ecriture de blocs de 512 octets (pour FatFs) ---------- */

static uint32_t SD_BlockAddress(uint32_t sector)
{
    /* SDHC : adresse en blocs ; SDSC : adresse en octets */
    return (g_card_type == SDLOGGER_CARD_SDHC) ? sector : sector * 512U;
}

bool SDLogger_ReadBlock(uint32_t sector, uint8_t *buffer)
{
    if (SD_SendCommand(17, SD_BlockAddress(sector), 0x01U) != 0x00U)   /* CMD17 */
    {
        SD_Deselect();
        return false;
    }

    uint8_t token;
    uint32_t start = HAL_GetTick();
    do
    {
        token = SD_SPI_Transfer(0xFF);
    }
    while (token == 0xFFU && (HAL_GetTick() - start) < 200U);

    if (token != 0xFEU)            /* jeton de debut de donnees */
    {
        SD_Deselect();
        return false;
    }

    for (uint32_t i = 0; i < 512U; i++)
        buffer[i] = SD_SPI_Transfer(0xFF);

    (void)SD_SPI_Transfer(0xFF);   /* CRC ignore */
    (void)SD_SPI_Transfer(0xFF);
    SD_Deselect();
    return true;
}

bool SDLogger_WriteBlock(uint32_t sector, const uint8_t *buffer)
{
    if (SD_SendCommand(24, SD_BlockAddress(sector), 0x01U) != 0x00U)   /* CMD24 */
    {
        SD_Deselect();
        return false;
    }

    (void)SD_SPI_Transfer(0xFF);
    (void)SD_SPI_Transfer(0xFE);   /* jeton de debut de donnees */

    for (uint32_t i = 0; i < 512U; i++)
        (void)SD_SPI_Transfer(buffer[i]);

    (void)SD_SPI_Transfer(0xFF);   /* CRC factice */
    (void)SD_SPI_Transfer(0xFF);

    uint8_t resp = SD_SPI_Transfer(0xFF);
    if ((resp & 0x1FU) != 0x05U)   /* donnees refusees par la carte */
    {
        SD_Deselect();
        return false;
    }

    bool ok = SD_WaitReady(500U);  /* la carte ecrit en interne */
    SD_Deselect();
    return ok;
}

/* ---------- fichiers (FatFs) ---------- */

extern void SD_DiskInvalidate(void);   /* diskio.c */

static FATFS g_fs;
static bool g_mounted = false;
static SDLoggerWriteError g_last_error = SDLOGGER_WRITE_OK;

static void SD_Drop(void)
{
    f_mount(0, "", 0);
    g_mounted = false;
    SD_DiskInvalidate();
}

bool SDLogger_Mount(void)
{
    if (g_mounted) return true;
    SD_DiskInvalidate();                 /* force une nouvelle initialisation de la carte */
    FRESULT fr = f_mount(&g_fs, "", 1);
    if (fr != FR_OK)
    {
        printf("[SD]   f_mount erreur %d\r\n", (int)fr);
        f_mount(0, "", 0);
        return false;
    }
    g_mounted = true;
    return true;
}

void SDLogger_Unmount(void)
{
    if (g_mounted) SD_Drop();
}

bool SDLogger_IsMounted(void)
{
    return g_mounted;
}

bool SDLogger_WriteLine(const char *filename, const char *line)
{
    g_last_error = SDLOGGER_WRITE_OK;

    if (!SDLogger_Mount())
    {
        g_last_error = SDLOGGER_WRITE_ERR_ACCESS;
        return false;
    }

    FIL f;
    if (f_open(&f, filename, FA_OPEN_APPEND | FA_WRITE) != FR_OK)
    {
        g_last_error = SDLOGGER_WRITE_ERR_ACCESS;
        SD_Drop();
        return false;
    }

    UINT bw1 = 0, bw2 = 0;
    UINT len = (UINT)strlen(line);
    FRESULT r1 = f_write(&f, line, len, &bw1);
    FRESULT r2 = (r1 == FR_OK) ? f_write(&f, "\r\n", 2, &bw2) : r1;
    FRESULT rc = f_close(&f);

    if (r1 != FR_OK || r2 != FR_OK || rc != FR_OK)
    {
        g_last_error = SDLOGGER_WRITE_ERR_ACCESS;
        SD_Drop();
        return false;
    }
    if (bw1 != len || bw2 != 2U)        /* ecriture incomplete = carte pleine */
    {
        g_last_error = SDLOGGER_WRITE_ERR_FULL;
        return false;
    }
    return true;
}

SDLoggerWriteError SDLogger_GetLastWriteError(void)
{
    return g_last_error;
}

bool SDLogger_GetFileSize(const char *filename, uint32_t *size)
{
    FILINFO fi;
    if (!SDLogger_Mount()) return false;
    if (f_stat(filename, &fi) != FR_OK) return false;
    *size = (uint32_t)fi.fsize;
    return true;
}

bool SDLogger_FileExists(const char *filename)
{
    FILINFO fi;
    if (!SDLogger_Mount()) return false;
    return f_stat(filename, &fi) == FR_OK;
}

bool SDLogger_Rename(const char *from, const char *to)
{
    if (!SDLogger_Mount()) return false;
    return f_rename(from, to) == FR_OK;
}