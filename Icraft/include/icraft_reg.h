#ifndef ICRAFT_REG_H
#define ICRAFT_REG_H

// Base
#define FPGA_REG_BASE      0x00000000
#define BUYI_REG_BASE      0x00040000
#define MMU_REG_BASE       DMA_GP1_REL_BASE + 0x2000
#define DETPOST_REG_BASE   DMA_GP1_REL_BASE + 0x800
#define HARDOP_FPGA_REG_BASE DMA_GP1_REL_BASE + 0x1400
#define CAST_REG_BASE       DMA_GP1_REL_BASE + 0x1000
#define WARPAFFINE_REG_BASE DMA_GP1_REL_BASE + 0x2800
#define SEGPOST_REG_BASE    DMA_GP1_REL_BASE + 0X2400


// HOST_CTRL
#define DEBUG_REG          0x000   + FPGA_REG_BASE
#define DDR_STATE_CLR_REQ  0x004   + FPGA_REG_BASE
#define DDR_STATE          0x008   + FPGA_REG_BASE
#define AWLAYER_CNT        0x00C   + FPGA_REG_BASE
#define AWTILE_CNT         0x010   + FPGA_REG_BASE
#define ARTILE_CNT         0x014   + FPGA_REG_BASE
#define RUN_TIMER_CNT      0x018   + FPGA_REG_BASE
#define PRJ_VER            0x01C   + FPGA_REG_BASE
#define ITF_EXHS_VLD       0x020   + FPGA_REG_BASE
#define ITF_EXHS           0x024   + FPGA_REG_BASE
#define ITF_EXHS_ACK_REQ   0x028   + FPGA_REG_BASE
#define VMA_OFFSET         0x038   + FPGA_REG_BASE
#define DR_STATUS          0x20004 + FPGA_REG_BASE   ///> 频率状态寄存器
#define DR_CFG0            0x20200 + FPGA_REG_BASE   //
#define DR_CFG2            0x20208 + FPGA_REG_BASE
#define DR_CFG23           0x2025c + FPGA_REG_BASE

// BUYI REG
#define RESET_REG          0x1DC + BUYI_REG_BASE
#define RUN_RESUME         0x004 + BUYI_REG_BASE
#define RUN_BASE           0x008 + BUYI_REG_BASE
#define RUN_SIZE           0x00C + BUYI_REG_BASE
#define RUN_VALID          0x010 + BUYI_REG_BASE
#define BUYI_DEBUG_REG     0x3C  + BUYI_REG_BASE
#define RA_IDMA_CNT        0x40  + BUYI_REG_BASE
#define RA_TIMER           0x44  + BUYI_REG_BASE
#define RA_DUMMY_FLAG      0x48  + BUYI_REG_BASE
#define BYAI_VERSION1      0x4C  + BUYI_REG_BASE
#define BYAI_VERSION2      0x50  + BUYI_REG_BASE
#define BYAI_VERSION3      0x54  + BUYI_REG_BASE
#define INST_BIST_STATUS   0x5C  + BUYI_REG_BASE
#define BIST_ABNORMAL      0x60  + BUYI_REG_BASE
#define OCM_CLK_EN_B       0x14  + BUYI_REG_BASE
#define OCM_BIST_ST_SEL    0x18  + BUYI_REG_BASE
#define OCM_BIST_STATUS    0x64  + BUYI_REG_BASE
#define STATIC_OCM_ERR     0x68  + BUYI_REG_BASE
#define COL_ROW_EN         0x6C  + BUYI_REG_BASE

#define CLK_REG            0x4000002C
#define ICORE_CLK_FREQ     860E6 //860 M

#define GP0_BOTTOM         0x40000000
#define GP0_TOP            0x80000000 - 1
#define GP0_DEFAULT_BASE   0x40000000

#define GP1_BOTTOM         0x80000000
#define GP1_TOP            0xC0000000 - 1
#define GP1_DEFAULT_BASE   0x80000000

