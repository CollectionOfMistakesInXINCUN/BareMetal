/*
 * Uncomment one of the following two lines, depending on the target,
 * if ps7/psu init source files are added in the source directory for
 * compiling example outside of IAR.
 */
#include "ps_init.h"
#include "fmsh_ps_parameters.h"
#include "fmsh_uart_lib.h"
#include "fmsh_common.h"
#include "platform/fmsh_print.h"

#ifdef STDOUT_IS_16550
 #include "fuartplns550_l.h"
 #define UART_BAUD 9600
#endif


u8 init_uart()
{
    u8 ret=FMSH_SUCCESS;
#ifdef STDOUT_IS_16550
    FUartPlNs550_SetBaud(STDOUT_BASEADDR, FPAR_FUARTPLNS550_CLOCK_HZ, UART_BAUD);
    FUartPlNs550_SetLineControlReg(STDOUT_BASEADDR, FUN_LCR_8_DATA_BITS);
#endif    /* Bootrom/BSP configures PS7/PSU UART to 115200 bps */
#ifdef STDOUT_BASEADDRESS  
    FUartPs_Config *Config=NULL;
    
    /*Initialize UARTs and set baud rate*/
    Config= FUartPs_LookupConfig(STDOUT_BASEADDRESS==FPS_UART0_BASEADDR?FPAR_UARTPS_0_DEVICE_ID:FPAR_UARTPS_1_DEVICE_ID);
    if(Config==NULL)
      return FMSH_FAILURE;
    ret=FUartPs_init(&g_UART, Config);
    if(ret!=FMSH_SUCCESS)
      return ret;
    
    FUartPs_setBaudRate(&g_UART, STDOUT_BASEADDRESS==FPS_UART0_BASEADDR?FPAR_UARTPS_0_BAUDRATE : FPAR_UARTPS_1_BAUDRATE);
    /*line settings*/
    FUartPs_setLineControl(&g_UART, Uart_line_8n1);
    
    /*enable FIFOs*/   
    FUartPs_enableFifos(&g_UART);
#endif
    return ret;
}

void init_platform()
{
    /*
     * If you want to run this example outside of IAR,
     * uncomment one of the following two lines and also #include "ps7_init.h"
     * or #include "ps7_init.h" at the top, depending on the target.
     * Make sure that the ps7/psu_init.c and ps7/psu_init.h files are included
     * along with this example source files for compilation.
     */
    u32 Status = PS_INIT_SUCCESS;
#if PS_PREINITED == 0
    Status = ps_init();
#endif	
    init_uart();
    if (Status != PS_INIT_SUCCESS) 
    {
      fmsh_print("PS7_INIT_FAIL!\n\r");
      while(1){};
    } 
    
#if defined(MMU_ENABLE) && (PS_PREINITED == 0) && defined(DDRPS_0_DEVICE_ID)   /* init DDR memory MMU table */
#if EL1_NONSECURE
    Fmsh_SetTlbAttributesRange(0, 0x40000000, NORM_WB_CACHE | NON_SECURE);
#else
    Fmsh_SetTlbAttributesRange(0, 0x40000000, NORM_WB_CACHE);
#endif
#endif    
}

void cleanup_platform()
{
}
