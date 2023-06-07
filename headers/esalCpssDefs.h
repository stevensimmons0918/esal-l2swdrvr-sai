/*
 *  esalCpssDefs.h
 *
 *  Copyright (C) 2022, Fujitsu Networks Communications, Inc.
 *
 *  Brief: Global Definitions of CPSS (Marvell SDK) used by ESAL.
 *
 *  Author: Steve Simmons
 */

#ifndef _ESAL_CPSS_DEFS_H_
#define _ESAL_CPSS_DEFS_H_
extern "C" {


#ifdef HAVE_MRVL

/**
* @enum GT_BOOL
 *
 * @brief Enumeration of boolean.
*/
typedef enum{

    /** false. */
    GT_FALSE = 0,

    /** true. */
    GT_TRUE  = 1

} GT_BOOL;

typedef unsigned int GT_STATUS;
/***** generic return codes **********************************/
#define GT_ERROR                 (-1)
#define GT_OK                    (0x00) /* Operation succeeded */
#define GT_FAIL                  (0x01) /* Operation failed    */

#define GT_BAD_VALUE             (0x02) /* Illegal value        */
#define GT_OUT_OF_RANGE          (0x03) /* Value is out of range*/
#define GT_BAD_PARAM             (0x04) /* Illegal parameter in function called  */
#define GT_BAD_PTR               (0x05) /* Illegal pointer value                 */
#define GT_BAD_SIZE              (0x06) /* Illegal size                          */
#define GT_BAD_STATE             (0x07) /* Illegal state of state machine        */
#define GT_SET_ERROR             (0x08) /* Set operation failed                  */
#define GT_GET_ERROR             (0x09) /* Get operation failed                  */
#define GT_CREATE_ERROR          (0x0A) /* Fail while creating an item           */
#define GT_NOT_FOUND             (0x0B) /* Item not found                        */
#define GT_NO_MORE               (0x0C) /* No more items found                   */
#define GT_NO_SUCH               (0x0D) /* No such item                          */
#define GT_TIMEOUT               (0x0E) /* Time Out                              */
#define GT_NO_CHANGE             (0x0F) /* The parameter is already in this value*/
#define GT_NOT_SUPPORTED         (0x10) /* This request is not support           */
#define GT_NOT_IMPLEMENTED       (0x11) /* This request is not implemented       */
#define GT_NOT_INITIALIZED       (0x12) /* The item is not initialized           */
#define GT_NO_RESOURCE           (0x13) /* Resource not available (memory ...)   */
#define GT_FULL                  (0x14) /* Item is full (Queue or table etc...)  */
#define GT_EMPTY                 (0x15) /* Item is empty (Queue or table etc...) */
#define GT_INIT_ERROR            (0x16) /* Error occurred while INIT process     */
#define GT_NOT_READY             (0x1A) /* The other side is not ready yet       */
#define GT_ALREADY_EXIST         (0x1B) /* Tried to create existing item         */
#define GT_OUT_OF_CPU_MEM        (0x1C) /* Cpu memory allocation failed.         */
#define GT_ABORTED               (0x1D) /* Operation has been aborted.           */
#define GT_NOT_APPLICABLE_DEVICE (0x1E) /* API not applicable to device , can
                                           be returned only on devNum parameter  */
#define GT_UNFIXABLE_ECC_ERROR   (0x1F) /* the CPSS detected ECC error that can't
                                           be fixed when reading from the memory which is protected by ECC.
                                           NOTE: relevant only when the table resides in the CSU ,
                                           the ECC is used , and the CPSS emulates the ECC detection
                                           and correction for 'Read entry' operations */
#define GT_UNFIXABLE_BIST_ERROR  (0x20) /* Built-in self-test detected unfixable error */
#define GT_CHECKSUM_ERROR        (0x21) /* checksum doesn't fits received data */
#define GT_DSA_PARSING_ERROR     (0x22) /* DSA tag parsing error */
#define GT_TX_RING_ERROR         (0x23) /* TX descriptors ring broken            */
#define GT_NOT_ALLOWED           (0x24) /* The operation is not allowed          */
#define GT_HW_ERROR_NEED_RESET   (0x25) /* There was HW error that requires HW reset (soft/hard) */
#define GT_LEARN_LIMIT_PORT_ERROR               (0x26)/* FDB manager - learn limit on port reached  */
#define GT_LEARN_LIMIT_TRUNK_ERROR              (0x27)/* FDB manager - learn limit on trunk reached */
#define GT_LEARN_LIMIT_GLOBAL_EPORT_ERROR       (0x28)/* FDB manager - learn limit on global eport reached */
#define GT_LEARN_LIMIT_FID_ERROR                (0x29)/* FDB manager - learn limit on fid reached   */
#define GT_LEARN_LIMIT_GLOBAL_ERROR             (0x2a)/* FDB manager - learn limit globally reached */

/* size of array of interfaces advertised by port during AP process */
#define CPSS_DXCH_PORT_AP_IF_ARRAY_SIZE_CNS 10

typedef int8_t      GT_8,   *GT_8_PTR;
typedef uint8_t     GT_U8,  *GT_U8_PTR;
typedef short               GT_16,  *GT_16_PTR;
typedef unsigned short      GT_U16, *GT_U16_PTR;
typedef int32_t     GT_32,  *GT_32_PTR;
typedef uint32_t    GT_U32,  *GT_U32_PTR;

typedef GT_U32 GT_PHYSICAL_PORT_NUM;

/**
* @struct GT_ETHERADDR
 *
 * @brief Defines the mac address
*/
typedef struct{

    GT_U8 arEther[6];

} GT_ETHERADDR;

#include "esalCpssPortCtrl.h"

/**
* @enum CPSS_PORT_SPEED_ENT
 *
 * @brief Enumeration of port speeds
*/
typedef enum
{
    CPSS_PORT_SPEED_10_E,       /* 0 */
    CPSS_PORT_SPEED_100_E,      /* 1 */
    CPSS_PORT_SPEED_1000_E,     /* 2 */
    CPSS_PORT_SPEED_10000_E,    /* 3 */
    CPSS_PORT_SPEED_12000_E,    /* 4 */
    CPSS_PORT_SPEED_2500_E,     /* 5 */
    CPSS_PORT_SPEED_5000_E,     /* 6 */
    CPSS_PORT_SPEED_13600_E,    /* 7 */
    CPSS_PORT_SPEED_20000_E,    /* 8 */
    CPSS_PORT_SPEED_40000_E,    /* 9 */
    CPSS_PORT_SPEED_16000_E,    /* 10 */
    CPSS_PORT_SPEED_15000_E,    /* 11 */
    CPSS_PORT_SPEED_75000_E,    /* 12 */
    CPSS_PORT_SPEED_100G_E,     /* 13 */
    CPSS_PORT_SPEED_50000_E,    /* 14 */
    CPSS_PORT_SPEED_140G_E,     /* 15 */

    CPSS_PORT_SPEED_11800_E,    /* 16 */ /*used in combination with CPSS_PORT_INTERFACE_MODE_XHGS_E or CPSS_PORT_INTERFACE_MODE_XHGS_SR_E */
    CPSS_PORT_SPEED_47200_E,    /* 17  */ /*used in combination with CPSS_PORT_INTERFACE_MODE_XHGS_E or CPSS_PORT_INTERFACE_MODE_XHGS_SR_E */
    CPSS_PORT_SPEED_22000_E,    /* 18  */ /*used in combination with CPSS_PORT_INTERFACE_MODE_XHGS_SR_E */
    CPSS_PORT_SPEED_23600_E,    /* 19  */ /*used in combination with CPSS_PORT_INTERFACE_MODE_XHGS_E or CPSS_PORT_INTERFACE_MODE_XHGS_SR_E */
    CPSS_PORT_SPEED_12500_E,    /* 20  */ /* used in combination with CPSS_PORT_INTERFACE_MODE_KR_E */
    CPSS_PORT_SPEED_25000_E,    /* 21  */ /* used in combination with CPSS_PORT_INTERFACE_MODE_KR_E, CPSS_PORT_INTERFACE_MODE_KR2_E */
    CPSS_PORT_SPEED_107G_E,     /* 22 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_KR4_E */
    CPSS_PORT_SPEED_29090_E,    /* 23 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_SR_LR_E */
    CPSS_PORT_SPEED_200G_E,     /* 24 */  /* Falcon new speed */
    CPSS_PORT_SPEED_400G_E,     /* 25 */  /* Falcon new speed */
    CPSS_PORT_SPEED_102G_E,     /* 26 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_KR4_E */
    CPSS_PORT_SPEED_52500_E,    /* 27 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_KR2_E */
    CPSS_PORT_SPEED_26700_E,    /* 28 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_KR_E */
    CPSS_PORT_SPEED_106G_E,     /* 29 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_KR4_E */
    CPSS_PORT_SPEED_42000_E,    /* 30 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_KR4_E */
    CPSS_PORT_SPEED_53000_E,    /* 31 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_KR2_E */
    CPSS_PORT_SPEED_424G_E,     /* 32 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_KR8_E */
    CPSS_PORT_SPEED_212G_E,     /* 33 */  /* used in combination with CPSS_PORT_INTERFACE_MODE_KR4_E */
    CPSS_PORT_SPEED_REMOTE_E,   /* 34 */     /* used for remote port configuration (For SIP6 devices only)*/
    CPSS_PORT_SPEED_NA_E,       /* 35 */
    CPSS_PORT_SPEED_NA_HCD_E = 0xFF   /* 255 */  /* Used to indicate we are waiting for HCD resolution in AP */

} CPSS_PORT_SPEED_ENT;

/**
* @enum CPSS_PORT_DUPLEX_ENT
 *
 * @brief Enumeration of port duplex modes
*/
typedef enum{

    /** full duplex mode */
    CPSS_PORT_FULL_DUPLEX_E,

    /** half duplex mode */
    CPSS_PORT_HALF_DUPLEX_E

} CPSS_PORT_DUPLEX_ENT;

/**
* @struct CPSS_DXCH_PORT_AUTONEG_ADVERTISMENT_STC
 *
 * @brief A struct containing parameters to build
 * <TX Config Reg> data for Auto-Negotiation.
*/

typedef struct{

    /** port Link Up if GT_TRUE, Link Down if GT_FALSE; */
    GT_BOOL link;

    /** port speed; */
    CPSS_PORT_SPEED_ENT speed;

    /** @brief port duplex mode;
     *  Comments:
     *  None
     */
    CPSS_PORT_DUPLEX_ENT duplex;

} CPSS_DXCH_PORT_AUTONEG_ADVERTISMENT_STC;

typedef struct {
    GT_BOOL inbandEnable;
    GT_BOOL duplexEnable;
    GT_BOOL speedEnable;
    GT_BOOL byPassEnable;
    GT_BOOL flowCtrlEnable;
    GT_BOOL flowCtrlPauseAdvertiseEnable;
    GT_BOOL flowCtrlAsmAdvertiseEnable;
    GT_BOOL readyToUpdFlag;
} CPSS_PORT_MANAGER_SGMII_AUTO_NEGOTIATION_STC;

typedef enum{
    CPSS_PORT_FLOW_CONTROL_DISABLE_E = GT_FALSE,
    CPSS_PORT_FLOW_CONTROL_RX_TX_E = GT_TRUE,
    CPSS_PORT_FLOW_CONTROL_RX_ONLY_E,
    CPSS_PORT_FLOW_CONTROL_TX_ONLY_E
} CPSS_PORT_FLOW_CONTROL_ENT;

typedef enum{
    CPSS_PORT_INTERFACE_MODE_REDUCED_10BIT_E,   /* 0 */
    CPSS_PORT_INTERFACE_MODE_REDUCED_GMII_E,    /* 1 */
    CPSS_PORT_INTERFACE_MODE_MII_E,             /* 2 */
    CPSS_PORT_INTERFACE_MODE_SGMII_E,           /* 3 */ /* CPSS_PORT_SPEED_1000_E , CPSS_PORT_SPEED_2500_E   */
    CPSS_PORT_INTERFACE_MODE_XGMII_E,           /* 4 */ /* CPSS_PORT_SPEED_10000_E, CPSS_PORT_SPEED_12000_E, CPSS_PORT_SPEED_16000_E, CPSS_PORT_SPEED_20000_E, */
    CPSS_PORT_INTERFACE_MODE_MGMII_E,           /* 5 */
    CPSS_PORT_INTERFACE_MODE_1000BASE_X_E,      /* 6 */ /* CPSS_PORT_SPEED_1000_E, */
    CPSS_PORT_INTERFACE_MODE_GMII_E,            /* 7 */
    CPSS_PORT_INTERFACE_MODE_MII_PHY_E,         /* 8 */
    CPSS_PORT_INTERFACE_MODE_QX_E,              /* 9 */  /* CPSS_PORT_SPEED_2500_E,  CPSS_PORT_SPEED_5000_E,  */
    CPSS_PORT_INTERFACE_MODE_HX_E,              /* 10 */ /* CPSS_PORT_SPEED_5000_E,  CPSS_PORT_SPEED_10000_E, */
    CPSS_PORT_INTERFACE_MODE_RXAUI_E,           /* 11 */ /* CPSS_PORT_SPEED_10000_E  */
    CPSS_PORT_INTERFACE_MODE_100BASE_FX_E,      /* 12 */
    CPSS_PORT_INTERFACE_MODE_QSGMII_E,          /* 13 */ /* CPSS_PORT_SPEED_1000_E, */
    CPSS_PORT_INTERFACE_MODE_XLG_E,             /* 14 */
    CPSS_PORT_INTERFACE_MODE_LOCAL_XGMII_E,     /* 15 */
    CPSS_PORT_INTERFACE_MODE_NO_SERDES_PORT_E =
                                        CPSS_PORT_INTERFACE_MODE_LOCAL_XGMII_E,
    CPSS_PORT_INTERFACE_MODE_KR_E,              /* 16 */ /* CPSS_PORT_SPEED_10000_E, CPSS_PORT_SPEED_12000_E, CPSS_PORT_SPEED_20000_E, CPSS_PORT_SPEED_40000_E, CPSS_PORT_SPEED_100G_E, */
    CPSS_PORT_INTERFACE_MODE_HGL_E,             /* 17 */ /* CPSS_PORT_SPEED_15000_E, CPSS_PORT_SPEED_16000_E, CPSS_PORT_SPEED_40000_E */
    CPSS_PORT_INTERFACE_MODE_CHGL_12_E,         /* 18 */ /* CPSS_PORT_SPEED_100G_E , */
    CPSS_PORT_INTERFACE_MODE_ILKN12_E,          /* 19 */
    CPSS_PORT_INTERFACE_MODE_SR_LR_E,           /* 20 */ /* CPSS_PORT_SPEED_5000_E, CPSS_PORT_SPEED_10000_E, CPSS_PORT_SPEED_12000_E, CPSS_PORT_SPEED_20000_E, CPSS_PORT_SPEED_40000_E */
    CPSS_PORT_INTERFACE_MODE_ILKN16_E,          /* 21 */
    CPSS_PORT_INTERFACE_MODE_ILKN24_E,          /* 22 */
    CPSS_PORT_INTERFACE_MODE_ILKN4_E,           /* 23 */ /* CPSS_PORT_SPEED_12000_E, CPSS_PORT_SPEED_20000_E, */
    CPSS_PORT_INTERFACE_MODE_ILKN8_E,           /* 24 */ /* CPSS_PORT_SPEED_20000_E, CPSS_PORT_SPEED_40000_E, */
    CPSS_PORT_INTERFACE_MODE_XHGS_E,            /* 25 */ /* CPSS_PORT_SPEED_11800_E, CPSS_PORT_SPEED_23600_E, CPSS_PORT_SPEED_47200_E, */
    CPSS_PORT_INTERFACE_MODE_XHGS_SR_E,         /* 26 */ /* CPSS_PORT_SPEED_11800_E, CPSS_PORT_SPEED_47200_E, */
    CPSS_PORT_INTERFACE_MODE_KR2_E,             /* 27 */
    CPSS_PORT_INTERFACE_MODE_KR4_E,             /* 28 */
    CPSS_PORT_INTERFACE_MODE_SR_LR2_E,          /* 29 */ /* CPSS_PORT_SPEED_50000_E*/
    CPSS_PORT_INTERFACE_MODE_SR_LR4_E,              /* 30 */ /* CPSS_PORT_SPEED_100G_E  */
    CPSS_PORT_INTERFACE_MODE_MLG_40G_10G_40G_10G_E, /* 31 */ /* Multi-Link Gearbox speeds: 40G, 10G, 40G, 10G */
    CPSS_PORT_INTERFACE_MODE_KR_C_E,            /* 32 */ /*CONSORTIUM - CPSS_PORT_SPEED_25000_E*/
    CPSS_PORT_INTERFACE_MODE_CR_C_E,            /* 33 */ /*CONSORTIUM - CPSS_PORT_SPEED_25000_E*/
    CPSS_PORT_INTERFACE_MODE_KR2_C_E,           /* 34 */ /*CONSORTIUM - CPSS_PORT_SPEED_50000_E*/
    CPSS_PORT_INTERFACE_MODE_CR2_C_E,           /* 35 */ /*CONSORTIUM - CPSS_PORT_SPEED_50000_E*/
    CPSS_PORT_INTERFACE_MODE_CR_E,              /* 36 */
    CPSS_PORT_INTERFACE_MODE_CR2_E,             /* 37 */
    CPSS_PORT_INTERFACE_MODE_CR4_E,             /* 38 */
    CPSS_PORT_INTERFACE_MODE_KR_S_E,            /* 39 */
    CPSS_PORT_INTERFACE_MODE_CR_S_E,            /* 40 */
    CPSS_PORT_INTERFACE_MODE_KR8_E,             /* 41 */
    CPSS_PORT_INTERFACE_MODE_CR8_E,             /* 42 */
    CPSS_PORT_INTERFACE_MODE_SR_LR8_E,          /* 43 */
    /* USX 1 Lane */
    CPSS_PORT_INTERFACE_MODE_USX_2_5G_SXGMII_E, /* 44 */ /* 1 Port -> 10M/100M/1G/2.5G/5G       with SD speed 2.578125G */
    CPSS_PORT_INTERFACE_MODE_USX_5G_SXGMII_E,   /* 45 */ /* 1 Port -> 10M/100M/1G/2.5G/5G       with SD speed 5.15625G */
    CPSS_PORT_INTERFACE_MODE_USX_10G_SXGMII_E,  /* 46 */ /* 1 Port -> 10M/100M/1G/2.5G/5G/10G   with SD speed 10.3125G */
    /* USX 2 Lanes */
    CPSS_PORT_INTERFACE_MODE_USX_5G_DXGMII_E,   /* 47 */ /* 2 Ports -> 10M/100M/1G/2.5G         with SD speed 5.15625G */
    CPSS_PORT_INTERFACE_MODE_USX_10G_DXGMII_E,  /* 48 */ /* 2 Ports -> 10M/100M/1G/2.5G/5G      with SD speed 10.3125G */
    CPSS_PORT_INTERFACE_MODE_USX_20G_DXGMII_E,  /* 49 */ /* 2 Ports -> 10M/100M/1G/2.5G/5G/10G  with SD speed 20.625G */
    /* USX 4 Lanes */
    CPSS_PORT_INTERFACE_MODE_USX_QUSGMII_E,     /* 50 */ /* Like QSGMII but with PCH - maximum of 4 ports, CPSS_PORT_SPEED_1000_E */
    CPSS_PORT_INTERFACE_MODE_USX_10G_QXGMII_E,  /* 51 */ /* 4 Ports -> 10M/100M/1G/2.5G         with SD speed 10.3125G */
    CPSS_PORT_INTERFACE_MODE_USX_20G_QXGMII_E,  /* 52 */ /* 4 Ports -> 10M/100M/1G/2.5G/5G      with SD speed 20.625G */
    /* USX 8 Lanes */
    CPSS_PORT_INTERFACE_MODE_USX_OUSGMII_E,     /* 53 */ /* Maximum of 8 ports with PCH, CPSS_PORT_SPEED_1000_E, SD speed 10G */
    CPSS_PORT_INTERFACE_MODE_USX_20G_OXGMII_E,  /* 54 */ /* 8 Ports -> 10M/100M/1G/2.5G         with SD speed 20.625G */
    CPSS_PORT_INTERFACE_MODE_2500BASE_X_E,      /* 55 */ /* CPSS_PORT_SPEED_2500_E, */
    CPSS_PORT_INTERFACE_MODE_REMOTE_E,          /* 56 */ /* used for remote ports configuration.(For SIP6 devices only) */
    CPSS_PORT_INTERFACE_MODE_NA_E,              /* 57 */
    CPSS_PORT_INTERFACE_MODE_NA_HCD_E = 0xFF    /* 255 */ /* Used to indicate we are waiting for HCD resolution in AP */
}CPSS_PORT_INTERFACE_MODE_ENT;

typedef enum{
    CPSS_PORT_AP_FLOW_CONTROL_SYMMETRIC_E,
    CPSS_PORT_AP_FLOW_CONTROL_ASYMMETRIC_E
} CPSS_PORT_AP_FLOW_CONTROL_ENT;

typedef enum{
    CPSS_DXCH_PORT_AP_FLOW_CONTROL_SYMMETRIC_E = CPSS_PORT_AP_FLOW_CONTROL_SYMMETRIC_E,
    CPSS_DXCH_PORT_AP_FLOW_CONTROL_ASYMMETRIC_E = CPSS_PORT_AP_FLOW_CONTROL_ASYMMETRIC_E
} CPSS_DXCH_PORT_AP_FLOW_CONTROL_ENT;

typedef struct{
    CPSS_PORT_INTERFACE_MODE_ENT ifMode;
    CPSS_PORT_SPEED_ENT speed;
} CPSS_PORT_MODE_SPEED_STC;

typedef enum{
    CPSS_PORT_FEC_MODE_ENABLED_E,
    CPSS_PORT_FEC_MODE_DISABLED_E,
    CPSS_PORT_RS_FEC_MODE_ENABLED_E,
    CPSS_PORT_BOTH_FEC_MODE_ENABLED_E,
    CPSS_PORT_RS_FEC_544_514_MODE_ENABLED_E,
    CPSS_PORT_FEC_MODE_LAST_E
} CPSS_PORT_FEC_MODE_ENT;

typedef enum{
    CPSS_DXCH_PORT_FEC_MODE_ENABLED_E = CPSS_PORT_FEC_MODE_ENABLED_E,
    CPSS_DXCH_PORT_FEC_MODE_DISABLED_E = CPSS_PORT_FEC_MODE_DISABLED_E,
    CPSS_DXCH_PORT_RS_FEC_MODE_ENABLED_E = CPSS_PORT_RS_FEC_MODE_ENABLED_E,
    CPSS_DXCH_PORT_BOTH_FEC_MODE_ENABLED_E = CPSS_PORT_BOTH_FEC_MODE_ENABLED_E,
    CPSS_DXCH_PORT_RS_FEC_544_514_MODE_ENABLED_E = CPSS_PORT_RS_FEC_544_514_MODE_ENABLED_E,
    /* Last */
    CPSS_DXCH_PORT_FEC_MODE_LAST_E = CPSS_PORT_FEC_MODE_LAST_E
}CPSS_DXCH_PORT_FEC_MODE_ENT;

typedef struct{
    GT_BOOL fcPause;
    CPSS_DXCH_PORT_AP_FLOW_CONTROL_ENT fcAsmDir;
    GT_BOOL fecSupported;
    GT_BOOL fecRequired;
    GT_BOOL noneceDisable;
    GT_U32 laneNum;
    CPSS_PORT_MODE_SPEED_STC modesAdvertiseArr [CPSS_DXCH_PORT_AP_IF_ARRAY_SIZE_CNS];
    CPSS_DXCH_PORT_FEC_MODE_ENT fecAbilityArr [CPSS_DXCH_PORT_AP_IF_ARRAY_SIZE_CNS];
    CPSS_DXCH_PORT_FEC_MODE_ENT fecRequestedArr [CPSS_DXCH_PORT_AP_IF_ARRAY_SIZE_CNS];
} CPSS_DXCH_PORT_AP_PARAMS_STC;


typedef enum{

    CPSS_PORT_MANAGER_EVENT_CREATE_E,   /* 0 */
    CPSS_PORT_MANAGER_EVENT_DELETE_E,   /* 1 */
    CPSS_PORT_MANAGER_EVENT_ENABLE_E,   /* 2 */
    CPSS_PORT_MANAGER_EVENT_DISABLE_E,   /* 3 */
    CPSS_PORT_MANAGER_EVENT_INIT_E,   /* 4 */
    CPSS_PORT_MANAGER_EVENT_LOW_LEVEL_STATUS_CHANGED_E,   /* 5 */
    CPSS_PORT_MANAGER_EVENT_MAC_LEVEL_STATUS_CHANGED_E,   /* 6 */
    CPSS_PORT_MANAGER_EVENT_PORT_AP_HCD_FOUND_E,          /* 7 */
    CPSS_PORT_MANAGER_EVENT_CREATE_AND_DISABLE_E,           /* 8 */
    CPSS_PORT_MANAGER_EVENT_REMOTE_FAULT_TX_CHANGE_E,       /* 9 */
    CPSS_PORT_MANAGER_EVENT_PORT_AP_DISABLE_E,              /* 10 */
    CPSS_PORT_MANAGER_EVENT_PORT_DEBUG_E,                   /* 11 */
    CPSS_PORT_MANAGER_EVENT_PORT_AP_RESTART_E,              /* 12 */
    CPSS_PORT_MANAGER_EVENT_PORT_NO_DEBUG_E,                /* 13 */
    CPSS_PORT_MANAGER_EVENT_CREATE_AND_FORCE_LINK_DOWN_E,   /* 14 */
    CPSS_PORT_MANAGER_EVENT_FORCE_LINK_DOWN_E,              /* 15 */
    CPSS_PORT_MANAGER_EVENT_UNFORCE_LINK_DOWN_E,            /* 16 */
    CPSS_PORT_MANAGER_EVENT_PORT_AP_PARALLEL_DETECT_E,    /* 17 */
    CPSS_PORT_MANAGER_EVENT_TYPE_LAST_E
} CPSS_PORT_MANAGER_EVENT_ENT;


typedef struct{

    /** @brief Event which is used to operate the port within the
      *  port manager state machine
      *  Comments:
      *  None.
      */
    CPSS_PORT_MANAGER_EVENT_ENT portEvent;

} CPSS_PORT_MANAGER_STC;

typedef enum {
    CPSS_HW_PP_RESET_SKIP_TYPE_REGISTER_E = 0,
    CPSS_HW_PP_RESET_SKIP_TYPE_TABLE_E,
    CPSS_HW_PP_RESET_SKIP_TYPE_EEPROM_E,
    CPSS_HW_PP_RESET_SKIP_TYPE_PEX_E,
    CPSS_HW_PP_RESET_SKIP_TYPE_LINK_LOSS_E,
    CPSS_HW_PP_RESET_SKIP_TYPE_CHIPLETS_E,
    CPSS_HW_PP_RESET_SKIP_TYPE_POE_E,
    CPSS_HW_PP_RESET_SKIP_TYPE_ALL_E,
    CPSS_HW_PP_RESET_SKIP_TYPE_ALL_EXCLUDE_PEX_E
} CPSS_HW_PP_RESET_SKIP_TYPE_ENT;

 /**
 * Typedef enum CPSS_SYSTEM_RECOVERY_STATE_ENT
 *
 * @brief : Indicates in which state system recovery process is.
 *
 */
typedef enum
{
    /** system is preparing to recovery process*/
    CPSS_SYSTEM_RECOVERY_PREPARATION_STATE_E,
    /** system is going through recovery init */
    CPSS_SYSTEM_RECOVERY_INIT_STATE_E,
    /** system recovery is completed */
    CPSS_SYSTEM_RECOVERY_COMPLETION_STATE_E,
    /** system is going to make hw catch up */
    CPSS_SYSTEM_RECOVERY_HW_CATCH_UP_STATE_E
} CPSS_SYSTEM_RECOVERY_STATE_ENT;

 /**
 * Typedef enum CPSS_SYSTEM_RECOVERY_PROCESS_ENT
 *
 * @brief : Indicates which system recovery process is running.
 *
 */
typedef enum
{
    /** HSU process */
    CPSS_SYSTEM_RECOVERY_PROCESS_HSU_E,
    /** Fast Boot process */
    CPSS_SYSTEM_RECOVERY_PROCESS_FAST_BOOT_E,
    /** recovery process after High Availability event */
    CPSS_SYSTEM_RECOVERY_PROCESS_HA_E,
    /** no active system recovery process */
    CPSS_SYSTEM_RECOVERY_PROCESS_NOT_ACTIVE_E,
    /** recovery process after High Availability event with multi processes */
    CPSS_SYSTEM_RECOVERY_PROCESS_PARALLEL_HA_E,
    /** hitless startup process */
    CPSS_SYSTEM_RECOVERY_PROCESS_HITLESS_STARTUP_E

} CPSS_SYSTEM_RECOVERY_PROCESS_ENT;

 /**
 * Typedef enum CPSS_SYSTEM_RECOVERY_MANAGER_ENT
 *
 * @brief : Indicates which system recovery manager is handle ,used for parallel High Availability.
 *
 */
typedef enum
{
    /** indicate in completion state to sync only non managers units */
    CPSS_SYSTEM_RECOVERY_NO_MANAGERS_E,
    /** action take place only for port manager*/
    CPSS_SYSTEM_RECOVERY_PORT_MANAGER_E,
    /** action take place only for fdb manager*/
    CPSS_SYSTEM_RECOVERY_FDB_MANAGER_E,
    /** action take place only for lpm manager*/
    CPSS_SYSTEM_RECOVERY_LPM_MANAGER_E,
    /** action take place only for tcam manager*/
    CPSS_SYSTEM_RECOVERY_TCAM_MANAGER_E,
    /** action take place only for exact match manager*/
    CPSS_SYSTEM_RECOVERY_EXACT_MATCH_MANAGER_E,
    /** action take place only for trunk manager*/
    CPSS_SYSTEM_RECOVERY_TRUNK_MANAGER_E,
    CPSS_SYSTEM_RECOVERY_LAST_MANAGER_E
} CPSS_SYSTEM_RECOVERY_MANAGER_ENT;


/**
 * Typedef enum CPSS_SYSTEM_RECOVERY_HA_2_PHASES_INIT_ENT
 *
 * @brief : Describes in which phase of HA_2_PHASES_INIT procedure system
 * recovery process is.
 *
 */
typedef enum
{
    /** HA_2_PHASES_INIT is not running */
    CPSS_SYSTEM_RECOVERY_HA_2_PHASES_INIT_NONE_E,
    /** phase1 of HA_2_PHASES_INIT is running */
    CPSS_SYSTEM_RECOVERY_HA_2_PHASES_INIT_PHASE1_E,
    /** phase2 of HA_2_PHASES_INIT is running */
    CPSS_SYSTEM_RECOVERY_HA_2_PHASES_INIT_PHASE2_E
} CPSS_SYSTEM_RECOVERY_HA_2_PHASES_INIT_ENT;



/**
 * Typedef enum CPSS_SYSTEM_RECOVERY_HA_STATE_ENT
 *
 * @brief : Describes in which state PEX during HA recovery process is.
 *
 */
typedef enum
{
    /** all operations via PEX are enabled */
    CPSS_SYSTEM_RECOVERY_HA_STATE_READ_ENABLE_WRITE_ENABLE_E,
    /** only read operations via PEX are enabled */
    CPSS_SYSTEM_RECOVERY_HA_STATE_READ_ENABLE_WRITE_DISABLE_E,
    /** only write operations via PEX are enabled */
    CPSS_SYSTEM_RECOVERY_HA_STATE_READ_DISABLE_WRITE_ENABLE_E,
    /** all operations via PEX are disabled */
    CPSS_SYSTEM_RECOVERY_HA_STATE_READ_DISABLE_WRITE_DISABLE_E

} CPSS_SYSTEM_RECOVERY_HA_STATE_ENT;


/**
* @struct CPSS_SYSTEM_RECOVERY_MODE_STC
 *
 * @brief This struct containes description of system recovery modes.
*/
typedef struct{

    /** @brief This mode describes what memory allocation is for Rx descriptors and Rx buffers:
     *  GT_TRUE: Rx descriptors and Rx buffers are allocated in the same memory
     *  before and after system recovery process.
     *  GT_FALSE: Rx descriptors and Rx buffers are allocated in different memory
     *  before and after system recovery process.
     */
    GT_BOOL continuousRx;

    /** @brief This mode describes what memory allocation is for Tx descriptors:
     *  GT_TRUE: Tx descriptors are allocated in the same memory
     *  before and after system recovery process.
     *  GT_FALSE: Tx descriptors are allocated in different memories
     *  before and after system recovery process.
     */
    GT_BOOL continuousTx;

    /** @brief This mode describes what memory allocation is for AUQ descriptors:
     *  GT_TRUE: AUQ descriptors are allocated in the same memory
     *  before and after system recovery process.
     *  GT_FALSE: AUQ descriptors are allocated in different memories
     *  before and after system recovery process.
     */
    GT_BOOL continuousAuMessages;

    /** @brief This mode describes what memory allocation is for FUQ descriptors:
     *  GT_TRUE: FUQ descriptors are allocated in the same memory
     *  before and after system recovery process.
     *  GT_FALSE: FUQ descriptors are allocated in different memories
     *  before and after system recovery process.
     */
    GT_BOOL continuousFuMessages;

    /** @brief GT_TRUE: special mode after HA event when PP can't access CPU memory but CPU can access PP.
     *  - GT_FALSE: regular mode; both CPU and PP can access each other memories.
     *  It is actually when not the same memory address spaces are used for AUQ/FUQ/RX messages
     *  before and after HA event.
     *  This mode should be set by Application after HA event before any usage of
     *  AUQ/FUQ/RX memories allocated before HA event, in order to prevent sending by PP
     *  messages to these memories already not related to PP.
     *  It is obligement of Application to revert memory access to normal mode after cpssInit.
     *  (During cpssInit PP is initialized with new AUQ/FUQ/RX pointers )
     */
    GT_BOOL haCpuMemoryAccessBlocked;

    /** @brief  Describes  HA cpss init mode */
    CPSS_SYSTEM_RECOVERY_HA_2_PHASES_INIT_ENT ha2phasesInitPhase;

    /** @brief Describes HA PEX state */
    CPSS_SYSTEM_RECOVERY_HA_STATE_ENT haReadWriteState;

} CPSS_SYSTEM_RECOVERY_MODE_STC;

/**
* @struct CPSS_SYSTEM_RECOVERY_INFO_STC
 *
 * @brief This struct containes information about system recovery process.
*/
typedef struct{

    /** Indicates in which state system recovery process is */
    CPSS_SYSTEM_RECOVERY_STATE_ENT systemRecoveryState;

    /** Indicates in which working mode recovery process is. */
    CPSS_SYSTEM_RECOVERY_MODE_STC systemRecoveryMode;

    /** Indicates which recovery process is taking place. */
    CPSS_SYSTEM_RECOVERY_PROCESS_ENT systemRecoveryProcess;

} CPSS_SYSTEM_RECOVERY_INFO_STC;

/*
 * Typedef: GT_TRUNK_ID
 *
 * Description: Defines trunk id
 *
 *  used as the type for the trunk Id's
 *
 */
typedef GT_U16  GT_TRUNK_ID;


/*
 * Typedef: GT_HW_DEV_NUM
 *
 * Description: Defines HW dev num
 *
 *  used as the type for the hw dev num
 *
 */
typedef GT_U32 GT_HW_DEV_NUM;

/*
 * Typedef: GT_PORT_NUM
 *
 * Description: Defines port num
 *
 *  used as the type for the port num
 *
 */
typedef GT_U32 GT_PORT_NUM;

/**
* @enum CPSS_INTERFACE_TYPE_ENT
 *
 * @brief enumerator for interface
 * To be used for:
 * 1. set mac entry info
 * 2. set next hop info
 * 3. redirect pcl info
 * 4. get new Address info
*/
typedef enum{

    /** the interface is of port type (dev,port) */
    CPSS_INTERFACE_PORT_E = 0,

    /** the interface is of trunk type (trunkId) */
    CPSS_INTERFACE_TRUNK_E,

    /** the interface is of Vidx type (vidx) */
    CPSS_INTERFACE_VIDX_E,

    /** the interface is of Vid type (vlan-id) */
    CPSS_INTERFACE_VID_E,

    /** the interface is device */
    CPSS_INTERFACE_DEVICE_E,

    /** the interface is of Vidx type (fabricVidx) */
    CPSS_INTERFACE_FABRIC_VIDX_E,

    /** the interface index type */
    CPSS_INTERFACE_INDEX_E

} CPSS_INTERFACE_TYPE_ENT;

/**
* @struct CPSS_INTERFACE_INFO_STC
 *
 * @brief Defines the interface info
 * To be used for:
 * 1. set mac entry info
 * 2. set next hop info
 * 3. redirect pcl info
 * 4. get new Address info
*/
typedef struct{
    /** the interface type */
    CPSS_INTERFACE_TYPE_ENT     type;

    /* !!!! NOTE : next fields treated as UNION !!!! */

    /** info about the {dev,port} - relevant to CPSS_INTERFACE_PORT_E */
    struct{
        /** @brief - the HW device number */
        GT_HW_DEV_NUM   hwDevNum;
        /** @brief - port number (physical port / eport) */
        GT_PORT_NUM     portNum;
    }devPort;

    /** info about the {trunkId}  - relevant to CPSS_INTERFACE_TRUNK_E */
    GT_TRUNK_ID     trunkId;

    /** info about the {vidx}     - relevant to CPSS_INTERFACE_VIDX_E */
    GT_U16          vidx;

    /** info about the {vid}      - relevant to CPSS_INTERFACE_VID_E */
    GT_U16          vlanId;

    /** info about the {HW device number} - relevant to CPSS_INTERFACE_DEVICE_E */
    GT_HW_DEV_NUM   hwDevNum;

    /** info about the {fabricVidx} - relevant to CPSS_INTERFACE_FABRIC_VIDX_E */
    GT_U16          fabricVidx;

    /** info about the {index}  - relevant to CPSS_INTERFACE_INDEX_E */
    GT_U32          index;
}CPSS_INTERFACE_INFO_STC;

/**
* @enum CPSS_PACKET_CMD_ENT
 *
 * @brief This enum defines the packet command.
*/
typedef enum{

    /** forward packet */
    CPSS_PACKET_CMD_FORWARD_E              ,

    /** mirror packet to CPU */
    CPSS_PACKET_CMD_MIRROR_TO_CPU_E        ,

    /** trap packet to CPU */
    CPSS_PACKET_CMD_TRAP_TO_CPU_E          ,

    /** hard drop packet */
    CPSS_PACKET_CMD_DROP_HARD_E            ,

    /** soft drop packet */
    CPSS_PACKET_CMD_DROP_SOFT_E            ,

    /** IP Forward the packets */
    CPSS_PACKET_CMD_ROUTE_E                ,

    /** @brief Packet is routed and mirrored to
     *  the CPU.
     */
    CPSS_PACKET_CMD_ROUTE_AND_MIRROR_E     ,

    /** Bridge and Mirror to CPU. */
    CPSS_PACKET_CMD_BRIDGE_AND_MIRROR_E    ,

    /** Bridge only */
    CPSS_PACKET_CMD_BRIDGE_E               ,

    /** Do nothing. (disable) */
    CPSS_PACKET_CMD_NONE_E                 ,

    /** loopback packet is send back to originator */
    CPSS_PACKET_CMD_LOOPBACK_E             ,

    /** same as CPSS_PACKET_CMD_ROUTE_E but packet can be failed by loose uRPF. */
    CPSS_PACKET_CMD_DEFAULT_ROUTE_ENTRY_E

} CPSS_PACKET_CMD_ENT;



/**
* @enum CPSS_DROP_MODE_TYPE_ENT
 *
 * @brief Enumeration for drop mode for red packets
*/
typedef enum{

    /** drop mode is Soft drop */
    CPSS_DROP_MODE_SOFT_E = 0,

    /** drop mode is hard drop */
    CPSS_DROP_MODE_HARD_E

} CPSS_DROP_MODE_TYPE_ENT;

/**
* @enum CPSS_PACKET_ATTRIBUTE_MODIFY_TYPE_ENT
 *
 * @brief Enumerator for modification of packet's attribute
 * like User Priority and DSCP.
*/
typedef enum{

    /** @brief Keep
     *  previous packet's attribute modification command.
     *  CPSS_PACKET_ATTRIBUTE_MODIFY_DISABLE_E,    - disable
     *  modification of the packet's attribute.
     */
    CPSS_PACKET_ATTRIBUTE_MODIFY_KEEP_PREVIOUS_E = 0,

    CPSS_PACKET_ATTRIBUTE_MODIFY_DISABLE_E,

    /** @brief enable
     *  modification of the packet's attribute.
     */
    CPSS_PACKET_ATTRIBUTE_MODIFY_ENABLE_E

} CPSS_PACKET_ATTRIBUTE_MODIFY_TYPE_ENT;

/**
* @enum CPSS_PACKET_ATTRIBUTE_ASSIGN_PRECEDENCE_ENT
 *
 * @brief Enumerator for the packet's attribute assignment precedence
 * for the subsequent assignment mechanism.
*/
typedef enum{

    /** @brief Soft precedence:
     *  The packet's attribute assignment can be overridden
     *  by the subsequent assignment mechanism
     */
    CPSS_PACKET_ATTRIBUTE_ASSIGN_PRECEDENCE_SOFT_E = 0,

    /** @brief Hard precedence:
     *  The packet's attribute assignment is locked
     *  to the last value of attribute assigned to the packet
     *  and cannot be overridden.
     */
    CPSS_PACKET_ATTRIBUTE_ASSIGN_PRECEDENCE_HARD_E

} CPSS_PACKET_ATTRIBUTE_ASSIGN_PRECEDENCE_ENT;

/**
* @enum CPSS_PACKET_ATTRIBUTE_ASSIGN_CMD_ENT
 *
 * @brief Enumerator for the packet's attribute assignment command.
*/
typedef enum{

    /** @brief packet's attribute assignment
     *  disabled
     */
    CPSS_PACKET_ATTRIBUTE_ASSIGN_DISABLED_E,

    /** @brief packet's attribute assignment
     *  only if the packet is VLAN tagged.
     */
    CPSS_PACKET_ATTRIBUTE_ASSIGN_FOR_TAGGED_E,

    /** @brief packet's attribute assignment
     *  only if the packet is untagged or Prioritytagged.
     */
    CPSS_PACKET_ATTRIBUTE_ASSIGN_FOR_UNTAGGED_E,

    /** @brief packet's attribute assignment
     *  regardless of packet tagging state.
     */
    CPSS_PACKET_ATTRIBUTE_ASSIGN_FOR_ALL_E

} CPSS_PACKET_ATTRIBUTE_ASSIGN_CMD_ENT;

/**
* @enum CPSS_IP_SITE_ID_ENT
 *
 * @brief This enum defines IPv6 Site ID (Used by Router for Ipv6 scope checking)
*/
typedef enum{

    /** Internal */
    CPSS_IP_SITE_ID_INTERNAL_E,

    /** External */
    CPSS_IP_SITE_ID_EXTERNAL_E

} CPSS_IP_SITE_ID_ENT;

/**
* @enum CPSS_IP_CNT_SET_ENT
 *
 * @brief Each UC/MC Route Entry result can be linked with one the
 * below IP counter sets.
*/
typedef enum{

    /** counter set 0 */
    CPSS_IP_CNT_SET0_E   = 0,

    /** counter set 1 */
    CPSS_IP_CNT_SET1_E   = 1,

    /** counter set 2 */
    CPSS_IP_CNT_SET2_E   = 2,

    /** counter set 3 */
    CPSS_IP_CNT_SET3_E   = 3,

    /** do not link route entry with a counter set */
    CPSS_IP_CNT_NO_SET_E = 4

} CPSS_IP_CNT_SET_ENT;

#include "esalCpssFdb.h"

/**
* @enum CPSS_DXCH_CFG_TABLES_ENT
 *
 * @brief the type of tables that the DXCH devices hold.
 * NOTE: some tables may shared on same memory space (like router and IPCL)
*/
typedef enum{

    /** table type represent the VLAN table */
    CPSS_DXCH_CFG_TABLE_VLAN_E,

    /** @brief  table type represent the FDB table
     *
     *   NOTE: 1. For next APPLICABLE DEVICES : Ironman.
     *          This refer to the FDB partition of the table that not include
     *          the DDE part. To have the FDB with the DDE part , use
     *          CPSS_DXCH_CFG_TABLE_FDB_WITH_DDE_PARTITION_E.
    */
    CPSS_DXCH_CFG_TABLE_FDB_E,

    /** table type represent the PCL action table */
    CPSS_DXCH_CFG_TABLE_PCL_ACTION_E,

    /** table type represent the PCL Tcam table */
    CPSS_DXCH_CFG_TABLE_PCL_TCAM_E,

    /** table type represent the router next hop table */
    CPSS_DXCH_CFG_TABLE_ROUTER_NEXT_HOP_E,

    /** table type represent the router lookup translation table (LTT) */
    CPSS_DXCH_CFG_TABLE_ROUTER_LTT_E,

    /** @brief table type represent the Router Tcam table
     *  Note: take in account that cpssDxChCfgTableNumEntriesGet
     *  function will return number of entries for IPv6,
     *  while actual number of IPv4 entries is 4 times bigger.
     */
    CPSS_DXCH_CFG_TABLE_ROUTER_TCAM_E,

    /** table type represent the L3 ECMP/QoS table */
    CPSS_DXCH_CFG_TABLE_ROUTER_ECMP_QOS_E,

    /** table type represent the TTI table */
    CPSS_DXCH_CFG_TABLE_TTI_TCAM_E,

    /** @brief table type represent the MLL pair table
     *  (the MLLs reside as pair in each entry)
     */
    CPSS_DXCH_CFG_TABLE_MLL_PAIR_E,

    /** table type represent the policer metering table */
    CPSS_DXCH_CFG_TABLE_POLICER_METERS_E,

    /** table type represent the policer billing counters table */
    CPSS_DXCH_CFG_TABLE_POLICER_BILLING_COUNTERS_E,

    /** table type represent the VIDX (multicast groups) table */
    CPSS_DXCH_CFG_TABLE_VIDX_E,

    /** table type represent the ARP entries in ARP/Tunnel Start table */
    CPSS_DXCH_CFG_TABLE_ARP_E,

    /** table type represent Tunnel Start entries in the ARP/Tunnel Start table */
    CPSS_DXCH_CFG_TABLE_TUNNEL_START_E,

    /** table type represent the STG (spanning tree groups) table */
    CPSS_DXCH_CFG_TABLE_STG_E,

    /** table type represent the QOS profile table */
    CPSS_DXCH_CFG_TABLE_QOS_PROFILE_E,

    /** table type represent the Mac to me table */
    CPSS_DXCH_CFG_TABLE_MAC_TO_ME_E,

    /** @brief table type represent the centralized counters (CNC) table
     *  (the number of counters (X per block) , so 8 blocks means 8X counters).
     *  Refer to device datasheet to see number of CNC blocks and the number of
     *  counters per CNC block.
     */
    CPSS_DXCH_CFG_TABLE_CNC_E,

    /** @brief table type represent CNC block (the number of conters per CNC block).
     *  Refer to device datasheet to see number of counters per CNC block.
     */
    CPSS_DXCH_CFG_TABLE_CNC_BLOCK_E,

    /** table type represent trunk table. */
    CPSS_DXCH_CFG_TABLE_TRUNK_E,

    /** table type represent LPM RAM */
    CPSS_DXCH_CFG_TABLE_LPM_RAM_E,

    /** table type represent router ECMP table */
    CPSS_DXCH_CFG_TABLE_ROUTER_ECMP_E,

    /** table type represent L2 MLL LTT */
    CPSS_DXCH_CFG_TABLE_L2_MLL_LTT_E,

    /** table type represent ePorts table */
    CPSS_DXCH_CFG_TABLE_EPORT_E,

    /** table type represent default ePorts table */
    CPSS_DXCH_CFG_TABLE_DEFAULT_EPORT_E,

    /** table type represent physical Ports table */
    CPSS_DXCH_CFG_TABLE_PHYSICAL_PORT_E,

    /** table type represent the Exact Match table
     *  (APPLICABLE DEVICES : FALCON.) */
    CPSS_DXCH_CFG_TABLE_EXACT_MATCH_E,

    /** table type represent the Source ID table*/
    CPSS_DXCH_CFG_TABLE_SOURCE_ID_E,

    /** table type represent OAM table. */
    CPSS_DXCH_CFG_TABLE_OAM_E,

    /** @brief : table type represent the DDE partition (in FDB table)
     *  (APPLICABLE DEVICES : Ironman.)
    */
    CPSS_DXCH_CFG_TABLE_DDE_PARTITION_E,

    /** @brief : table type represent the FDB table with the DDE partition
     *   this table is the summary of 'FDB' and 'DDE_PARTITION'
     *  (APPLICABLE DEVICES : Ironman.)
    */
    CPSS_DXCH_CFG_TABLE_FDB_WITH_DDE_PARTITION_E,


    /** indication of the last table (not represents a table) */
    CPSS_DXCH_CFG_TABLE_LAST_E

} CPSS_DXCH_CFG_TABLES_ENT;

extern GT_STATUS cpssDxChPhyPortSmiRegisterWrite(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum,
                    GT_U8 phyReg, uint16_t data);
extern GT_STATUS cpssDxChPhyPortSmiRegisterRead(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum,
                    GT_U8 phyReg, uint16_t *data);

extern GT_STATUS cpssDxChPortDuplexModeSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, int dMode);
extern GT_STATUS cpssDxChPortDuplexModeGet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, int *dModePtr);

