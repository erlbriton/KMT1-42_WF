#include "modbus_data.h"//содержит описания структур данных
//#include "memutil.h"

#pragma section = ".fdsection"
const TFLASH_DATA FLASH_DATA @ ".fdsection" =
{         
    0x0401, //Modbus2_USB;
    //уставки работы
    //1667,//TiristPulseWith 1667 мкс = 30 градусов. 10000 - 180 град
    //126,//TirPWM_Pulse

    
}; 


#pragma section = ".bkfdsection"
const TFLASH_DATA BKFLASH_DATA @ ".bkfdsection" = 
{ 
    0x0401, //Modbus2_USB;
    //уставки работы
    //1667,//TiristPulseWith 1667 мкс = 30 градусов. 10000 - 180 град
    //126,//TirPWM_Pulse скважность

    
};

//TFLASH_DATA *settings = (TFLASH_DATA *) &FlashTmpBuffer;
