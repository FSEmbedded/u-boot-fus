/*
 * Filename: hab.c
 *
 * Description: Program to check and display HAB Status and get the HAB address
 *
 */

#include <hab.h>
#include <asm/arch/sys_proto.h>		//get_cpu_rev, get_imx_type


/* get address of hab function vector table */

/* minor version differs between processor revisions, so set to 0 and mask out
 * for comparison
 */
#define HAB_HDR_SDQ 0x402c00dd
#define HAB_HDR_SX  0x402c00dd
#define HAB_HDR_UL  0x403000dd
#define HAB_HDR_VYB 0x402c00dd


/*
 * Function:   GetHABAddress(void)
 *
 * Parameters: -
 *
 * Return:     u32 -> start address of the hab library
 *
 * Content:    check the CPU type and set the hab address.
 */
u32 GetHABAddress(void)
{
	u32* addr = 0;

#ifdef CONFIG_FSIMX6
	u32 cpurev = 0;

	cpurev = get_cpu_rev();
	switch ((cpurev & 0xFF000) >> 12) {
		case MXC_CPU_MX6D:
		case MXC_CPU_MX6Q:
			if (((cpurev & 0x000F0) >> 4) == 1 && ((cpurev & 0x0000F) >> 0) >= 5)
			addr = (u32*)0x00000098;
			else
			addr = (u32*)0x00000094;
			break;
		case MXC_CPU_MX6SOLO:
		case MXC_CPU_MX6DL:
			if (((cpurev & 0x000F0) >> 4) == 1 && ((cpurev & 0x0000F) >> 0) >= 2)
			addr =  (u32*)0x00000098;
			else
			addr = (u32*)0x00000094;
			break;
		default:
			printf("processor type not supported\n");
			return 0;
			break;
	}
	if ((*addr & 0xf0f0ffff) != HAB_HDR_SDQ)
	{
		printf("HAB_HDR not found\n");
		return 0;
	}
	return (u32)addr;
#elif defined CONFIG_FSIMX6SX
	addr = (u32*)0x00000100;
	if ((*addr & 0xf0f0ffff) != HAB_HDR_SX)
	{
		printf("HAB_HDR not found\n");
		return 0;
	}
	return (u32)addr;
#elif defined CONFIG_FSIMX6UL
	addr = (u32*)0x00000100;
	if ((*addr & 0xf0f0ffff) != HAB_HDR_UL)
	{
		printf("HAB_HDR not found\n");
		return 0;
	}
	return (u32)addr;
#elif defined CONFIG_FSVYBRID
	addr = (u32*)0x00000054;
	if ((*addr & 0xf0f0ffff) != HAB_HDR_VYB)
	{
		printf("HAB_HDR not found\n");
		return 0;
	}
	return (u32)addr;
#else
#ifndef __DEPEND__
#error "no processor defined"
#endif
#endif
}


/*
 * Function:   DisplayEvent(uint8_t *event_data, size_t bytes)
 *
 * Parameters: event_data -> data of the events
 *             bytes      -> length of the events
 *
 * Return:     -
 *
 * Content:    print out the reported events.
 */
void DisplayEvent(uint8_t *event_data, size_t bytes)
{
  uint32_t i;
  if((event_data) && (bytes > 0))
    {
      for(i = 0; i < bytes; i++)
	{
	  if(i == 0)
	    {
	      printf(" 0x%02x", event_data[i]);
	    }else if((i % 8) == 0)
	    {
	      printf("\n 0x%02x", event_data[i]);
	    }else
	    {
	      printf(" 0x%02x", event_data[i]);
	    }
	}
    }
}


/*
 * Function:   GetHABStatus(void)
 *
 * Parameters: -
 *
 * Return:     -
 *
 * Content:    check if any events were reported.
 */
void GetHABStatus(void)
{
	struct rvt* hab = NULL;
	uint32_t index = 0; // Loop index
	uint8_t event_data[128]; // Event Data Buffer
	size_t bytes = sizeof(event_data); //event size in bytes
	hab_config_t config = 0;
	hab_status_t state = 0;

	if (GetHABAddress())
		hab = (struct rvt*)GetHABAddress();
	else
		return;
	/* Check HAB Status */
	if(hab->report_status(&config, &state) != HAB_SUCCESS) {
		/* Display HAB Error events */
		while(hab->report_event(HAB_STS_ANY, index, event_data, &bytes) == HAB_SUCCESS) {
			printf("\n");
			printf("---------- HAB EVENT %d ----------\n", index+1);
			printf("event data:\n");
			DisplayEvent(event_data, bytes);
			printf("\n\n");
			index++;
		}
	}
	/* Display messages if no HAB events are found */
	else {
		printf("\nHAB Configuration: 0x%02x HAB State: 0x%02x\n", config, state);
		printf("No HAB Events Found!\n\n");
	}
}