extern GT_STATUS cpssDxChPortAutoNegAdvertismentConfigSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum,
                                                             CPSS_DXCH_PORT_AUTONEG_ADVERTISMENT_STC *portAnAdvertismentPtr);
extern GT_STATUS cpssDxChPortAutoNegAdvertismentConfigGet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum,
                                                             CPSS_DXCH_PORT_AUTONEG_ADVERTISMENT_STC *portAnAdvertismentPtr);

extern GT_STATUS cpssDxChPortInterfaceModeSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, CPSS_PORT_INTERFACE_MODE_ENT modev);
extern GT_STATUS cpssDxChSamplePortManagerMandatoryParamsSet
(
      GT_U8                           devNum,
      GT_PHYSICAL_PORT_NUM            portNum,
      CPSS_PORT_INTERFACE_MODE_ENT    ifMode,
      CPSS_PORT_SPEED_ENT             speed,
      CPSS_PORT_FEC_MODE_ENT          fecMode
);

extern GT_STATUS cpssDxChPortManagerEventSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum,  CPSS_PORT_MANAGER_STC   *portEventStc);


extern GT_STATUS cpssDxChPortInbandAutoNegEnableSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL enable);
extern GT_STATUS cpssDxChPortInbandAutoNegEnableGet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL *enablePtr);

