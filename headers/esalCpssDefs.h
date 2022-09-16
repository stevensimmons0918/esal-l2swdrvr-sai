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

extern unsigned int cpssDxChPhyPortSmiRegisterWrite(uint8_t devNum, uint32_t portNum,
                    uint8_t phyReg, uint16_t data);
extern unsigned int cpssDxChPhyPortSmiRegisterRead(uint8_t devNum, uint32_t portNum,
                    uint8_t phyReg, uint16_t *data);

extern unsigned int cpssDxChPortDuplexModeSet(uint8_t devNum, uint32_t portNum, int dMode);
extern unsigned int cpssDxChPortDuplexModeGet(uint8_t devNum, uint32_t portNum, int *dModePtr);
extern unsigned int cpssDxChPortAutoNegAdvertismentConfigSet(uint8_t devNum, uint32_t portNum,
                                                             CPSS_DXCH_PORT_AUTONEG_ADVERTISMENT_STC *portAnAdvertismentPtr);
extern unsigned int cpssDxChPortAutoNegAdvertismentConfigGet(uint8_t devNum, uint32_t portNum,
                                                             CPSS_DXCH_PORT_AUTONEG_ADVERTISMENT_STC *portAnAdvertismentPtr);
extern unsigned int cpssDxChPortInbandAutoNegEnableSet(uint8_t devNum, uint32_t portNum, int enable);
extern unsigned int cpssDxChPortInbandAutoNegEnableGet(uint8_t devNum, uint32_t portNum, int *enablePtr);

#endif
}

#endif
