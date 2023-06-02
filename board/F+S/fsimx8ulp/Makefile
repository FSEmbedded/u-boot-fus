# SPDX-License-Identifier: GPL-2.0+

obj-y   += fsimx8ulp.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
ifdef CONFIG_IMX8ULP_ND_MODE
obj-$(CONFIG_TARGET_FSIMX8ULP) += lpddr4_timing_266.o
else
obj-$(CONFIG_TARGET_FSIMX8ULP) += lpddr4_timing.o
endif
else
obj-y				+= ../../freescale/common/mmc.o
endif
