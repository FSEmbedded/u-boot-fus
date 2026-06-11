# NEW NBOOT LAYOUT

With the new i.MX processors, such as i.MX8ULP, i.MX91/93, the NBOOT.fs and UBOOT.fs images have adopted a new image concept based on the IMX_CONTAINER. Several implementations of this affect BL2 (SPL) and BL33 (U-Boot).

## Files and Directories

All implementations of the new NBOOT are limited to the board files located under:

`board/F+S/<BOARD>/`

`include/configs/<BOARD>.h`

and the firmware directory:

`board/F+S/NXP-Firmware/`

The firmware directory contains fully compiled binary data required for the various NBOOT images. However, the type and number of binaries depend on the architecture used.

### Common Files for All `<BOARD>`:

- `bl31.bin` (ATF)
- `bl32.bin` (OpTEE) (optional)
- `mcore_image.bin` (Cortex-M) (optional)

Some architectures may require additional firmware, data, etc. (e.g., DRAM-FW), which can also be placed here.

The `board/F+S/<BOARD>/` directory contains the board-specific `.c` files, configuration scripts, and the NBOOT build directory.

### Important Files in the Board Directory:

- `<BOARD>.c`:  
    Board-specific code for U-Boot (BL33).
  
- `<BOARD>.h`:  
    Definitions that can be used in `spl.c` or `<BOARD>.c`.
  
- `spl.c`:  
    SPL code for U-Boot SPL.
  
- `Makefile`:  
    Build script for this `<BOARD>`. Additional targets for building the UBOOT-INFO images are defined here.
  
- `lpddr<X>_type.cfg`:  
    Lists all board-specific timing files (`timing.fs`) that should be packed into an IMX_CONTAINER.
  
- `Kconfig`:  
    Defines specific configurations. Also, it defines the `.cfg` files to be used for UBOOT-INFO and BOOT-INFO. Other `.cfg` file assignments can be made via `menuconfig`.
  
- `boot-info.cfg`:  
    Lists all board-specific binaries that should be packed into a BOOT-CONTAINER.
  
- `board-info.cfg`:  
    Lists all BOARD-CFGs that should be packed into a BOARD-INFO container.
  
- `bl31-bl33.cfg` (default):  
    Lists all BL3<x> binaries that should be loaded after U-Boot SPL. These binaries are packed into a UBOOT-INFO container.
  
- `bl31-bl32-bl33.cfg`:  
    Similar to `bl31-bl33.cfg`, but includes BL32 (OpTEE). This can be assigned via `menuconfig` (FUS_UBOOTCNTR_CONFIG).

### Files in `board/F+S/<BOARD>/nboot/`:

- `dram-timings.lds`:  
    Linker script for all DRAM-TIMING structs.
  
- `<dram_type>_<board>_<size>g_<mt/s>m_<ch>ch_<cs>cs_timing.c`:  
    DRAM timings.
  
- `Makefile`:  
    Build script for NBOOT.
  
- `<BOARD>-FERT<x>.dts`:  
    BOARD-CFG.

## SPL Implementation

U-Boot SPL provides a MULTI-DTB configuration. See `README.multi-dtb-fit` for more details. The following configurations must be considered:

- `CONFIG_OF_LIST`:  
    A list of all U-Boot device trees.
  
- `CONFIG_MULTI_DTB_FIT`:  
    Enables MULTI-DTB for U-Boot.
  
- `CONFIG_SPL_MULTI_DTB_FIT`:  
    Enables MULTI-DTB for SPL.
  
- `CONFIG_SPL_MULTI_DTB_FIT_USER_DEFINED_AREA`:  
    Set `CONFIG_SPL_MULTI_DTB_FIT_USER_DEF_ADDR=<addr-in-ocram>`.
  
- `CONFIG_BOARD_TYPES`:  
    Adds the `board_types` attribute to GLOBAL_DATA.

In `spl.c`, the following function calls should be defined in the `board_init_f()` function, in the required order:

```c
void board_init_f(ulong dummy)
{
    /* Pre INIT Code */
    . . . 

    /* Setup default Device Tree */
    spl_early_init();

    /* Load Board ID to identify the board in the early state */
    fs_cntr_load_board_id();

    /* Setup Multiple Device Tree */
    board_early_init_f();

    regulators_enable_boot_on(false);
    
    preloader_console_init();

    print_bootstage();

    print_devinfo();

    /* Probe MU for ELE-API */ 
    . . .

    /* Clock and Power INIT  */
    . . .

    /* Load F&S NBOOT-Images */
    fs_cntr_init(true);

    /* Setup TRDC for DDR access */
    trdc_init();
    
    /* DDR initialization */
    spl_dram_init();

    /* Remainder of board_init_f() */
    . . . 
}
```

### Function Details:

- `spl_early_init()`:   
Must be called early. This initializes the driver model stack with a default device tree (first in OF_LIST). Flash accesses via boot ROM can only be realized after this function call.

- `fs_cntr_load_board_id()`:    
    Loads BOARD-ID via boot ROM.

