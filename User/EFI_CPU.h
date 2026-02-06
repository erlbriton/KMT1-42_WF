

void EFICPUdataInit(void);
void EFICPUprocess(void);

extern filter_struct Filter_Uin_A; //структура параметров для полосового фильтра фаза А вход
extern filter_struct Filter_Uin_B; //структура параметров для полосового фильтра фаза B вход
extern filter_struct Filter_Uin_C; //структура параметров для полосового фильтра фаза С вход
extern filter_struct Filter_Uout_A; //структура параметров для полосового фильтра фаза А выход
extern filter_struct Filter_Uout_B; //структура параметров для полосового фильтра фаза B выход
extern filter_struct Filter_Uout_C; //структура параметров для полосового фильтра фаза С выход
extern filter_struct Filter_Ia; //структура параметров для полосового фильтра фаза A ток
extern filter_struct Filter_Ib; //структура параметров для полосового фильтра фаза B ток
extern filter_struct Filter_Ic; //структура параметров для полосового фильтра фаза C ток

extern u8 DI_stat; //в кем из вх вых работаем в данный момент
extern u8 DO_stat;
