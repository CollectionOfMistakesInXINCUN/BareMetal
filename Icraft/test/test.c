#include <stddef.h>

#include "ff.h"
#include "fmsh_dmac_lib.h"
#include "fmsh_ps_parameters.h"
#include "fmsh_gic.h"
#include "fmsh_gic_hw.h"
#include "fmsh_sdmmc.h"

#include "icraft_device.h"
#include "icraft_ftmp.h"
#include "utils/icraft_print.h"


#define CONFIG_VOLUMES_NUM (4) 

FATFS fatfs_s[FF_VOLUMES];
BYTE f_mkfsBuff[FF_VOLUMES][FF_MAX_SS];
BYTE work[FF_MAX_SS] __attribute__ ((aligned (64)));  /* Working buffer */

PARTITION VolToPart[FF_VOLUMES] = {
    {0, 1},    /* "0:" ==> 1st partition in PD#0 */
    {0, 2},    /* "1:" ==> 2nd partition in PD#0 */
    {0, 3},    /* "2:" ==> 3nd partition in PD#0 */
    {0, 4},    /* "3:" ==> 4th partition in PD#0 */
    {1, 1},    /* "4:" ==> 1st partition in PD#1 */
    {1, 2},    /* "5:" ==> 2nd partition in PD#1 */
    {1, 3},    /* "6:" ==> 3nd partition in PD#1 */
    {1, 4}     /* "7:" ==> 4th partition in PD#1 */    
};

FDmaPs_T g_DMA_dmac;
FDmaPs_Param_T g_DMA_param;
FDmaPs_Instance_T g_DMA_instance;

void FDmaPs_IRQ (void *InstancePtr)
{
	FDmaPs_irqHandler((FDmaPs_T *)InstancePtr);
}


FRESULT show_partition_usage(u32 ulPhyDriveNo,u32 ulPartitionNum)
{
	FRESULT Res = FR_OK;
	TCHAR *Path[] = {"0:","1:","2:","3:","4:","5:","6:","7:"};/*Logical drive number */
	u32 i;
	DWORD fre_clust, fre_sect, tot_sect;
	FATFS *fs;

	for(i=0;i < ulPartitionNum;i++)
	{	
		/* Get total sectors and free sectors */
		Res = f_getfree(Path[i+4*ulPhyDriveNo], &fre_clust, &fs);
		if (Res)
		{
		  fmsh_print("f_getfree err[%d]!\r\n",Res);
		  continue;
		}
		tot_sect = (fs->n_fatent - 2) * fs->csize;
		fre_sect = fre_clust * fs->csize;
		fmsh_print("partition [%d] :%10lu KiB total space.%10lu KiB available.\r\n", i,tot_sect/2, fre_sect/2);/*sector size = 512 bytes*/
	}
	return Res;
}


