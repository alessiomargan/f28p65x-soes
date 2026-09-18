//###########################################################################
//
// FILE:   f28p65x_cpu1_system.h
//
// TITLE:  F28P65x CPU1 EtherCAT System APIs
//
// This file contains the function prototypes for any device system APIs that
// require modification for compatibility with EtherCAT stack usage.
//
//###########################################################################
// $TI Release: F28P65x EtherCAT Software v2.01.00.00 $
// $Release Date: August 31 2020 $
// $Copyright: Copyright (C) 2020 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//###########################################################################

#ifndef F28P65X_CPU1_SYSTEM_H
#define F28P65X_CPU1_SYSTEM_H

//
// Included Files
//
#include <string.h>

//
// Function Prototypes
//
void F28P65XCPU1_strcpy(void * __restrict dest, const void * __restrict src,
                       size_t n);
void F28P65XCPU1_memcpy(void * __restrict destination,
                       const void * __restrict source,
                       size_t length);
void F28P65XCPU1_memset(void *mem, int ch, size_t length);                       
int F28P65XCPU1_memcmp(const void *cs, const void *ct, size_t n);


#endif // F28P65X_CPU1_SYSTEM_H

//
// End of File
//
