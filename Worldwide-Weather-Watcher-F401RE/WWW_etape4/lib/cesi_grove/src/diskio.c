/*
 * Interface entre FatFs et la carte SD (SPI) du projet WWW.
 * FatFs appelle ces fonctions pour lire/ecrire des blocs de 512 octets.
 */
#include "ff.h"
#include "diskio.h"
#include "sd_logger.h"
#include "grove_rtc_ds1307.h"

static DSTATUS g_stat = STA_NOINIT;

/* Oblige a reinitialiser la carte (apres un retrait ou une erreur) */
void SD_DiskInvalidate(void)
{
    g_stat = STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
    return (pdrv == 0) ? g_stat : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    g_stat = SDLogger_Init() ? 0 : STA_NOINIT;
    return g_stat;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || count == 0) return RES_PARERR;
    if (g_stat & STA_NOINIT) return RES_NOTRDY;

    for (UINT i = 0; i < count; i++)
        if (!SDLogger_ReadBlock((uint32_t)sector + i, buff + 512U * i))
            return RES_ERROR;
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || count == 0) return RES_PARERR;
    if (g_stat & STA_NOINIT) return RES_NOTRDY;

    for (UINT i = 0; i < count; i++)
        if (!SDLogger_WriteBlock((uint32_t)sector + i, buff + 512U * i))
            return RES_ERROR;
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    (void)buff;
    if (pdrv != 0) return RES_PARERR;
    if (cmd == CTRL_SYNC) return RES_OK;   /* chaque bloc est deja ecrit completement */
    return RES_PARERR;
}

/* Date des fichiers : lue dans l'horloge RTC */
DWORD get_fattime(void)
{
    RTC_DateTime dt;
    if (!GroveRTC_GetDateTime(&dt))
        return ((DWORD)(2026 - 1980) << 25) | (1UL << 21) | (1UL << 16);

    return ((DWORD)(dt.year - 1980) << 25) | ((DWORD)dt.month << 21) | ((DWORD)dt.day << 16)
         | ((DWORD)dt.hours << 11) | ((DWORD)dt.minutes << 5) | ((DWORD)dt.seconds / 2U);
}
