/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_print.c
*
* This file contains 
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   wfb  11/23/2018  First Release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include <stdio.h>
#include "fmsh_print.h"
#include "fmsh_ps_parameters.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/
FUartPs_T g_UART;
static void SendData(FUartPs_T *dev,char *UartBuffer,u32 NumBytes);
void fmsh_print(const char *ptr,...) ;

void fmsh_print(const char *ptr,...)                               
{
#ifdef STDOUT_BASEADDRESS
  va_list ap;
  char string[256];
  va_start(ap,ptr);
  int cnt;
  cnt = vsnprintf(string,256,ptr,ap);
  SendData(&g_UART,string,strlen(string));  	
  va_end(ap);
#else
    printf(ptr);
#endif
    return ;
}

static void SendData(FUartPs_T *dev,char *UartBuffer,u32 NumBytes)
{
  for(u16 i=0; i<NumBytes; i++) {
    FUartPs_write(dev,*UartBuffer);
    while((FUartPs_getLineStatus(dev) & Uart_line_thre) != Uart_line_thre);
      UartBuffer++;
  }
}

