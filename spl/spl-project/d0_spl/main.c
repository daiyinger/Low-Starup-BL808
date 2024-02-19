#include "bflb_mtimer.h"
#include "board.h"
#include "bl808_glb.h"
#include "bflb_flash.h"
#include "bflb_l1c.h"
#include "rv_pmp.h"

#define     D0_FLASH_BASIC_ADDR         0x100000

#define     DTB_SRC_ADDR                0x110000
#define     DTB_DST_ADDR 		0x51ff8000

#define     OPENSBI_SRC_ADDR            0x120000
#define     OPENSBI_DST_ADDR		0x53F00000

#define     KERNEL_SRC_ADDR             0x130000
#define     KERNEL_DST_ADDR 		0x50000000

#define     ROOTFS_SRC_ADDR 		0x580000
#define     ROOTFS_DST_ADDR 		0x52000000

#define     COMPRESS_FLAG_SIZE          1
#define     KERNEL_LEN_SIZE             4

#if (__riscv_xlen == 64)
/* linux pmp setting */
const pmp_config_entry_t pmp_entry_tab[9] = {
    [0] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x20000000,
        .entry_pa_length = 0x10000,
    },

    [1] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x30000000,
        .entry_pa_length = PMP_REG_SZ_1M,
    },

    [2] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_X | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x3eff0000,
        .entry_pa_length = 0x10000,
    },

    [3] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_X | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x40000000,
        .entry_pa_length = PMP_REG_SZ_16K,
    },
    
    [4] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_X | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x50000000,
        .entry_pa_length = 0x4000000,
    },

    [5] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x58000000,
        .entry_pa_length = 0x4000000,
    },

    [6] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0xe0000000,
        .entry_pa_length = 0x8000000,
    },
    
    [7] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_X | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x3F000000,
        .entry_pa_length = 0x10000,
    },

    [8] = {
        .entry_flag = ENTRY_FLAG_ADDR_TOR,
        .entry_pa_base = 0xffffffffff, /* 40-bit PA */
        .entry_pa_length = 0,
    },
};

#endif

static int load_image(void)
{
    uint32_t flash_addr;
    uint8_t *read_buf;


    uint32_t header_kernel_len = 0;
    uint32_t rootfs_len = 0;
    uint8_t kernel[KERNEL_LEN_SIZE];
    uint8_t compress_flag;

    //printf("load start\r\n");
    flash_addr = KERNEL_SRC_ADDR - (COMPRESS_FLAG_SIZE + KERNEL_LEN_SIZE) - D0_FLASH_BASIC_ADDR;
    read_buf = (uint8_t *)(FLASH_XIP_BASE + flash_addr);
    bflb_l1c_dcache_invalidate_range(read_buf, COMPRESS_FLAG_SIZE + KERNEL_LEN_SIZE);
    
    memcpy(kernel, read_buf + 1, 4);

    if(kernel[3] == 0xff && kernel[2] == 0xff && kernel[1] == 0xff && kernel[0] ==0xff)
    {
        printf("not found kernel file...\r\n");
        return -1;
    }
    else
    {
        header_kernel_len = kernel[3];
        header_kernel_len = header_kernel_len << 8 | kernel[2];
        header_kernel_len = header_kernel_len << 8 | kernel[1];
        header_kernel_len = header_kernel_len << 8 | kernel[0];

        __NOP_BARRIER();

        compress_flag = read_buf[0];
        if (compress_flag != 0)
        {
            flash_addr = KERNEL_SRC_ADDR - D0_FLASH_BASIC_ADDR;
            read_buf = (uint8_t *)(FLASH_XIP_BASE + flash_addr);
            bflb_l1c_dcache_invalidate_range(read_buf, header_kernel_len);
            unlz4((const void *)read_buf, (void *)KERNEL_DST_ADDR, header_kernel_len);
	}
        else 
        {
            flash_addr = KERNEL_SRC_ADDR - D0_FLASH_BASIC_ADDR;
            read_buf = (uint8_t *)(FLASH_XIP_BASE + flash_addr);
            bflb_l1c_dcache_invalidate_range(read_buf, header_kernel_len);
            memcpy((void *)KERNEL_DST_ADDR, read_buf, header_kernel_len);
        }
    }
    

    //copy dtb code
    flash_addr = DTB_SRC_ADDR - D0_FLASH_BASIC_ADDR;
    read_buf = (uint8_t *)(FLASH_XIP_BASE + flash_addr);
    bflb_l1c_dcache_invalidate_range(read_buf, 0x10000);
    memcpy((void *)DTB_DST_ADDR, read_buf, 0x10000);


    flash_addr = ROOTFS_SRC_ADDR - D0_FLASH_BASIC_ADDR - 0x04;
    read_buf = (uint8_t *)(FLASH_XIP_BASE + flash_addr);
    bflb_l1c_dcache_invalidate_range(read_buf, 4);
    rootfs_len = *((uint32_t *)read_buf);


    flash_addr = ROOTFS_SRC_ADDR - D0_FLASH_BASIC_ADDR;
    read_buf = (uint8_t *)(FLASH_XIP_BASE + flash_addr);
    bflb_l1c_dcache_invalidate_range(read_buf, rootfs_len);
    unlz4((const void *)read_buf, (void *)ROOTFS_DST_ADDR, rootfs_len);


    /* Copy opensbi code */
    flash_addr = OPENSBI_SRC_ADDR - D0_FLASH_BASIC_ADDR;
    read_buf = (uint8_t *)(FLASH_XIP_BASE + flash_addr);

    bflb_l1c_dcache_invalidate_range(read_buf, 0xc000);
    memcpy((void *)OPENSBI_DST_ADDR, read_buf, 0xc000);

    csi_dcache_clean_invalid();

    return 0;
}

int main(void)
{
    board_init();

    if (load_image() < 0)
    {
        printf("load image failed...\n");
        while(1)
            bflb_mtimer_delay_ms(1000);
    }


    __ASM volatile("csrw mcor, %0"
                   :
                   : "r"(0x30013));

    rvpmp_init(pmp_entry_tab, sizeof(pmp_entry_tab) / sizeof(pmp_config_entry_t));
    
    __ISB();


    void (*opensbi)(int hart_id, int fdt_addr) = (void (*)(int hart_id, int fdt_addr))OPENSBI_DST_ADDR;
    
    /* go to run kernel ... */
    opensbi(0, DTB_DST_ADDR);

    while (1) 
    {
        bflb_mtimer_delay_ms(1000);
    }
}