extern GT_STATUS cpssDxChPortDuplexAutoNegEnableSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL  state);
extern GT_STATUS cpssDxChPortDuplexAutoNegEnableGet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL *statePtr);

extern GT_STATUS cpssDxChPortSpeedAutoNegEnableSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL state);
extern GT_STATUS cpssDxChPortSpeedAutoNegEnableGet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL *statePtr);

extern GT_STATUS cpssDxChPortInBandAutoNegBypassEnableSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL  enable);
extern GT_STATUS cpssDxChPortInBandAutoNegBypassEnableGet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL *enablePtr);

extern GT_STATUS cpssDxChPortFlowCntrlAutoNegEnableSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL  enable, GT_BOOL pauseAdvertise);
extern GT_STATUS cpssDxChPortFlowCntrlAutoNegEnableGet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, GT_BOOL *statePtr, GT_BOOL *pauseAdvertisePtr);

extern GT_STATUS cpssDxChPortFlowControlEnableSet(GT_U8 devNum,
                    GT_PHYSICAL_PORT_NUM portNum,
                    CPSS_PORT_FLOW_CONTROL_ENT  state);
extern GT_STATUS cpssDxChPortFlowControlEnableGet(GT_U8 devNum,
                    GT_PHYSICAL_PORT_NUM portNum,
                    CPSS_PORT_FLOW_CONTROL_ENT *statePtr);

