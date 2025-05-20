#include <common.h>
#include <asm/mach-imx/sys_proto.h>
#include <asm/arch/clock.h>
#include <imx_thermal.h>
#include <dm.h>
#include <cpu.h>
#include <env.h>

struct cpuinfo {
	u32 freq;
	const char *temp;
	const char *type;
	u32 rev_h, rev_l;
};

#if defined(CONFIG_IMX8MM) ||  defined(CONFIG_IMX8MN) || defined(CONFIG_IMX8MP) || defined(CONFIG_IMX8ULP)
static int get_processorInfo_soc(struct cpuinfo *ci){

	u32 cpurev = get_cpu_rev();
	u32 max_freq;
	__maybe_unused int minc,maxc;

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

#if !defined(CONFIG_IMX8ULP)
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
#else
	ci->temp = "UNKNOWN";
#endif
	return 0;
}
#endif

int get_processorInfo(bool silent){

	struct cpuinfo ci;
	char cpu_str[512];
	int ret = 0;

	ret = get_processorInfo_soc(&ci);

	if (ret){
		printf("No CPU-Info found \n");
		return ret;
	}

	snprintf(cpu_str, 512, "CPU: i.MX %s %s %dMHz Rev %d.%d\n",
		ci.type, ci.temp, ci.freq, ci.rev_h, ci.rev_l);

	if(!silent)
		printf("%s", cpu_str);

	return ret;
}

#if CONFIG_IS_ENABLED(CPU)
int print_cpuinfo(bool silent)
{
	struct udevice *dev;
	char cpu_str[512];
	char *desc;
	int ret;

	dev = cpu_get_current_dev();
	if (!dev) {
		debug("%s: Could not get CPU device\n",
		      __func__);
		return -ENODEV;
	}

	snprintf(cpu_str, 512, "CPU: ");
	desc = &cpu_str[5];

	ret = cpu_get_desc(dev, desc, sizeof(cpu_str) - 5);
	if (ret) {
		debug("%s: Could not get CPU description (err = %d)\n",
		      dev->name, ret);
		return ret;
	}

	if(!silent)
		printf("%s\n", cpu_str);

	env_set("cpu_info", cpu_str);

	return 0;
}
#endif