#define MEM_MAP_SIZE       0x1000000    ///> 16M
#define DMA_SWAP_SIZE      0x1000000    ///> 16M

#define HP0_DMA_BL         64
#define DDR_DMA_BL         64

#define DMA_GP1_REL_BASE   0x100000000
#define HOST_CMD_ADDR      0x04 + DMA_GP1_REL_BASE
#define HP0_RDMA_FROM_ADDR 0x08 + DMA_GP1_REL_BASE
#define HP0_RDMA_TO_ADDR   0x0C + DMA_GP1_REL_BASE
#define HP0_WDMA_FROM_ADDR 0x10 + DMA_GP1_REL_BASE
#define HP0_WDMA_TO_ADDR   0x14 + DMA_GP1_REL_BASE
#define DDR_RDMA_FROM_ADDR 0x18 + DMA_GP1_REL_BASE
#define DDR_RDMA_TO_ADDR   0x1C + DMA_GP1_REL_BASE
#define DDR_WDMA_FROM_ADDR 0x20 + DMA_GP1_REL_BASE
#define DDR_WDMA_TO_ADDR   0x24 + DMA_GP1_REL_BASE

#define DEV_STATE_ADDR     0x80 + DMA_GP1_REL_BASE
#define DMA_STATE_ADDR     0x84 + DMA_GP1_REL_BASE
#define HP_RDMA_NUM        0x88 + DMA_GP1_REL_BASE
#define HP_WDMA_NUM        0x8C + DMA_GP1_REL_BASE
#define DDR_RDMA_NUM       0x90 + DMA_GP1_REL_BASE
#define DDR_WDMA_NUM       0x94 + DMA_GP1_REL_BASE

#define HP0_2_DDR_CMD      0x1
#define DDR_2_HP0_CMD      0x2

#define DMA_ERROR_MASK     0x44AA
#define DMA_ERROR_EXPECT   0x0000
#define DMA_STATE_MASK     0xFFFF
#define DMA_STATE_EXPECT   0x2200
#define DEV_STATE_MASK	   0x7
#define DEV_STATE_EXPECT   0x7

#define FPGA_RESET_REG     0x34
#define BUYI_RESET_REG     0x1DC
#define VERSION_REG        0x1C

#define MPE_REG            0x0000002C

//MMU_REG
#define MMU_GP_START       0x004 + MMU_REG_BASE  //01bit, start mmu convert
#define MMU_CONVERT_DONE   0x008 + MMU_REG_BASE  //01bit, hw set 0, sw set 1
#define MMU_VERSION        0x00C + MMU_REG_BASE  //32bit, get MMU version
#define MMU_TEST_REG       0x010 + MMU_REG_BASE  //32bit, TEST REG for debug
#define MMU_TIME_CNT       0x014 + MMU_REG_BASE  //32bit, time counter
#define MMU_SOFT_RESET     0x018 + MMU_REG_BASE  //01bit, set 1 to reset MMU
#define MMU_MODE           0x01C + MMU_REG_BASE  //02bit, mode 0 disable , mode 1 activate
#define MMU_LOG_BASE0      0x020 + MMU_REG_BASE  //0x020~0x09C + 4byte area, LogiSegTable0, 32*32bit logiical base, low to high
#define MMU_LOG_BASE1      0x0A0 + MMU_REG_BASE  //0x0A0~0x11C + 4byte area, LogiSegTable1, 32*32bit logiical base, low to high
#define MMU_PHY_BASE0      0x120 + MMU_REG_BASE  //0x120~0x19C + 4byte area, PhySegTable0, 32*32bit (physical base - logiical base), low to high
#define MMU_PHY_BASE1      0x1A0 + MMU_REG_BASE  //0x1A0~0x21C + 4byte area, PhySegTable1, 32*32bit (physical base - logiical base), low to high
#define MMU_SIZE  32

// DETPOST REG
#define DETPOST_VERSION    0x068 + DETPOST_REG_BASE


#endif // ICRAFT_REG_H