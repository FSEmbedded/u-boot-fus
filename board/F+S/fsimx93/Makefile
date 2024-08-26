#
# Copyright 2024 F&S Elektronik Systeme GmbH
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

obj-y += fsimx93.o

ifdef CONFIG_SPL_BUILD
obj-y += spl.o
obj-$(CONFIG_TARGET_FSIMX93_PC) += lpddr4_pc_MT53D512M16D1_timing.o
obj-$(CONFIG_TARGET_FSIMX93_OSM) += lpddr4_osm_MT53D512M16D1_timing.o
endif