/****************************************************************************************************************
* u32 ulPhyDriveNo---physical drive number : 0 or 1
* u32 ulPartitionNum----logical partition number of a physical drive:1,2,3 or 4
* DWORD plist[]----List of partition size to create on the physical drive eg:
              {50, 50, 0, 0};  Divide the drive into two equal partitions 
              {0x10000000,50,50,0}; 256M sectors for 1st partition and 50% of the left for 2nd partition and 3nd partition each
              {20, 20, 20, 0}; 20% for 3 partitions each and remaing space is left not allocated
              {25, 25, 25, 25}; 25% for 4 partitions each
              {100, 0, 0, 0}; only one partition,all allocated to this partition
*****************************************************************************************************************/
u32 fmsh_SdEmmcInitPartFAT32(u32 ulPhyDriveNo,u32 ulPartitionNum,DWORD plist[])
{
	FRESULT Res = FR_OK;
	TCHAR *Path[] = {"0:","1:","2:","3:","4:","5:","6:","7:"}; /*Logical drive number */
	u32 i;

	if(CONFIG_VOLUMES_NUM < ulPartitionNum)
	{
		return FMSH_FAILURE;  
	}

	if(SDMMCPS_0_DEVICE_ID == ulPhyDriveNo)
	{
		FDmaPs_T *pDmac = &g_DMA_dmac;
		FDmaPs_Instance_T *pInstance = &g_DMA_instance;
		FDmaPs_Param_T *pParam = &g_DMA_param;
		FDmaPs_Config *pDmaCfg;
		s32 Status;

		FGicPs_SetupInterruptSystem(&IntcInstance);
		FMSH_ExceptionRegisterHandler(FMSH_EXCEPTION_ID_IRQ_INT,(FMSH_ExceptionHandler)FGicPs_InterruptHandler_IRQ,&IntcInstance); 

		/*Initialize the DMA Driver */
		pDmaCfg = FDmaPs_LookupConfig(FPAR_DMAPS_DEVICE_ID);
		if (pDmaCfg == NULL) {
		  return FMSH_FAILURE;
		}
		FDmaPs_initDev(pDmac, pInstance, pParam, pDmaCfg);
		Status = FGicPs_registerInt(&IntcInstance, DMA_INT_ID,
				(FMSH_InterruptHandler)FDmaPs_IRQ, pDmac);
		if (Status != FMSH_SUCCESS)
		{
		  return FMSH_FAILURE;
		}
		Status = FDmaPs_autoCompParams(pDmac);
		if (Status != FMSH_SUCCESS)
		{
		  return FMSH_FAILURE;
		}

		set_sdmmc_workmode(sdmmc_trans_mode_dw_dma);
	}

	for(i=0;i < ulPartitionNum;i++)
	{
		/* Try to mount FAT file system */
		Res |= f_mount(&fatfs_s[i+4*ulPhyDriveNo], Path[i+4*ulPhyDriveNo], 1);
	}
	if (Res != FR_OK) 
	{        
		fmsh_print("Volume is not FAT formatted, formatting FAT32,please waiting......\r\n");
		Res = f_fdisk(ulPhyDriveNo, plist, work);  /* Divide physical drive */
		if (Res != FR_OK) 
		{
		  fmsh_print("f_fdisk err[%d]!\r\n",Res);
		  return FMSH_FAILURE;
		}
		else
		{
		  fmsh_print("f_fdisk OK!\r\n");
		}

		for(i=0;i < ulPartitionNum;i++)
		{
		  /*make FAT32 fs for each partition*/
		  Res = f_mkfs(Path[i+4*ulPhyDriveNo], FM_FAT32, 0, f_mkfsBuff[i+4*ulPhyDriveNo], sizeof(f_mkfsBuff[i+4*ulPhyDriveNo]));
		  if (Res != FR_OK) 
		  {
		    fmsh_print("Unable to format FATfs[%d],err[%d]\r\n",i,Res);
		    return FMSH_FAILURE;
		  }
		  else
		  {
			fmsh_print("partition[%d],mkfs FAT32 OK!\r\n",i);
		  }

		  Res = f_mount(&fatfs_s[i+4*ulPhyDriveNo], Path[i+4*ulPhyDriveNo], 1);
		  if (Res != FR_OK) 
		  {
		    fmsh_print("Unable to mount FATfs[%d],err[%d]\r\n",i,Res);
		    return FMSH_FAILURE;
		  }
		  else
		  {
			fmsh_print("partition[%d],mount FAT32 OK!\r\n",i);
		  }
		}
	}

	(void)show_partition_usage(ulPhyDriveNo,ulPartitionNum);
	fmsh_print("File system initialization successful\r\n");
	return FMSH_SUCCESS;
}

void test_hashtable();
void test_list();
void test_device_open();
void test_device_dma();
void test_device_get_layercount();
void test_device_set_dtype();
void test_device_enable_mpe();
void test_ff_load_file();
void test_unpack_network();
void test_ff_write_file();
void test_ff_mkdir();
void test_ff_listdir();
void test_two_networks_forward();
void test_queue();
void test_create_network();
void test_ff_bad_case();
void test_gic_single();
void test_gic_two_networks();
void network_test();
void test_free();

int main(){
    /**初始化平台**/
 	init_platform();
// 	/**初始化Cache**/
 	Fmsh_ICacheEnable();
 #if PSOC_DCACHE_ENABLE==1
 	Fmsh_DCacheEnable();
 #endif

   DWORD plist_only_one[] = {100, 0, 0, 0};
   u32 ulret = FMSH_SUCCESS;
   ulret = fmsh_SdEmmcInitPartFAT32(SDMMCPS_0_DEVICE_ID, 1, plist_only_one);

    //network_test();
    //test_network_forward();
    test_gic_single();
    //test_gic_two_networks();
    //test_free();
}