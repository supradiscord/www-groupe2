#include "storage.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include <string.h>

#define STORAGE_ADDR    0x08060000UL      /* debut du secteur 7 */
#define STORAGE_SECTOR  FLASH_SECTOR_7
#define STORAGE_MAGIC   0x57575731UL      /* "WWW1" : reconnait nos donnees */

typedef struct
{
    uint32_t magic;
    uint32_t size;       /* taille de Params : change si la structure evolue */
    Params   params;
    uint32_t checksum;   /* detecte des donnees abimees */
} StoredBlock;

static uint32_t checksum(const uint8_t *data, uint32_t len)
{
    uint32_t sum = 0x12345678UL;
    for (uint32_t i = 0; i < len; i++)
        sum = (sum << 5) + sum + data[i];    /* hash simple (djb2) */
    return sum;
}

bool Storage_Load(void)
{
    const StoredBlock *blk = (const StoredBlock *)STORAGE_ADDR;

    if (blk->magic != STORAGE_MAGIC) return false;          /* flash vide */
    if (blk->size != sizeof(Params)) return false;          /* ancienne version */
    if (blk->checksum != checksum((const uint8_t *)&blk->params, sizeof(Params)))
        return false;                                       /* donnees abimees */

    memcpy(&g_params, &blk->params, sizeof(Params));
    return true;
}

bool Storage_Save(void)
{
    StoredBlock blk;
    memset(&blk, 0, sizeof(blk));
    blk.magic = STORAGE_MAGIC;
    blk.size = sizeof(Params);
    blk.params = g_params;
    blk.checksum = checksum((const uint8_t *)&blk.params, sizeof(Params));

    HAL_FLASH_Unlock();

    /* 1) effacer le secteur (la flash ne s'ecrit qu'apres effacement) */
    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = STORAGE_SECTOR;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    uint32_t bad_sector = 0;

    if (HAL_FLASHEx_Erase(&erase, &bad_sector) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return false;
    }

    /* 2) ecrire le bloc mot de 32 bits par mot de 32 bits */
    const uint32_t *words = (const uint32_t *)&blk;
    uint32_t n = (sizeof(blk) + 3U) / 4U;
    bool ok = true;

    for (uint32_t i = 0; i < n && ok; i++)
        ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, STORAGE_ADDR + 4U * i, words[i]) == HAL_OK;

    HAL_FLASH_Lock();
    return ok;
}