extern GT_STATUS cpssDxChPortApPortConfigSet(GT_U8 devNum,
                    GT_PHYSICAL_PORT_NUM portNum, GT_BOOL apEnable,
                    CPSS_DXCH_PORT_AP_PARAMS_STC *apParamsPtr);
extern GT_STATUS cpssDxChPortApPortConfigGet(GT_U8 devNum,
                    GT_PHYSICAL_PORT_NUM portNum, GT_BOOL *apEnablePtr,
                    CPSS_DXCH_PORT_AP_PARAMS_STC *apParamsPtr);

extern GT_STATUS cpssDxChDiagDeviceTemperatureGet(GT_U8 devNum,
                     GT_32 *temperaturePtr);
extern GT_STATUS cpssSystemRecoveryStateSet(CPSS_SYSTEM_RECOVERY_INFO_STC *recovery_info);


extern GT_STATUS prvCpssDrvHwPpWriteRegister (GT_U8 devNum, GT_U32 regAddr, GT_U32 value);

extern GT_STATUS cpssDxChCfgTableNumEntriesGet(GT_U8 devNum, CPSS_DXCH_CFG_TABLES_ENT table, 
                                               GT_U32 *numEntriesPtr);
extern GT_STATUS cpssDxChBrgFdbMacEntryRead(GT_U8 devNum, GT_U32 index, GT_BOOL *validPtr,
                                            GT_BOOL                 *skipPtr,
                                            GT_BOOL                 *agedPtr,
                                            GT_HW_DEV_NUM           *associatedHwDevNumPtr,
                                            CPSS_MAC_ENTRY_EXT_STC  *entryPtr);

