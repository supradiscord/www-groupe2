#include "logger.h"
#include "config.h"
#include "status_led.h"
#include "sd_logger.h"
#include <stdio.h>
#include <string.h>

/* Revision : 0..9 puis A..Z (noms au format 8.3 : 6 + '_' + 1 caractere) */
static char rev_char(int n)
{
    return (n < 10) ? (char)('0' + n) : (char)('A' + (n - 10));
}

/* "2026-10-21 09:45:00" -> "261021_0.LOG" */
static void make_name(const char *date, int rev, char *out, size_t size)
{
    if (strlen(date) >= 10 && date[4] == '-' && date[7] == '-')
        snprintf(out, size, "%c%c%c%c%c%c_%c.LOG",
                 date[2], date[3], date[5], date[6], date[8], date[9], rev_char(rev));
    else
        snprintf(out, size, "000000_%c.LOG", rev_char(rev));   /* date inconnue (RTC en erreur) */
}

static void sd_ok(void)
{
    StatusLed_ClearError(ERR_SD_ACCESS);
    StatusLed_ClearError(ERR_SD_FULL);
}

void Logger_Init(void)
{
    if (SDLogger_Mount())
    {
        printf("Carte SD  : OK\r\n");
        sd_ok();
    }
    else
    {
        printf("Carte SD  : ERREUR (absente ou illisible)\r\n");
        StatusLed_RaiseError(ERR_SD_ACCESS);
    }
}

/* Archive le fichier _0 s'il est trop gros pour accueillir la nouvelle ligne */
static bool archive_if_needed(const char *date, const char *name, size_t line_len)
{
    uint32_t size = 0;
    if (!SDLogger_GetFileSize(name, &size)) return true;                  /* fichier pas encore cree */
    if (size + line_len + 2U <= g_params.file_max_size) return true;      /* encore de la place */

    char arch[16];
    for (int n = 1; n < 36; n++)
    {
        make_name(date, n, arch, sizeof(arch));
        if (SDLogger_FileExists(arch)) continue;

        if (!SDLogger_Rename(name, arch)) return false;
        printf("[SD]   %s plein -> archive en %s\r\n", name, arch);
        return true;
    }

    printf("[SD]   plus de numero d'archive libre pour aujourd'hui\r\n");
    StatusLed_RaiseError(ERR_SD_FULL);
    return false;
}

bool Logger_Write(const char *date, const char *line)
{
    char name[16];
    make_name(date, 0, name, sizeof(name));

    if (archive_if_needed(date, name, strlen(line)) && SDLogger_WriteLine(name, line))
    {
        sd_ok();
        return true;
    }

    if (SDLogger_GetLastWriteError() == SDLOGGER_WRITE_ERR_FULL)
    {
        printf("[SD]   ERREUR : carte SD pleine\r\n");
        StatusLed_RaiseError(ERR_SD_FULL);
    }
    else if (StatusLed_GetError() != ERR_SD_FULL)
    {
        printf("[SD]   ERREUR : ecriture impossible dans %s\r\n", name);
        StatusLed_RaiseError(ERR_SD_ACCESS);
    }
    return false;
}
