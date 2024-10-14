#
# (C) Copyright 2018
# Hartmut Keller, F&S Elektronik Systeme GmbH, keller@fs-net.de
#
# Common code for different F&S boards
#
# SPDX-License-Identifier:	GPL-2.0+
#

MINIMAL=

ifdef CONFIG_SPL_BUILD
ifdef CONFIG_SPL_INIT_MINIMAL
MINIMAL=y
endif
endif

ifdef MINIMAL
# necessary to create built-in.o
obj- := __dummy__.o
else

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_TARGET_FSGAL)	+= fs_common.o
obj-$(CONFIG_TARGET_FSGAL)	+= fs_ls1028a_common.o
obj-$(CONFIG_TARGET_FSGAL)	+= fs_fdt_common.o
obj-$(CONFIG_TARGET_FSGAL)	+= fs_eth_common.o
ifdef CONFIG_NXP_ESBC
obj-$(CONFIG_CMD_ESBC_VALIDATE)	+= ../../freescale/common/cmd_esbc_validate.o
obj-$(CONFIG_CMD_ESBC_VALIDATE)	+= ../../freescale/common/fsl_validate.o
endif
obj-$(CONFIG_CHAIN_OF_TRUST)	+= ../../freescale/common/fsl_chain_of_trust.o
endif
endif