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

typedef int8_t      GT_8,   *GT_8_PTR;
typedef uint8_t     GT_U8,  *GT_U8_PTR;
typedef int32_t     GT_32,  *GT_32_PTR;
typedef uint32_t    GT_U32,  *GT_U32_PTR;

typedef GT_U32 GT_PHYSICAL_PORT_NUM;


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

extern GT_STATUS cpssDxChPortFlowControlEnableSet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, CPSS_PORT_FLOW_CONTROL_ENT  state);
extern GT_STATUS cpssDxChPortFlowControlEnableGet(GT_U8 devNum, GT_PHYSICAL_PORT_NUM portNum, CPSS_PORT_FLOW_CONTROL_ENT *statePtr);

#endif
}

#endif
