#include <common.h>
#include <dm.h>

#include "../check_config.h"

int skip_node(struct udevice *dev)
{

		/* WLAN / mcp251x */
	if (dev_read_bool(dev, "is-wlan")) {
		if (has_feature(FEAT_WLAN)) {
			return 1;
		}
	}

	return 0;
}
