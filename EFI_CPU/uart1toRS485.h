#ifndef UART1TORS485_H
#define UART1TORS485_H

#include "bastypes.h"

  #ifdef __cplusplus
extern "C" {
#endif
    void uart1to485_init (void);
    void uart1to485_ReInit (void);
    u16  U1_SwCNT (void);
    void U1_Timer (void);
    void Tx1DMA (void);//настройка DMA на передачу данных в UART1

#ifdef __cplusplus
}
#endif
#endif

