# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2004-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.

# core
ifndef CONFIG_SPL_BUILD
obj-y += init/
obj-y += main.o
obj-y += exports.o
obj-y += cli_getch.o cli_simple.o cli_readline.o
obj-$(CONFIG_HUSH_OLD_PARSER) += cli_hush.o
obj-$(CONFIG_HUSH_MODERN_PARSER) += cli_hush_modern.o
obj-$(CONFIG_AUTOBOOT) += autoboot.o
obj-$(CONFIG_BUTTON_CMD) += button_cmd.o
obj-y += version.o

# # boards
obj-y += board_f.o
obj-y += board_r.o
obj-$(CONFIG_DISPLAY_BOARDINFO) += board_info.o
obj-$(CONFIG_DISPLAY_BOARDINFO_LATE) += board_info.o

obj-$(CONFIG_MII) += miiphyutil.o
obj-$(CONFIG_CMD_MII) += miiphyutil.o
obj-$(CONFIG_PHYLIB) += miiphyutil.o

obj-$(CONFIG_USB_HOST) += usb.o usb_hub.o
obj-$(CONFIG_USB_GADGET) += usb.o
obj-$(CONFIG_USB_STORAGE) += usb_storage.o
obj-$(CONFIG_USB_ONBOARD_HUB) += usb_onboard_hub.o

# Extended LCD support
obj-$(CONFIG_CMD_LCD) += xlcd_panels.o
obj-$(CONFIG_CMD_DRAW) += xlcd_draw_ll.o xlcd_drawtext_ll.o
obj-$(CONFIG_CMD_ADRAW) += xlcd_adraw_ll.o
obj-$(CONFIG_XLCD_CONSOLE) += xlcd_drawtext_ll.o
obj-$(CONFIG_XLCD_PNG) += xlcd_png.o
obj-$(CONFIG_XLCD_BMP) += xlcd_bmp.o
obj-$(CONFIG_XLCD_JPG) += xlcd_jpg.o

# others
obj-$(CONFIG_CONSOLE_MUX) += iomux.o
obj-$(CONFIG_MTD_NOR_FLASH) += flash.o
obj-$(CONFIG_CMD_KGDB) += kgdb.o kgdb_stubs.o
obj-$(CONFIG_I2C_EDID) += edid.o
obj-$(CONFIG_KALLSYMS) += kallsyms.o
obj-$(CONFIG_SPLASH_SCREEN) += splash.o
obj-$(CONFIG_SPLASH_SOURCE) += splash_source.o
obj-$(CONFIG_MENU) += menu.o
obj-$(CONFIG_UPDATE_COMMON) += update.o
obj-$(CONFIG_USB_KEYBOARD) += usb_kbd.o

endif # !CONFIG_SPL_BUILD

obj-$(CONFIG_$(SPL_TPL_)BOOTSTAGE) += bootstage.o
obj-$(CONFIG_$(SPL_TPL_)BLOBLIST) += bloblist.o

ifdef CONFIG_SPL_BUILD
ifdef CONFIG_SPL_DFU
obj-$(CONFIG_DFU_OVER_USB) += dfu.o
endif
obj-$(CONFIG_SPL_NET) += miiphyutil.o

obj-$(CONFIG_SPL_USB_HOST) += usb.o usb_hub.o
obj-$(CONFIG_SPL_USB_STORAGE) += usb_storage.o
obj-$(CONFIG_SPL_MUSB_NEW) += usb.o
obj-$(CONFIG_SPL_SPLASH_SCREEN) += splash.o
obj-$(CONFIG_SPL_SPLASH_SOURCE) += splash_source.o
endif # CONFIG_SPL_BUILD

#others
obj-$(CONFIG_DDR_SPD) += ddr_spd.o
obj-$(CONFIG_SPD_EEPROM) += ddr_spd.o
obj-$(CONFIG_HWCONFIG) += hwconfig.o
obj-$(CONFIG_BOUNCE_BUFFER) += bouncebuf.o
ifdef CONFIG_SPL_BUILD
ifdef CONFIG_TPL_BUILD
obj-$(CONFIG_TPL_SERIAL) += console.o
else
obj-$(CONFIG_SPL_SERIAL) += console.o
endif
else
obj-y += console.o
endif # CONFIG_SPL_BUILD

obj-$(CONFIG_CROS_EC) += cros_ec.o
obj-y += dlmalloc.o
obj-$(CONFIG_$(SPL_TPL_)SYS_MALLOC_F) += malloc_simple.o

obj-$(CONFIG_CYCLIC) += cyclic.o
obj-$(CONFIG_$(SPL_TPL_)EVENT) += event.o

obj-$(CONFIG_$(SPL_TPL_)HASH) += hash.o
obj-$(CONFIG_IO_TRACE) += iotrace.o
obj-y += memsize.o
obj-y += stdio.o

ifdef CONFIG_CMD_EEPROM_LAYOUT
obj-y += eeprom/eeprom_field.o eeprom/eeprom_layout.o
endif

obj-y += cli.o
obj-$(CONFIG_FSL_DDR_INTERACTIVE) += cli_getch.o cli_simple.o cli_readline.o
obj-$(CONFIG_STM32MP1_DDR_INTERACTIVE) += cli_getch.o cli_simple.o cli_readline.o
obj-$(CONFIG_DFU_OVER_USB) += dfu.o
obj-y += command.o
obj-$(CONFIG_$(SPL_TPL_)LOG) += log.o
obj-$(CONFIG_$(SPL_TPL_)LOG_CONSOLE) += log_console.o
obj-$(CONFIG_$(SPL_TPL_)LOG_SYSLOG) += log_syslog.o
obj-$(CONFIG_CMD_LOADS) += s_record.o
obj-$(CONFIG_CMD_LOADB) += xyzModem.o
obj-$(CONFIG_$(SPL_TPL_)YMODEM_SUPPORT) += xyzModem.o

obj-$(CONFIG_$(SPL_TPL_)AVB_VERIFY) += avb_verify.o
obj-$(CONFIG_$(SPL_TPL_)STACKPROTECTOR) += stackprot.o
obj-$(CONFIG_SCP03) += scp03.o

obj-$(CONFIG_QFW) += qfw.o
obj-$(CONFIG_KASLR) += kaslr.o
