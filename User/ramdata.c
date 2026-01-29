//смотри ramdata.h
#include "modbus_data.h"//содержит описания структур данных
#include "intmash_dsp.h" 

volatile TRAM_DATA RAM_DATA;

u16 FLASH_change;
u16 period_over;

u16 AIN_buf[3];


