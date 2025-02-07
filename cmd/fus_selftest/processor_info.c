#include <common.h>
#include <asm/mach-imx/sys_proto.h>
#include <asm/arch/clock.h>
#include <imx_thermal.h>
#include <dm.h>
#include <cpu.h>


 struct cpuinfo {
	u32 freq;
	const char *temp;
	const char *type;
	u32 rev_h, rev_l;
};


static int get_processorInfo_soc(struct cpuinfo *ci){

#if defined(CONFIG_IMX8MM) ||  defined(CONFIG_IMX8MN) || defined (CONFIG_IMX8MP)
	u32 cpurev = get_cpu_rev();
	u32 max_freq;
	int minc,maxc;

	ci->type = get_imx_type((cpurev & 0x1FF000) >> 12);
	ci->rev_h = (cpurev & 0x000F0) >> 4;
	ci->rev_l = (cpurev & 0x0000F) >> 0;

	max_freq = get_cpu_speed_grade_hz();
	ci->freq = mxc_get_clock(MXC_ARM_CLK) / 1000000;
	if (!max_freq || max_freq == mxc_get_clock(MXC_ARM_CLK)) {
		ci->freq = mxc_get_clock(MXC_ARM_CLK) / 1000000;
	} else {
		ci->freq = max_freq / 1000000;
	}

	switch (get_cpu_temp_grade(&minc, &maxc)) {
	case TEMP_AUTOMOTIVE:
		ci->temp = "Automotive";
		break;
	case TEMP_INDUSTRIAL:
		ci->temp = "Industrial";
		break;
	case TEMP_EXTCOMMERCIAL:
		ci->temp = "Extended Commercial";
		break;
	default:
		ci->temp = "Commercial";
		break;
	}

	return 0;
#else
	return -1;
#endif
}


int get_processorInfo (void){

	struct cpuinfo ci;
	int ret = 0;

	ret = get_processorInfo_soc(&ci);

	if (ret)
		printf("No CPU-Info found \n");
	else
		printf("CPU: i.MX %s %s %dMHz Rev %d.%d\n",
				ci.type, ci.temp, ci.freq, ci.rev_h, ci.rev_l);
	return ret;

}

int print_cpuinfo(void)
{
	struct udevice *dev;
	char desc[512];
	int ret;

	dev = cpu_get_current_dev();
	if (!dev) {
		debug("%s: Could not get CPU device\n",
		      __func__);
		return -ENODEV;
	}

	ret = cpu_get_desc(dev, desc, sizeof(desc));
	if (ret) {
		debug("%s: Could not get CPU description (err = %d)\n",
		      dev->name, ret);
		return ret;
	}

	printf("CPU: %s\n", desc);

	return 0;
}