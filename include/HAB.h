/*
 * Filename: HAB.h
 *
 * Description: Program to use HAB Libary
 *
 * */

#ifndef __HAB_H__
#define __HAB_H__

#include <common.h>

/* Data Structures  */

typedef enum hab_structure
{
    HAB_TAG_IVT = 0xd1,
    HAB_TAG_DCD = 0xd2,
    HAB_TAG_CSF = 0xd4,
    HAB_TAG_CRT = 0xd7,
    HAB_TAG_SIG = 0xd8,
    HAB_TAG_EVT = 0xdb,
    HAB_TAG_RVT = 0xdd,
    HAB_TAG_WRP = 0x81,
    HAB_TAG_MAC = 0xac,
    HAB_MAJOR_VERSION = 0x04
}hab_hdr_t;

/* Status definitions  */
typedef enum hab_status
{
    HAB_STS_ANY = 0x00,
    HAB_FAILURE = 0x33,
    HAB_WARNING = 0x69,
    HAB_SUCCESS = 0xf0
}hab_status_t;

/* Target definitions  */
typedef enum hab_target
{
    HAB_TGT_MEMORY = 0x0f,      /* check memory white list  */
    HAB_TGT_PERIPHERAL = 0xf0,  /* check peripheral white list  */
    HAB_TGT_ANY = 0x55          /* check memory and peripheral white list  */
}hab_target_t;

/* security configuration definitions  */
typedef enum hab_config
{
    HAB_CFG_RETURN = 0x33,      /* Field Return IC  */
    HAB_CFG_OPEN = 0xf0,        /* Non-secure IC  */
    HAB_CFG_CLOSED = 0xcc       /* Secure IC  */
}hab_config_t;

/* State definitions  */
typedef enum hab_state
{
    HAB_STATE_INITIAL = 0x33,   /* initialising state  */
    HAB_STATE_CHECK = 0x55,     /* check state  */
    HAB_STATE_NONSECURE = 0x66, /* Non secure state  */
    HAB_STATE_TRUSTED = 0x99,   /* Trusted state  */
    HAB_STATE_SECURE = 0xaa,    /* secure state  */
    HAB_STATE_FAIL_SOFT = 0xcc, /* soft fail state  */
    HAB_STATE_FAIL_HARD = 0xff, /* hard fail state; terminal  */
    HAB_STATE_NONE = 0xf0,      /* no security state machine  */
    HAB_STATE_MAX
}hab_state_t;

/* assertion definition  */
typedef enum hab_assertion_t
{
    HAB_ASSERTION_BLOCK = 0x0
}hab_assertion_t;

typedef u32 *hab_image_entry_f;

typedef hab_status_t (*hab_loader_callback_f)(void **start, size_t *bytes, const void *boot_data);

struct rvt
{
    /* Header with tag HAB_TAG_RVT, length and HAB version fields (see Data Structures)  */
    hab_hdr_t hdr;

    /* enter and initialize library  */
    hab_status_t(* entry)(void);

    /* finalize and exit HAB library  */
    hab_status_t(*exit)(void);

    /* check target address  */
    hab_status_t(*check_target)(hab_target_t type, const void* start, size_t bytes);

    /* authenticate image  */
    hab_status_t(*authenticate_image)(uint8_t cid, ptrdiff_t ivt_offset, void **start, size_t *bytes, hab_loader_callback_f loader);

    /* execute a boot configuration script  */
    hab_status_t(*run_dcd)(const uint8_t *dcd);

    /* execute an authentication script  */
    hab_status_t(*run_csf)(const uint8_t *csf, uint8_t cid);

    /* test an assertion against the audit  */
    hab_status_t(*assert)(hab_assertion_t type, const void *data, uint32_t count);

    /* report an event from the audit log */
    hab_status_t(*report_event)(hab_status_t status, uint32_t index, uint8_t *event, size_t * bytes);

    /* report security status  */
    hab_status_t(*report_status)(hab_config_t* config, hab_status_t *state);

    /* enter failsafe boot mode. The ROM Vector Table consists of a Header follwoed by
     * a list of addresses as described further below*/
    void(*failsafe)(void);
};

void DisplayEvent(uint8_t* , size_t);
void GetHABStatus(void);

#endif

