/*
* Copyright 2025 F&S Elektronik Systeme GmbH
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef _CONFIG_FUS_BOOTCMD
#define _CONFIG_FUS_BOOTCMD

#define BOOT_WITH_FDT "booti ${loadaddr} - ${fdt_addr_r}"

#if defined(CONFIG_ENV_IS_IN_MMC)
#define FILSEIZE2BLOCKCOUNT "block_size=200\0" 	\
		"filesize2blockcount=" \
			"setexpr test_rest \\${filesize} % \\${block_size}; " \
			"if test \\${test_rest} = 0; then " \
				"setexpr blockcount \\${filesize} / \\${block_size}; " \
			"else " \
				"setexpr blockcount \\${filesize} / \\${block_size}; " \
				"setexpr blocckount \\${blockcount} + 1; " \
			"fi;\0"
#else
#define FILSEIZE2BLOCKCOUNT
#endif /* CONFIG_ENV_IS_IN_MMC */

/*
 * In case of (e)MMC, the rootfs is loaded from a separate partition. Kernel
 * and device tree are loaded as files from a different partition that is
 * typically formated with FAT.
 */
#ifdef CONFIG_CMD_MMC
#define BOOT_FROM_MMC								\
	".kernel_mmc=setenv kernel \"mmc rescan; "				\
		" load mmc ${mmcdev} . ${bootfile}\"\0"				\
	".fdt_mmc=setenv fdt \"mmc rescan; "					\
		"load mmc ${mmcdev} ${fdt_addr_r} ${fdtfile}; "			\
		BOOT_WITH_FDT "\"\0"						\
	".rootfs_mmc=setenv root /dev/mmcblk${mmcdev}p2 rootwait\0"
#else
#define BOOT_FROM_MMC
#endif

/* In case of USB, the layout is the same as on MMC. */
#define BOOT_FROM_USB							\
	".kernel_usb=setenv kernel \"usb start; "				\
		"load usb 0 . ${bootfile}\"\0"				\
	".fdt_usb=setenv fdt \"usb start; "				\
		"load usb 0 ${fdt_addr_r} ${fdtfile}; " 		\
		BOOT_WITH_FDT "\"\0"					\
	".rootfs_usb=setenv root /dev/sda1 rootwait\0"

/* In case of TFTP, kernel and device tree are loaded from TFTP server */
#define BOOT_FROM_TFTP							\
	".kernel_tftp=setenv kernel \"tftpboot . ${bootfile}\"\0"	\
	".fdt_tftp=setenv fdt \"tftpboot ${fdt_addr_r} ${fdtfile}; " 	\
		BOOT_WITH_FDT "\"\0"					\

/* In case of NFS, kernel, device tree and rootfs are loaded from NFS server */
#define BOOT_FROM_NFS							\
	".kernel_nfs=setenv kernel \"nfs . "				\
		"${serverip}:${rootpath}/${bootfile}\"\0"		\
	".fdt_nfs=setenv fdt \"nfs ${fdt_addr_r} "			\
		"${serverip}:${rootpath}/${fdtfile}; "			\
		BOOT_WITH_FDT"\"\0"					\
	".rootfs_nfs=setenv root /dev/nfs "				\
		"nfsroot=${serverip}:${rootpath},v3,tcp\0"

#define FUS_LEGACY_BOOT \
	BOOT_FROM_MMC	\
	BOOT_FROM_USB	\
	BOOT_FROM_TFTP	\
	BOOT_FROM_NFS	\
	"bootcmd_fus_legacy="						\
		"if test \"${sec_boot}\" = \"yes\" || test \"${use_ab}\" = \"true\"; then "	\
			"echo \"No FuS Legacy-Boot support with A/B- or Secure-Boot active!\"; "	\
			"echo \"Skip Legacy Boot...\"; "					\
		"else "										\
			"if test -z \"${root}\"; then "						\
				"run .rootfs_mmc; "						\
			"fi; "									\
			"if test -z \"${kernel}\"; then "					\
				"run .kernel_mmc; "						\
			"fi; "									\
			"if test -z \"${fdt}\"; then "						\
				"run .fdt_mmc; "						\
			"fi; "									\
			"run set_bootargs; "							\
			"run kernel; run fdt; "							\
		"fi;\0"

