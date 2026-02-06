#ifndef MODBUS_H
#define MODBUS_H
#include "mbtypes.h"



//адреса оперативной памяти дисплея
#define r_min_RAM_DATA 0x0000
#define r_max_RAM_DATA 0x107F


//диапазон адресов уставок 
#define r_min_FLASH_DATA 0x2000
#define r_max_FLASH_DATA 0x20FF

//диапазон адресов калибровочных данных 
#define r_min_CD_DATA 0xC000
#define r_max_CD_DATA 0xC0FF



extern bool command_decode(TClient *pC);


#endif
