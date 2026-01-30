/*
  * @file    flashdata.h
  * @author  IMD, Sledin
  * @version V1.0.0
  * @date    18-06-2013
  * @brief   Файл содержит объявление структур, располагаемых в основном и 
  * резервном секторах уставок во FLASH памяти микроконтроллера.
  */
#ifndef FLASHDATA_H
#define FLASHDATA_H

#ifdef __cplusplus
extern "C" {
#endif
#include "modbus_data.h"//содержит описания структур данных

//параметры расположеные FLASH

extern TFLASH_DATA FLASH_DATA;
extern TFLASH_DATA BKFLASH_DATA;
//extern TFLASH_DATA *settings;
#ifdef __cplusplus
}
#endif
#endif