#if defined(CONFIG_FS_UPDATE_SUPPORT)
#define FUS_AB_BOOT \
	"BOOT_ORDER=A B\0"									\
	"BOOT_ORDER_OLD=A B\0"									\
	"BOOT_A_LEFT=3\0"									\
	"BOOT_B_LEFT=3\0"									\
	"update=0000\0"										\
	"application=A\0"									\
	"update_reboot_state=0\0"								\
	".boot_part_A=distro_bootpart=1; \0"							\
	".boot_part_B=distro_bootpart=2; \0"							\
	".rootfs_part_A=distro_rootpart=5; \0"							\
	".rootfs_part_B=distro_rootpart=6; \0"							\
	"bootcmd_mmc0_ab=use_ab=true; run bootcmd_mmc0; use_ab=false;\0"			\
	"bootcmd_mmc1_ab=use_ab=true; run bootcmd_mmc1; use_ab=false;\0"			\
	"scan_dev_for_ab_boot="									\
		"env delete devplist; "								\
		"if test \"${BOOT_ORDER_OLD}\" != \"${BOOT_ORDER}\"; then " 			\
			"if test ${update_reboot_state} -eq 0; then "				\
				"echo \"Update Boot-Order\"; "					\
				"setenv BOOT_ORDER ${BOOT_ORDER_OLD}; "				\
				"saveenv; "							\
			"fi; "									\
			"if test \"${BOOT_ORDER}\" = \"A\"; then "				\
				"setenv BOOT_ORDER A B; "					\
				"saveenv; "							\
			"elif test \"${BOOT_ORDER}\" = \"B\"; then "				\
				"setenv BOOT_ORDER B A; "					\
				"saveenv; "							\
			"fi; "									\
		"fi; "										\
		"for slot in ${BOOT_ORDER}; do "						\
			"if test \"${slot}\" = \"A\"; then "					\
				"slot_cnt=${BOOT_A_LEFT}; "					\
			"else "									\
				"slot_cnt=${BOOT_B_LEFT}; "					\
			"fi; "									\
			"if test ${slot_cnt} -gt 0; then "					\
				"setexpr BOOT_${slot}_LEFT ${slot_cnt} - 1; "			\
				"setenv rauc_cmd rauc.slot=${slot}; "				\
				"saveenv; "							\
				"echo \"Try to boot from slot ${slot} (${slot_cnt} tries left)\"; "	\
				"run .boot_part_${slot}; "					\
				"run .rootfs_part_${slot}; "					\
				"if fstype ${devtype} ${devnum}:${distro_rootpart} rootfstype; then " \
					"part uuid ${devtype} ${devnum}:${distro_rootpart} distro_rootpart_uuid; " \
					"init=init=/usr/sbin/preinit.sh; "			\
					"run scan_dev_for_boot; "				\
				"else "								\
					"echo \"ROOTFS_PART_${slot} does not exist\"; "		\
					"echo \"continuing ...\"; "				\
				"fi; "								\
			"fi; "									\
		"done; "									\
		"if test ${BOOT_A_LEFT} -eq 0 && test ${BOOT_B_LEFT} -eq 0; then "		\
    			"echo \"Both slots have no remaining attempts!\"; "			\
			"echo \"Reset Attempts ...\"; "						\
			"setenv BOOT_A_LEFT 3; setenv BOOT_B_LEFT 3; saveenv; "			\
		"fi;\0"

#else
#define FUS_AB_BOOT
#endif /* CONFIG_FS_UPDATE_SUPPORT */

