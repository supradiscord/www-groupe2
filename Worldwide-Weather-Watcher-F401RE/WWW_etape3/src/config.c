#include "config.h"

Params g_params;

void Params_SetDefaults(void)
{
#if DEMO_MODE
    g_params.log_interval_min = 1;
#else
    g_params.log_interval_min = 10;
#endif
    g_params.file_max_size = 4096;
    g_params.timeout_s     = 30;

    g_params.lumin      = 1;
    g_params.lumin_low  = 255;
    g_params.lumin_high = 768;

    g_params.temp_air     = 1;
    g_params.min_temp_air = -10;
    g_params.max_temp_air = 60;

    g_params.hygr      = 1;
    g_params.hygr_mint = 0;
    g_params.hygr_maxt = 50;

    g_params.pressure     = 1;
    g_params.pressure_min = 850;
    g_params.pressure_max = 1080;
}
