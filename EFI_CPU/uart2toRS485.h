#ifndef UART2TO485_H
#define UART2TO485_H

#ifdef __cplusplus
extern "C" {
#endif
#include "bastypes.h"

  
    extern void uart2to485_init (void);
    extern void uart2to485_ReInit (void);
    extern u16  U2_SwCNT (void);
    extern void U2_Timer (void);

#ifdef __cplusplus
}
#endif

#endif