#define FUS_BOOT_ENV \
	"default_rootpart=2\0"									\
	"bootcmd=run bsp_bootcmd;\0"								\
	"console=undef\0" 									\
	".console_none=setenv console\0"							\
	".console_serial=setenv console ${sercon},${baudrate}\0" 				\
	".console_display=setenv console tty1\0"						\
	"mode=undef\0"										\
	".mode_rw=setenv mode rw rootwait\0"							\
	".mode_ro=setenv mode ro rootwait\0"							\
	"login=undef\0"										\
	".login_none=setenv login login_tty=null\0"						\
	".login_serial=setenv login login_tty=${sercon},${baudrate}\0"				\
	".login_display=setenv login login_tty=tty1\0"						\
	"netdev=eth0\0"										\
	".network_off=setenv network\0"								\
	".network_on=setenv network ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${hostname}:${netdev}\0" \
	".network_dhcp=setenv network ip=dhcp\0"						\
	"bd_kernel=undef\0"									\
	"set_bootargs="										\
		"setenv bootargs \"console=${console} ${login} root=${root} ${mode} ${network} "\
			"${init} ${mcore_clk} ${rauc_cmd} ${extra}\"\0"		\
	"cma_size=256M\0"									\
	".tftp_nfs=setenv boot_targets tftp_nfs;\0"						\
	".tftp_mmc=setenv boot_targets tftp_mmc;\0"						\
	".default_boot=setenv boot_targets mmc0 mmc1 usb0 usb1;\0"				\
	"nfsroot=/rootfs\0"									\
	"bsp_bootcmd="										\
		"for target in ${boot_targets}; "						\
			"do run bootcmd_${target}; "						\
		"done;\0"									\
	"bootcmd_update=update.update;\0"							\
	"bootcmd_install=update.install;\0"							\
	"bootcmd_mcore=boot_mcore=true;\0"							\
	"bootcmd_mmc0=devnum=0; run mmc_boot;\0"						\
	"bootcmd_mmc1=devnum=1; run mmc_boot;\0"						\
	"bootcmd_usb0=devnum=0; run usb_boot;\0"						\
	"bootcmd_usb1=devnum=1; run usb_boot;\0"						\
	"usb_boot="										\
		"usb start; "									\
		"if usb dev ${devnum}; then "							\
			"devtype=usb; "								\
			"run scan_dev_for_boot_part; "						\
		"fi;\0"										\
	".tftp_nfs_root="									\
		"setenv root \"/dev/nfs "							\
			"nfsroot=${serverip}:${nfsroot},v3,tcp\";\0"				\
	".tftp_mmc_root="									\
		"setenv root \"/dev/mmcblk${mmcdev}p${default_rootpart}\";\0"			\
	"ramdisk_addr_r=-\0"									\
	"bootcmd_tftp_mmc="									\
		"run .tftp_mmc_root; "								\
		"run set_bootargs; "								\
		"run tftp_boot;\0"								\
	"bootcmd_tftp_nfs="									\
		"run .tftp_mmc_nfs; "								\
		"run .network_dhcp; "								\
		"run set_bootargs; "								\
		"run tftp_boot;\0"								\
	"tftp_boot="										\
		"if env exists serverip && env exists ipaddr; then "				\
			"run tftp_boot_common; "						\
		"else "										\
			"echo \"check serverip and ipaddr!\"; "					\
		"fi;\0"										\
	"tftp_boot_common="									\
		"tftp ${kernel_addr_r} ${bootfile}; "						\
		"tftp ${fdt_addr_r} ${fdtfile}; "						\
		"booti ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r};\0"			\
	"mmc_boot="										\
		"if mmc dev ${devnum}; then "							\
			"devtype=mmc; "								\
			"run scan_dev_for_boot_part; "						\
		"fi;\0"										\
	"scan_dev_for_boot_part="								\
		"env delete devplist; "								\
		"part list ${devtype} ${devnum} -bootable devplist; "				\
		"if env exists devplist; then "							\
			"if test \"${use_ab}\" = \"true\"; then "				\
				"run scan_dev_for_ab_boot; "					\
			"else "									\
				"distro_rootpart=${default_rootpart}; "				\
				"for distro_bootpart in ${devplist}; do "			\
					"if fstype ${devtype} ${devnum}:${distro_bootpart} bootfstype; then "	\
						"part uuid ${devtype} ${devnum}:${distro_rootpart} distro_rootpart_uuid; "\
						"run scan_dev_for_boot; "			\
					"fi; "							\
				"done; "							\
			"fi; "									\
			"echo \"Failed to Boot from ${devtype}${devnum}\"; "			\
		"else "										\
			"echo \"Failed to scan for Boot-Partitions\"; "				\
			"setenv devplist; "							\
		"fi;\0"										\
	"scan_dev_for_ab_boot=echo \"F&S AB-Boot is not supported\";\0"				\
	"scan_dev_for_boot="									\
		"echo \"Scanning ${devtype} ${devnum}:${distro_bootpart}...\"; " 		\
		"run scan_dev_for_scripts; "							\
		"run scan_dev_for_mcore_images; "						\
		"run scan_dev_for_images;\0"							\
	"scan_dev_for_mcore_images="								\
		"if test \"${boot_mcore}\" = \"true\"; then "					\
			"echo  \"booting mcore is not supported\"; "				\
			"boot_mcore=false; "							\
		"fi;\0"										\
	"scan_dev_for_scripts="									\
		"if test -e ${devtype} ${devnum}:${distro_bootpart} ${prefix}/${script}; then "	\
			"echo Found U-Boot script ${prefix}/${script}; "			\
			"run boot_a_script; "							\
			"echo \"SCRIPT FAILED: continuing...\"; "				\
		"fi;\0"										\
	"boot_a_script="									\
		"load ${devtype} ${devnum}:${distro_bootpart} ${scriptaddr} ${prefix}/${script}; " \
		"source ${scriptaddr};\0"							\
	"scan_dev_for_images="									\
		"if test -e ${devtype} ${devnum}:${distro_bootpart} ${bootfile}; then "		\
			"echo Kernel ${bootfile} found; " 					\
			"if test -e ${devtype} ${devnum}:${distro_bootpart} ${fdtfile}; then " 	\
				"echo DeviceTree ${fdtfile} found; " 				\
				"run boot_a_image; " 						\
			"else " 								\
				"echo ${fdtfile} not found!; "					\
			"fi; " 									\
		"else "										\
			"echo \"Kernel ${bootfile} not found!\"; "				\
		"fi;\0"										\
	"boot_a_image="										\
		"load ${devtype} ${devnum}:${distro_bootpart} ${kernel_addr_r} ${bootfile}; "	\
		"load ${devtype} ${devnum}:${distro_bootpart} ${fdt_addr_r} ${fdtfile}; "	\
		"setenv root PARTUUID=${distro_rootpart_uuid}; "				\
		"run set_bootargs; "								\
		"booti ${kernel_addr_r} - ${fdt_addr_r};\0"					\