extern GT_STATUS cpssDxChHwPpSoftResetSkipParamSet(GT_U8 devNum,
                    CPSS_HW_PP_RESET_SKIP_TYPE_ENT skipType,
                    GT_BOOL skipEnable);

extern GT_STATUS cpssDxChHwPpSoftResetTrigger(GT_U8 devNum);

typedef enum{
    CPSS_PORT_MANAGER_FAILURE_NONE_E,
    CPSS_PORT_MANAGER_FAILURE_SIGNAL_STABILITY_FAILED_E,
    CPSS_PORT_MANAGER_FAILURE_TRAINING_FAILED_E,
    CPSS_PORT_MANAGER_FAILURE_ALIGNMENT_TIMER_EXPIRED_E,
    CPSS_PORT_MANAGER_FAILURE_CONFIDENCE_INTERVAL_TIMER_EXPIRED_E,
    CPSS_PORT_MANAGER_FAILURE_CREATE_PORT_FAILED_E,
    CPSS_PORT_MANAGER_FAILURE_LAST_E
} CPSS_PORT_MANAGER_FAILURE_ENT;

typedef enum{
    CPSS_PORT_MANAGER_STATE_RESET_E,                /* 0 */
    CPSS_PORT_MANAGER_STATE_LINK_DOWN_E,            /* 1 */
    CPSS_PORT_MANAGER_STATE_INIT_IN_PROGRESS_E,     /* 2 */
    CPSS_PORT_MANAGER_STATE_LINK_UP_E,              /* 3 */
    CPSS_PORT_MANAGER_STATE_MAC_LINK_DOWN_E,        /* 4 */
    CPSS_PORT_MANAGER_STATE_FAILURE_E,              /* 5 */
    CPSS_PORT_MANAGER_STATE_DEBUG_E,                /* 6 */
    CPSS_PORT_MANAGER_STATE_FORCE_LINK_DOWN_E,      /* 7 */
    CPSS_PORT_MANAGER_STATE_LAST_E
} CPSS_PORT_MANAGER_STATE_ENT;

typedef struct {
    CPSS_PORT_MANAGER_STATE_ENT portState;
    GT_BOOL portUnderOperDisable;
    CPSS_PORT_MANAGER_FAILURE_ENT failure;
    CPSS_PORT_INTERFACE_MODE_ENT ifMode;
    CPSS_PORT_SPEED_ENT speed;
    CPSS_PORT_FEC_MODE_ENT fecType;
    GT_BOOL remoteFaultConfig;
} CPSS_PORT_MANAGER_STATUS_STC;

extern GT_STATUS cpssDxChPortManagerStatusGet(
    GT_U8 devNum,  GT_PHYSICAL_PORT_NUM portNum,
    CPSS_PORT_MANAGER_STATUS_STC  *portStagePtr);

extern GT_STATUS cpssDxChBrgVlanNaToCpuEnable(
    GT_U8 devNum, GT_U16 vlanId, GT_BOOL enable);

extern GT_STATUS cpssDxChCfgDevEnableGet(GT_U8 devNum, GT_BOOL *enable);

extern GT_STATUS cpssHalWarmResetComplete(void);

#endif
}

#endif