- `board_early_init_f():`   
    This function must be defined in spl.c and is board-specific. It performs three relevant tasks: determines the board type, sets up UART PINMUX and UART CLK, and reinitializes the DM stack with the correct device tree.

    Example from `fsimx93`:
    ```c
    int board_early_init_f(void)
    {
        int rescan = 0;

        set_gd_board_type();

        switch(gd->board_type) {
            case BT_PICOCOREMX93:
                imx_iomux_v3_setup_multiple_pads(lpuart2_pads, ARRAY_SIZE(lpuart2_pads));
                init_uart_clk(LPUART2_CLK_ROOT);
                break;
            case BT_OSMSFMX93:
                imx_iomux_v3_setup_multiple_pads(lpuart1_pads, ARRAY_SIZE(lpuart1_pads));
                init_uart_clk(LPUART1_CLK_ROOT);
                break;
            default:
                return -EINVAL;
                break;
        }

        fdtdec_resetup(&rescan);

        if (rescan) {
            dm_uninit();
            dm_init_and_scan(!CONFIG_IS_ENABLED(OF_PLATDATA));
        }

        return 0;
    }
    ```

    set_gd_board_type() is defined as follows:
    ```c
    static int set_gd_board_type(void)
    {
        const char *board_id;
        const char *ptr;
        int len;

        board_id = fs_image_get_board_id();
        ptr = strchr(board_id, '-');
        len = (int)(ptr - board_id);

        SET_BOARD_TYPE("PCoreMX93", BT_PICOCOREMX93, board_id, len);
        SET_BOARD_TYPE("OSMSFMX93", BT_OSMSFMX93, board_id, len);

        return -EINVAL;
    }
    ```

- `regulators_enable_boot_on()`:    
    Activates all regulators in the device tree with the `regulator-boot-on` property.

- `preloader_console_init()`:   
    Initializes the SPL console.

- `print_bootstage()`:  
    Uses boot ROM to determine the current boot stage and prints the information to the console.

- `print_devinfo()`:    
    Uses boot ROM to determine the current boot device and prints the information to the console.

- `fs_cntr_init(true)`:     
    Loads the remaining NBOOT files from Flash.

- `spl_dram_init()`:    
    Board-specific DRAM initialization. Calls ddr_init() to set up DRAM, and dram_init() to set up `gd`. This is important for ELE-API! Also, please define CFG_SPL_FUS_EARLY_AHAB_BASE in `include/configs/<BOARD>.h`. CFG_SPL_FUS_EARLY_AHAB_BASE is needed to define a ELE_AHAB_BASE_ADDRESS, befor dram is initialized.
    ```c
    void spl_dram_init(void)
    {
        struct dram_timing_info *dtiming = _dram_timing;

        printf("DDR: %uMTS\n", dtiming->fsp_msg[0].drate);
        ddr_init(dtiming);

        /* Save RAM info in gd */
        dram_init();
    }
    ```

`spl.c` must define `fs_board_init_dram_data()` to create a reference to the DRAM timing struct `_dram_timing`. The function is used in `fs_cntr_common.c.`

For SPL to find its correct device tree, the following function is required:
```c
#if CONFIG_IS_ENABLED(MULTI_DTB_FIT)
int board_fit_config_name_match(const char *name)
{
    CHECK_BOARD_TYPE_AND_NAME("picocoremx93", BT_PICOCOREMX93, name);
    CHECK_BOARD_TYPE_AND_NAME("fs-osm-sf-mx93-adp-osm-bb", BT_OSMSFMX93, name);

    return -EINVAL;
}
#endif
```

## U-Boot Implementation
Similar to `spl.c`, the `<BOARD>.c` file also defines set_gd_board_type(). This function sets the board type in GLOBAL_DATA:
```c
static int set_gd_board_type(void)
{
    struct fs_header_v1_0 *cfg_fsh;
    const char *board_id;
    const char *ptr;
    int len;

    cfg_fsh = fs_image_get_regular_cfg_addr();
    board_id = cfg_fsh->param.descr;
    ptr = strchr(board_id, '-');
    len = (int)(ptr - board_id);

    SET_BOARD_TYPE("PCoreMX93", BT_PICOCOREMX93, board_id, len);
    SET_BOARD_TYPE("OSMSFMX93", BT_OSMSFMX93, board_id, len);

    return -EINVAL;
}
```

Additionally, as in `spl.c`, the `board_early_init_f()` function must be defined. `fs_setup_cfg_info()` reads and sets the BOARD-CFG struct in `gd`.

```c
int board_early_init_f(void)
{    
    fs_setup_cfg_info();

    switch(gd->board_type) {
        case BT_PICOCOREMX93:
            imx_iomux_v3_setup_multiple_pads(lpuart2_pads, ARRAY_SIZE(lpuart2_pads));
            init_uart_clk(LPUART2_CLK_ROOT);
            break;
        case BT_OSMSFMX93:
            imx_iomux_v3_setup_multiple_pads(lpuart1_pads, ARRAY_SIZE(lpuart1_pads));
            init_uart_clk(LPUART1_CLK_ROOT);
            break;
        default:
            return -EINVAL;
            break;
    }
    return 0;
}
```
To ensure that U-Boot loads its correct device tree from the MULTI-DTB-FIT configuration, the following function must be defined:
```c
#if CONFIG_IS_ENABLED(MULTI_DTB_FIT)
/* Definition for U-Boot */
int board_fit_config_name_match(const char *name)
{
    void *fdt;
    int offs;
    const char *board_fdt;

    fdt = fs_image_get_cfg_fdt();
    offs = fs_image_get_board_cfg_offs(fdt);
    board_fdt = fs_image_getprop(fdt, offs, 0, "board-fdt", NULL);

    if (board_fdt && !strncmp(name, board_fdt, 64))
        return 0;

    return -EINVAL;
}
#endif
```