#if defined(CONFIG_FS_CNTR_COMMON)
#define MCORE_BOOT \
	"mcore_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR)"\0"					\
	"mcorefile=m33_image.bin\0"								\
	"bootcmd_tftp_mcore="									\
		"run prepare_mcore; "								\
		"tftp ${mcore_addr_r} ${mcorefile}; "						\
		"if test $? -eq 0; then "							\
			"bootaux_cntr ${mcore_addr_r}; "					\
		"fi;\0"										\
	"scan_dev_for_mcore_images="								\
		"if test \"${boot_mcore}\" = \"true\"; then "					\
			"run prepare_mcore; "							\
			"if test -e ${devtype} ${devnum}:${distro_bootpart} ${mcorefile}; then "	\
				"load ${devtype} ${devnum}:${distro_bootpart} ${mcore_addr_r} ${mcorefile}; " \
				"bootaux_cntr ${mcore_addr_r}; "				\
				"boot_mcore=false; "						\
			"else "									\
				"echo \"Failed to boot mcore on ${devtype} ${devnum}:${distro_bootpart} ${prefix}/${mcorefile}\"; "	\
			"fi; "									\
		"fi;\0"
#else /* CONFIG_FS_CNTR_COMMON */
#define MCORE_BOOT \
	"mcore_addr_tcm=0x007E0000\0"								\
	"mcorefile=m33_image.bin\0"								\
	"bootcmd_tftp_mcore="									\
		"run prepare_mcore; "								\
		"tftp ${mcore_addr_tcm} ${mcorefile}; "						\
		"if test $? -eq 0; then "							\
			"bootaux ${mcore_addr_tcm}; "						\
		"fi;\0"										\
	"scan_dev_for_mcore_images="								\
		"if test \"${boot_mcore}\" = \"true\"; then "					\
			"run prepare_mcore; "							\
			"if test -e ${devtype} ${devnum}:${distro_bootpart} ${prefix}/${mcorefile}; then "	\
				"echo \"Found MCORE-Image ${prefix}/${mcorefile}\"; "		\
				"load ${devtype} ${devnum}:${distro_bootpart} ${mcore_addr_tcm} ${prefix}/${mcorefile}; " \
				"bootaux ${mcore_addr_tcm}; "					\
				"boot_mcore=false; "						\
			"else "									\
				"echo \"Failed to boot mcore on ${devtype} ${devnum}:${distro_boot_part} ${prefix}/${mcorefile}\"; "	\
			"fi; "									\
		"fi;\0"
