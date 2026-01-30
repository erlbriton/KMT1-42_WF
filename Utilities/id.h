/**
  ******************************************************************************
  * @file    ID.h
  * @author  IMD, Sledin
  * @version V1.0.0
  * @date    18-06-2013
  * @brief  Модуль, формирующий ID-строку устройства
  */
#ifndef ID_H
#define ID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

extern char DeviceID[256];
extern void IDinit(u32 SN, const char* ID_text);


#ifdef __cplusplus
} // Закрываем extern "C"
#endif

#endif // Закрываем Guard ID_H (это должно быть в самом конце!)