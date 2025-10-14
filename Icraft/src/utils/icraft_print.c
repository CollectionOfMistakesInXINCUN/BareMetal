#include "utils/icraft_print.h"

void icraft_print_mem(void *addr, uint32_t size)
{   
    fmsh_print("printing mem(0x%llX)...\r\n", (uint64_t)addr);
    uint8_t *p = addr;
    for(uint32_t i = 0; i < size; ++i){
        fmsh_print("%02X ", ((char*)addr)[i]);
        if(i % 16 == 15){
            fmsh_print("\r\n");
        }
        if(i % 64 == 63){
            fmsh_print("----------\r\n");
        }
    }
    fmsh_print("print finish.\r\n");
}




















































void icraft_budda_bless(){
    fmsh_print("#####################################################\r\n");
    fmsh_print("#                                                     #\r\n"); 
    fmsh_print("#                       _oo0oo_                       #\r\n"); 
    fmsh_print("#                      o8888888o                      #\r\n"); 
    fmsh_print("#                      88\" . \"88                      #\r\n"); 
    fmsh_print("#                      (| -_- |)                      #\r\n"); 
    fmsh_print("#                      0\\  =  /0                      #\r\n"); 
    fmsh_print("#                    ___/`---'\\___                    #\r\n"); 
    fmsh_print("#                  .' \\\\|???? |#  '.                  #\r\n"); 
    fmsh_print("#                 / \\\\|||? :? |||#  \\                 #\r\n"); 
    fmsh_print("#                / _||||| -:- |||||- \\                #\r\n");  
    fmsh_print("#               |?? | \\\\\\\? -? #/ |??  |               #\r\n"); 
    fmsh_print("#               | \\_|? ''\\---/''? |_/ |               #\r\n"); 
    fmsh_print("#               \\\? .-\\__? '-'? ___/-. /               #\r\n");  
    fmsh_print("#             ___'. .'? /--.--\\\? `. .'___             #\r\n"); 
    fmsh_print("#            .\"\" '<? `.___\\_<|>_/___.' >'   "".         #\r\n"); 
    fmsh_print("#         | | :? `- \\`.;`\\ _ /`;.`/ - ` : | |         #\r\n");  
    fmsh_print("#         \\\? \\ `_.?? \\_ __\\ /__ _/?? .-` /? /         #\r\n");  
    fmsh_print("#     =====`-.____`.___ \\_____/___.-`___.-'=====      #\r\n");  
    fmsh_print("#                       `=---='                       #\r\n");  
    fmsh_print("#     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~     #\r\n");  
    fmsh_print("#                                                     #\r\n"); 
    fmsh_print("#              佛祖保佑         永无BUG               #\r\n");
    fmsh_print("#                                                     #\r\n");
    fmsh_print("#####################################################\r\n");
}