#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	SDLOGGER_CARD_UNKNOWN = 0,
	SDLOGGER_CARD_SDSC,
	SDLOGGER_CARD_SDHC
} SDLoggerCardType;

typedef enum
{
	SDLOGGER_INIT_OK = 0,
	SDLOGGER_INIT_ERR_CMD0,
	SDLOGGER_INIT_ERR_CMD8_PATTERN,
	SDLOGGER_INIT_ERR_ACMD41_TIMEOUT,
	SDLOGGER_INIT_ERR_CMD58,
	SDLOGGER_INIT_ERR_CMD1_TIMEOUT
} SDLoggerInitStatus;

typedef enum
{
	SDLOGGER_WRITE_OK = 0,
	SDLOGGER_WRITE_ERR_ACCESS,   /* carte absente, illisible ou erreur d'ecriture */
	SDLOGGER_WRITE_ERR_FULL      /* plus de place sur la carte */
} SDLoggerWriteError;

bool SDLogger_Init(void);
SDLoggerCardType SDLogger_GetCardType(void);
SDLoggerInitStatus SDLogger_GetLastInitStatus(void);
const char *SDLogger_GetActiveCsLabel(void);
bool SDLogger_BitBangCMD0Test(void);
/* Ajoute une ligne de texte a la fin du fichier (cree le fichier si besoin) */
bool SDLogger_WriteLine(const char *filename, const char *line);
SDLoggerWriteError SDLogger_GetLastWriteError(void);

/* Gestion de la carte et des fichiers (systeme de fichiers FAT via FatFs) */
bool SDLogger_Mount(void);
void SDLogger_Unmount(void);          /* avant de retirer la carte */
bool SDLogger_IsMounted(void);
bool SDLogger_GetFileSize(const char *filename, uint32_t *size);
bool SDLogger_FileExists(const char *filename);
bool SDLogger_Rename(const char *from, const char *to);

/* Acces bas niveau (utilise par diskio.c) */
bool SDLogger_ReadBlock(uint32_t sector, uint8_t *buffer);
bool SDLogger_WriteBlock(uint32_t sector, const uint8_t *buffer);

#endif