#endif

#if defined(CONFIG_FS_CNTR_COMMON)
#define FUS_AHAB_ENV \
	"sec_boot=no\0"										\
	"tftp_boot_ahab="									\
		"tftp ${cntr_addr_r} ${bootcntrfile}; "						\
		"echo \"Validating Container ...\"; "						\
		"if auth_cntr ${cntr_addr_r}; then "						\
			"echo \"Container is valid\"; "						\
			"bootm ${cntr_loadaddr}#conf-${fdtfile}; "					\
		"else "										\
			"echo \"Container is invalid!\"; "					\
		"fi;\0"										\
	"tftp_boot="										\
		"if env exists serverip && env exists ipaddr; then "				\
			"if test \"${sec_boot}\" = \"yes\"; then "				\
				"run tftp_boot_ahab; "						\
			"else "									\
				"run tftp_boot_common; "					\
			"fi; "									\
		"else "										\
			"echo \"check serverip and ipaddr!\"; "					\
		"fi;\0"										\
	"scan_dev_for_cntr_scripts="								\
		"if test -e ${devtype} ${devnum}:${distro_bootpart} ${prefix}/${script}; then " \
			"echo Found U-Boot script ${prefix}/${script}; "			\
			"run boot_a_cntr_script; "						\
			"echo \"SCRIPT FAILED: continuing...\"; "				\
		"fi;\0"										\
	"boot_a_cntr_script="									\
		"echo \"load script container  ...\"; "						\
		"load ${devtype} ${devnum}:${distro_bootpart} ${cntr_addr_r} ${prefix}/${script}; "	\
		"echo \"check container signature ...\"; "					\
		"if auth_cntr ${cntr_addr_r}; then "						\
			"echo \"container is valid!\"; "					\
			"source ${cntr_loadaddr}; "						\
		"else "										\
			"echo \"container is invalid!\"; "					\
		"fi;\0"										\
	"boot_a_cntr_image="									\
		"load ${devtype} ${devnum}:${distro_bootpart} ${cntr_addr_r} ${bootcntrfile}; "	\
		"setenv root PARTUUID=${distro_rootpart_uuid}; "				\
		"run set_bootargs; "								\
		"echo \"check container signature ...\"; "					\
		"if auth_cntr ${cntr_addr_r}; then "						\
			"echo \"container is valid!\";	"					\
			"bootm ${cntr_loadaddr}#conf-${fdtfile}; "					\
		"else "										\
			"echo  \"container is invalid!\"; "					\
		"fi;\0"										\
	"scan_dev_for_cntr_images="								\
		"if test -e ${devtype} ${devnum}:${distro_bootpart} ${bootcntrfile}; then "	\
			"echo FIT-Image ${bootcntrfile} found; "				\
			"run boot_a_cntr_image; "						\
		"else "										\
			"echo \"FIT-Image ${bootcntrfile} not found!\"; "			\
		"fi;\0"										\
	"scan_dev_for_boot="									\
		"echo Scanning ${devtype} \"${devnum}:${distro_bootpart}...\"; " 		\
		"if test \"${sec_boot}\" = \"yes\"; then "					\
			"run scan_dev_for_cntr_scripts; "					\
			"run scan_dev_for_mcore_images; "					\
			"run scan_dev_for_cntr_images; "					\
		"else "										\
			"run scan_dev_for_scripts; "						\
			"run scan_dev_for_mcore_images; "					\
			"run scan_dev_for_images; "						\
		"fi;\0"										\

#endif

#if defined(CONFIG_FS_WINIOT_SUPPORT)
#define FUS_WIN_BOOT 										\
	"mmc_boot="										\
		"if mmc dev ${devnum}; then "							\
			"devtype=mmc; "								\
			"run scan_dev_for_uefi_fit; "						\
			"run scan_dev_for_boot_part; "						\
		"fi;\0"										\
	"scan_dev_for_uefi_fit="								\
		"mmc read ${loadaddr} 0x3a00 0xfff; "						\
		"if iminfo ${loadaddr} ; then "							\
			"bootm ${loadaddr}; "							\
		"fi;\0"										\

#else
#define FUS_WIN_BOOT
#endif

#endif /* _CONFIG_FUS_BOOTCMD */
