
#ifndef MODBUS_DATA_H
#define MODBUS_DATA_H
#include "bastypes.h"//содержит описания базовых типов

typedef struct {
    
        
    bavu16 MB_RS485_2;//1
    //уставки работы
    vu16 TiristPulseWith;//2 сколько шимить тиристором в мкс
    vu16 TirPWM_Pulse; // 3пустая

    
 } TFLASH_DATA;

///////////////////////////////////////////////////////////////////////////////

typedef struct  {
    
    u32 serial_number;//0,1
    bavu16 MB_RS485_1;//2
    u16 tdir_Modbus1_RS485;//3    
    u16 tdir_Modbus2_RS485;//4
    u16 lnk_mngr_t_TO;//5
//    u16 lnk_mngr_t_ChSl;//6
//    u16 lnk_mngr_t_DIR;//7
  //  vu16 Z; //7 пустая 

    vf32 O_Uin_A; //89
    vf32 O_Uin_B; //ab
    vf32 O_Uin_C; //cd
    vf32 O_Uout_A; //ef
    vf32 O_Uout_B; //1011
    vf32 O_Uout_C; //1213
    vf32 O_Ia; //1415
    vf32 O_Ib;
    vf32 O_Ic;
    vf32 O_Iload;
    vf32 O_Uload;
       
    vf32 K_Uin_A;//1c1d
    vf32 K_Uin_B;//1819 
    vf32 K_Uin_C;//1a1b 
    vf32 K_Uout_A;//1e1f
    vf32 K_Uout_B;//1c1d
    vf32 K_Uout_C;//2021
    vf32 K_Ia;//22,23 
    vf32 K_Ib;//2425
    vf32 K_Ic;
    vf32 K_Iload;
    vf32 K_Uload;
    
 } TCD_DATA;
///////////////////////////////////////////////////////////////////////////////


typedef union {
			  vu32	   i;
			  struct {
			    //флаги железных неполадок декса r0000
			    unsigned flash_error : 1;	//0 1
			    unsigned backup_error : 1;	//1 2
			    unsigned fram_error : 1;	//2 4 
			    unsigned fram_bkp_error : 1;	//3 8		    
			    unsigned syncfail : 1;	//4 16 ошибка синхронизации
			    unsigned d01 : 1;	//5 32
			    unsigned d00 : 1;	//6 64
			    unsigned d0 : 1;	//7 128
			    unsigned d1 : 1;	//8 1
			    unsigned d2 : 1;	//9 2
			    unsigned d3 : 1;	//а 4
			    unsigned d4 : 1;	//b 8  
			    unsigned d5 : 1;    //с 16
			    unsigned d6 : 1;    //d 32
			    unsigned d7 : 1;    //e 64  
                            unsigned d8 : 1;    //f 128
                            //r0001
                            unsigned d9 : 1;
                            unsigned d10 : 1;  
			    unsigned d11 : 1; 
                            unsigned d12 : 1;
			    unsigned d13 : 1;          
			    unsigned d14 : 1;          
			    unsigned d15 : 1;
                            unsigned d16 : 1;   
			    unsigned d17 : 1;
			    unsigned d18 : 1;        
                            unsigned d19 : 1;         
			    unsigned d20 : 1;       
			    unsigned d21 : 1;         
			    unsigned d22 : 1;      
			    unsigned d23 : 1;        
			    unsigned LED_RED : 1;  //f 128      		    
                            
			  } BA;
			} _FLG0;//флаги управления и индикации;

//параметры расположеные в RAM
typedef  struct {
    _FLG0 FLAGS;    //00,01  
    u16 DI_reg; //02 l
    u16 DO_reg; //03 h
    
    vf32 Uin_A;
    vf32 Uin_B; 
    vf32 Uin_C; 
    vf32 Uout_A; 
    vf32 Uout_B; 
    vf32 Uout_C; 
    vf32 Ia; 
    vf32 Ib; 
    vf32 Ic; 
    vf32 Iload;
    vf32 Uload;
    u16 U_REF;
    
} TRAM_DATA;
///////////////////////////////////////////////////////////////////////////////
typedef struct
{
  u16 test_mass[128];
} TSRAM_DATA;

#define uart_buff_size 0x0ff // buffer size;
#endif