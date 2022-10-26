#include <common.h>
#include <dm.h>

#include "../check_config.h"

int skip_node(struct udevice *dev)
{
	/* CAN / mcp251x */
	if (dev_read_bool(dev, "can-spi-mcp251x")) {
		if (has_feature(FEAT_CAN)) {
			return 1;
		}
	}

	/* WLAN / mcp251x */
	if (dev_read_bool(dev, "is-wlan")) {
		if (has_feature(FEAT_WLAN)) {
			port++;
			return 1;
		}
	}

	/* MIPI */
	if (dev_read_bool(dev, "mipi-gpios-only")) {
		if (!has_feature(FEAT_MIPI_DSI)) {
			return 1;
		}
	}

	/* LVDS */
	if (dev_read_bool(dev, "lvds-gpios-only")) {
		if (!has_feature(FEAT_LVDS)) {
			return 1;
		}
	}

	return 0;
}
