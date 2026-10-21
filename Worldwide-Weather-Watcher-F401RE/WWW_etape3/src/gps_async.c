#include "gps_async.h"
#include "board_pins.h"
#include <string.h>
#include <stdlib.h>

extern UART_HandleTypeDef GPS_UART_HANDLE;   /* defini dans grove_gps_air530z.c */

#define RB_SIZE 512U

static volatile uint8_t  rb[RB_SIZE];
static volatile uint16_t rb_head = 0, rb_tail = 0;
static uint8_t rx_byte;

static char line[96];
static uint16_t line_len = 0;

static GPS_Data last_gga;
static uint32_t gga_count = 0;

/* ---------- interruption UART ---------- */

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&GPS_UART_HANDLE);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != &GPS_UART_HANDLE) return;

    uint16_t next = (uint16_t)((rb_head + 1U) % RB_SIZE);
    if (next != rb_tail)            /* buffer plein : octet perdu */
    {
        rb[rb_head] = rx_byte;
        rb_head = next;
    }
    HAL_UART_Receive_IT(&GPS_UART_HANDLE, &rx_byte, 1);   /* octet suivant */
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart != &GPS_UART_HANDLE) return;
    HAL_UART_Receive_IT(&GPS_UART_HANDLE, &rx_byte, 1);   /* on relance */
}

void GpsAsync_Start(void)
{
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_UART_Receive_IT(&GPS_UART_HANDLE, &rx_byte, 1);
}

/* ---------- decodage NMEA ---------- */

static double nmea_to_decimal(const char *value, char hemisphere)
{
    if (value == 0 || value[0] == '\0') return 0.0;
    double raw = atof(value);
    int degrees = (int)(raw / 100.0);
    double dec = degrees + (raw - degrees * 100.0) / 60.0;
    return (hemisphere == 'S' || hemisphere == 'W') ? -dec : dec;
}

static void parse_gga(const char *s)
{
    char copy[96];
    strncpy(copy, s, sizeof(copy));
    copy[sizeof(copy) - 1] = '\0';

    /* decoupage manuel : garde les champs vides ",," */
    char *f[16] = {0};
    int n = 0;
    char *p = copy;
    f[n++] = p;
    while (*p != '\0' && n < 16)
    {
        if (*p == ',' || *p == '*') { *p = '\0'; f[n++] = p + 1; }
        p++;
    }
    if (n < 8) return;

    GPS_Data g = {0};
    strncpy(g.raw_sentence, s, sizeof(g.raw_sentence) - 1);
    g.latitude    = nmea_to_decimal(f[2], f[3][0]);
    g.longitude   = nmea_to_decimal(f[4], f[5][0]);
    g.fix_quality = (uint8_t)atoi(f[6]);
    g.satellites  = (uint8_t)atoi(f[7]);
    g.valid       = g.fix_quality > 0;

    last_gga = g;
    gga_count++;
}

void GpsAsync_Poll(void)
{
    while (rb_tail != rb_head)
    {
        char c = (char)rb[rb_tail];
        rb_tail = (uint16_t)((rb_tail + 1U) % RB_SIZE);

        if (c == '\n')
        {
            line[line_len] = '\0';
            if (strncmp(line, "$GPGGA", 6) == 0 || strncmp(line, "$GNGGA", 6) == 0)
                parse_gga(line);
            line_len = 0;
        }
        else if (c != '\r' && line_len < sizeof(line) - 1)
        {
            line[line_len++] = c;
        }
    }
}

uint32_t GpsAsync_GgaCount(void) { return gga_count; }
GPS_Data GpsAsync_LastGga(void)  { return last_gga; }
