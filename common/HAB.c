/*
 *
 * Filename: HAB.c
 *
 * Description: Program ti check and display HAB Status
 *
 *
 ******************************************************************************/

#include <HAB.h>

/* Display logged events  */

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

/* check if HAB logged Failures/Warnings  */
void GetHABStatus(void)
{
#ifdef HAB_RVT_iMX6
    struct rvt *hab = (struct rvt*) 0x00000098;
#endif
#ifdef HAB_RVT_VYBRID
    struct rvt *hab = (struct rvt*) 0x00000054;
#endif

    uint32_t index = 0; // Loop index
    uint8_t event_data[128]; // Event Data Buffer
    size_t bytes = sizeof(event_data); //event size in bytes
    hab_config_t config = 0;
    hab_status_t state = 0;
    /* Check HAB Status  */
    if(hab->report_status(&config, &state) != HAB_SUCCESS)
    {
        printf("\nHAB Configuration: 0x%02x HAB State: 0x%02x\n", config, state);
        /* Display HAB Error events  */
        while(hab->report_event(HAB_STS_ANY, index, event_data, &bytes) == HAB_SUCCESS)
        {
            printf("\n");
            printf("---------- HAB EVENT %d ----------\n", index+1);
            printf("event data:\n");
            DisplayEvent(event_data, bytes);
            printf("\n");
            index++;
        }
    }
    /* Display messages if no HAB events are found  */
    else
    {
        printf("\nHAB Configuration: 0x%02x HAB State: 0x%02x\n", config, state);
        printf("No HAB Events Found!\n\n");
    }
}
