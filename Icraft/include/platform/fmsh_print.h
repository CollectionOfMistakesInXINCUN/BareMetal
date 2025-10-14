/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file  fmsh_sd.h
*
* This file contains header fmsh_uart_lib.h
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   xx  11/23/2018  First Release
*</pre>
******************************************************************************/

#ifndef _FMSH_PRINT_H_
#define _FMSH_PRINT_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#include "string.h"
#include "fmsh_uart_lib.h"

  
/************************** Constant Definitions *****************************/  

/**************************** Type Definitions *******************************/
 
/***************** Macros (Inline Functions) Definitions *********************/  

/************************** Variable Definitions *****************************/
extern FUartPs_T g_UART;

/************************** Function Prototypes ******************************/
extern void fmsh_print(const char *ptr,...);
extern void uart_printf(const char *ptr,...);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of protection macro */

