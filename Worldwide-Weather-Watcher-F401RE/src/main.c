#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "setup.h"
#include "sd_logger.h"

/*
 * TEST CARTE SD - Nucleo F401RE
 * Toutes les 2 s : tente d'initialiser la carte SD et affiche le resultat.
 */

static const char *status_text(SDLoggerInitStatus s)
{
    switch (s)
    {
        case SDLOGGER_INIT_OK:                 return "OK";
        case SDLOGGER_INIT_ERR_CMD0:           return "pas de reponse (cablage SPI ou carte absente)";
        case SDLOGGER_INIT_ERR_CMD8_PATTERN:   return "reponse incorrecte (fil MISO/MOSI ?)";
        case SDLOGGER_INIT_ERR_ACMD41_TIMEOUT: return "carte trop lente a demarrer";
        case SDLOGGER_INIT_ERR_CMD58:          return "erreur lecture type de carte";
        case SDLOGGER_INIT_ERR_CMD1_TIMEOUT:   return "vieille carte non reconnue";
        default:                               return "inconnu";
    }
}

static const char *type_text(SDLoggerCardType t)
{
    switch (t)
    {
        case SDLOGGER_CARD_SDHC: return "SDHC (> 2 Go)";
        case SDLOGGER_CARD_SDSC: return "SDSC (<= 2 Go)";
        default:                 return "inconnu";
    }
}

int main(void)
{
    Global_Init();
    HAL_Delay(200);

    printf("\r\n=== TEST CARTE SD ===\r\n");

    while (1)
    {
        bool ok = SDLogger_Init();

        printf("Carte SD  : %s", ok ? "OK" : "ERREUR");
        printf("  -> %s", status_text(SDLogger_GetLastInitStatus()));
        if (ok)
            printf("  | type : %s", type_text(SDLogger_GetCardType()));
        printf("\r\n");

        HAL_Delay(2000);
    }
}