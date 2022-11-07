#ifndef _ESAL_CPSS_PORT_CTRL_H_
#define _ESAL_CPSS_PORT_CTRL_H_

#define CPSS_PORT_DFE_VALUES_ARRAY_SIZE_CNS    6

#define CPSS_PORT_DFE_AVAGO_VALUES_ARRAY_SIZE_CNS    13

/* size of PAM4 eyes array*/
#define CPSS_PAM4_EYES_ARRAY_SIZE_CNS 6

/**
* @enum MV_HWS_RX_TERMINATION
* @endinternal
*
* @brief   RX termination mode
*
*/
typedef enum
{
    CPSS_SERDES_TERMINATION_GND,
    CPSS_SERDES_TERMINATION_VDD,
    CPSS_SERDES_TERMINATION_FLOATING

}CPSS_SERDES_TERMINATION;

/**
 * @enum CPSS_PORT_SERDES_TYPE_ENT
 *
 * @brief Enumeration of port's serdes types.
*/
typedef enum{

    /** COMPHY_H - BobCat2, Lion2, xCat3 */
    CPSS_PORT_SERDES_COMPHY_H_E,

    /**  BobK, PIPE, Aldrin, AC3X, Aldrin2, BC3, Falcon */
    CPSS_PORT_SERDES_AVAGO_E,

    /**  AC5 */
    CPSS_PORT_SERDES_COMPHY_C12G_E,

    /**  AC5X, AC5P */
    CPSS_PORT_SERDES_COMPHY_C28G_E,

    /**  AC5P */
    CPSS_PORT_SERDES_COMPHY_C112G_E,

    /**  Harrier */
    CPSS_PORT_SERDES_COMPHY_C56G_E,

    CPSS_PORT_SERDES_TYPE_LAST_E

} CPSS_PORT_SERDES_TYPE_ENT;

/**
* @struct CPSS_PORT_COMPHY_SERDES_TX_CONFIG_STC
 *
 * @brief Port COMPHY_H SERDES TX configuration parameters.
*/
typedef struct{

    /** @brief Tx Driver output Amplitude/Attenuator: APPLICABLE RANGES: [0...31]
     *  In ComPhyH Serdes for xCat3; Lion2; Bobcat2 devices: Amplitude
     */
    GT_U32 txAmp;

    /** @brief Transmitter Amplitude Adjust: (GT_TRUE
     *  APPLICABLE DEVICES only for ComPhyH Serdes: xCat3; AC5; Lion2; Bobcat2 devices
     */
    GT_BOOL txAmpAdjEn;

    /** @brief Controls the Emphasis Amplitude for Gen0 bit rates
     *  for Lion2 device [0...15]
     */
    GT_32 emph0;

    /** @brief Controls the emphasis amplitude for Gen1 bit rates
     *  for Lion2 device [0...15]
     */
    GT_32 emph1;

    /** @brief Transmitter Amplitude Shift
    *   (APPLICABLE DEVICES: xCat3; AC5; Lion2; Bobcat2)
    *   (APPLICABLE RANGES: GT_TRUE - enable, GT_FALSE - disable)
    */
    GT_BOOL  txAmpShft;

    /** @brief Pre
     *  APPLICABLE DEVICES only for ComPhyH Serdes: xCat3; AC5; Lion2; Bobcat2 devices
     */
    GT_BOOL txEmphEn;

    /** @brief Pre
     *  APPLICABLE DEVICES only for ComPhyH Serdes: xCat3; AC5; Lion2; Bobcat2 devices
     */
    GT_BOOL txEmphEn1;

    /** @brief Tx Driver output amplitude
     *  txAmpAdj- Transmitter Amplitude Adjust (APPLICABLE DEVICES:
     *  BC2, Lion2 ) (APPLICABLE RANGES: 0..15)
     */
    GT_U32 txAmpAdj;

    /** @brief slewCtrlEn
     *  APPLICABLE DEVICES only for ComPhyH Serdes: xCat3; AC5; Bobcat2 devices
     */
    GT_BOOL slewCtrlEn;

    /** @brief slewRate
     *  APPLICABLE DEVICES only for ComPhyH Serdes: xCat3; AC5; Bobcat2 devices
     *  (APPLICABLE RANGES: 0..7)
     */
    GT_U32 slewRate;

} CPSS_PORT_COMPHY_SERDES_TX_CONFIG_STC;

/**
 * @struct CPSS_PORT_COMPHY_C12G_TX_CONFIG_STC;
 *
 * @brief Comphy C12G Tx tune parameters
*/
typedef struct
{
    GT_U32 pre;
    GT_U32 peak;
    GT_U32 post;
}CPSS_PORT_COMPHY_C12G_TX_CONFIG_STC;

/**
 * @struct CPSS_PORT_COMPHY_C28G_TX_CONFIG_STC;
 *
 * @brief Comphy C28G Tx tune parameters
*/
typedef struct
{
    GT_U32 pre;
    GT_U32 peak;
    GT_U32 post;
}CPSS_PORT_COMPHY_C28G_TX_CONFIG_STC;

/**
 * @struct CPSS_PORT_COMPHY_C56G_TX_CONFIG_STC
 *
 * @brief Comphy C56G Tx tune parameters
*/
typedef struct
{
    GT_32   pre2;
    GT_32   pre;
    GT_32   main;
    GT_32   post;
    GT_BOOL usr;
}CPSS_PORT_COMPHY_C56G_TX_CONFIG_STC;

/**
 * @struct CPSS_PORT_COMPHY_C112G_TX_CONFIG_STC
 *
 * @brief Comphy C112G Tx tune parameters
*/
typedef struct
{
    GT_32 pre2;
    GT_32 pre;
    GT_32 main;
    GT_32 post;
}CPSS_PORT_COMPHY_C112G_TX_CONFIG_STC;

/**
* @struct CPSS_PORT_AVAGO_SERDES_TX_CONFIG_STC
*
* @brief Port AVAGO SERDES TX configuration parameters
*/
typedef struct
{
    /** @brief Serdes Tx Attenuator
    *   (APPLICABLE DEVICES: Caelum and above)
    *   (APPLICABLE RANGES: 0...31)
    */
    GT_U32   atten;

    /** @brief Serdes Post-Cursor
    *   (APPLICABLE DEVICES: Caelum, Aldrin, AC3X, Aldrin2(for
    *                        Serdes 24-71), Pipe(for Serdes 0-11)
    *   (APPLICABLE RANGES: 31...31)
    *   APPLICABLE DEVICES: Bobcat3, Aldrin2(for Serdes 0-23),
    *                       Pipe(for ports 12-15)
    *   (APPLICABLE RANGES: 0...31)
    *   APPLICABLE DEVICE: Falcon
    *   (APPLICABLE RANGES: even values -18...18)
    */
    GT_32    post;

    /** @brief Serdes Pre-Cursor
    *   (APPLICABLE DEVICES: Caelum, Aldrin, AC3X, Aldrin2(for
    *                        Serdes 24-71), Pipe(for ports 0-11)
    *   (APPLICABLE RANGES: -31...31)
    *   (APPLICABLE DEVICES: Bobcat3, Aldrin2(for Serdes 0-23),
    *                        Pipe(for ports 12-15)
    *   (APPLICABLE RANGES: 0...31)
    *   (APPLICABLE DEVICE: Falcon
    *   (APPLICABLE RANGES: even values -10...10)
    */
    GT_32    pre;

    /** @brief Serdes Pre2-Cursor
    *   APPLICABLE DEVICE: Falcon
    *   (APPLICABLE RANGES -15...15)
    */
    GT_32    pre2;

    /** @brief Serdes Pre3-Cursor
    *   APPLICABLE DEVICE: Falcon
    *   (APPLICABLE RANGES [-1, 0, 1)
    */
    GT_32    pre3;

}CPSS_PORT_AVAGO_SERDES_TX_CONFIG_STC;

/**
* @union CPSS_PORT_SERDES_TX_CONFIG_UNT
 *
*  @brief includes a union for the different serdeses.
*/
typedef union {

    CPSS_PORT_COMPHY_SERDES_TX_CONFIG_STC comphy;
    CPSS_PORT_COMPHY_C12G_TX_CONFIG_STC   comphy_C12G;
    CPSS_PORT_COMPHY_C28G_TX_CONFIG_STC   comphy_C28G;
    CPSS_PORT_COMPHY_C56G_TX_CONFIG_STC   comphy_C56G;
    CPSS_PORT_COMPHY_C112G_TX_CONFIG_STC  comphy_C112G;
    CPSS_PORT_AVAGO_SERDES_TX_CONFIG_STC  avago;
}CPSS_PORT_SERDES_TX_CONFIG_UNT;

/**
* @struct CPSS_PORT_SERDES_TX_CONFIG_STC
 *
 * @brief Port SERDES TX configuration parameters.
*/
typedef struct{

    CPSS_PORT_SERDES_TYPE_ENT type;
    CPSS_PORT_SERDES_TX_CONFIG_UNT txTune;

} CPSS_PORT_SERDES_TX_CONFIG_STC;

/**
* @struct CPSS_PORT_COMPHY_SERDES_RX_CONFIG_STC
 *
 * @brief Port COMPHY SERDES RX configuration parameters.
*/
typedef struct{
    /** @brief dcGain controls the gain according to the previous
     *  received bit (one tap) and compensates for interconnect ISI
     *  and ILD (refer to the DFE_F1 field)
     *  (APPLICABLE DEVICES: xCat3; AC5)
     *  (APPLICABLE RANGES: 0..31)
     */
    GT_U32 dcGain;

    /** @brief bandwidth controls the BW according to the
     *  previous received bit (one tap) and compensates for
     *  interconnect ISI and ILD (refer to the DFE_F1 field)
     *  (APPLICABLE DEVICES: xCat3; AC5)
     *  (APPLICABLE RANGES: 0..31)
     */
    GT_U32 bandWidth;

    /** @brief A digital filter controls the gain according to the previous
     *  received bit (one tap) and compensates for interconnect ISI
     *  and ILD (refer to the DFE_F1 field)
     *  (APPLICABLE DEVICES: xCat3; AC5)
     *  (APPLICABLE RANGES: 0..31)
     */
    GT_U32 dfe;

    /** @brief mainly controls the low frequency gain (refer to the
    *   FFE_res_sel field)
    *   (APPLICABLE DEVICES: xCat3; AC5; Lion2; Bobcat2)
    *   (APPLICABLE RANGES: 0..7)
    */
    GT_U32  ffeR;

    /** @brief mainly controls the high frequency gain (refer to the
    *   FFE_cap_sel field)
    *   (APPLICABLE DEVICES: xCat3; AC5; Lion2; Bobcat2)
    *   (APPLICABLE RANGES: 0..15)
    */
    GT_U32  ffeC;

    /** @brief sampler (refer to the cal_os_ph_rd field)
     *  (APPLICABLE DEVICES: xCat3; AC5; Lion2; Bobcat2)
     */
    GT_U32  sampler;

    /** @brief Threshold that trips the Squelch detector peak
    *   differential amplitude (refer to the SQ_THRESH field)
    *   (APPLICABLE DEVICES: xCat3; AC5; Lion2; Bobcat2)
    *  (APPLICABLE RANGES: 0..15)
    */
    GT_U32  sqlch;

    /** @brief Align 90 Calibration Phase Offset(This is the external
     *  value used in place of the autocalibration value for
     *  rxclkalign90). (APPLICABLE RANGES: 0..127)
     */
    GT_U32 align90;

    /** @brief FFE signal swing control (APPLICABLE DEVICES: xCat3; AC5)
    *  (APPLICABLE RANGES: 0..3)
    */
    GT_U32 ffeS;

    /** @brief Adapted DFE Coefficient
    *   This field indicates the DFE auto-calibration and auto-trained results
    *   (APPLICABLE DEVICES: xCat3; AC5; Lion2; Bobcat2)
    */
    GT_32  dfeValsArray[CPSS_PORT_DFE_VALUES_ARRAY_SIZE_CNS];

}CPSS_PORT_COMPHY_SERDES_RX_CONFIG_STC;

/**
 * @struct CPSS_PORT_COMPHY_C12GP41P2V_RX_CONFIG_ST
 *
 * @brief This struct include RX data
*/
typedef struct
{
    /* Basic (CTLE) */
    GT_U32 resSel;
    GT_U32 resShift;
    GT_U32 capSel;

    GT_U8 ffeSettingForce;
    GT_U8 adaptedResSel;
    GT_U8 adaptedCapSel;

    /* Advanced (CDR) */
    GT_U32 selmufi;
    GT_U32 selmuff;
    GT_U32 selmupi;
    GT_U32 selmupf;

    GT_32 squelch;

    GT_U32 align90;
    GT_U32 sampler;
    GT_U32 slewRateCtrl0;
    GT_U32 slewRateCtrl1;
    GT_32  dfe[12];

    GT_U32 EO;

} CPSS_PORT_COMPHY_C12GP41P2V_RX_CONFIG_STC;

/** @struct CPSS_PORT_COMPHY_C28GP4_RX_CONFIG_STC
 *
 * @brief This struct include RX data
*/
typedef struct
{
    /* Basic (CTLE) */
    GT_U32 dataRate;
    GT_U32 res1Sel;
    GT_U32 res2Sel;
    GT_U32 cap1Sel;
    GT_U32 cap2Sel;

    /* Advanced (CDR) */
    GT_U32 selmufi;
    GT_U32 selmuff;
    GT_U32 selmupi;
    GT_U32 selmupf;

    /* Advanced (Thresholds) */
    GT_U32 midpointLargeThresKLane;
    GT_U32 midpointSmallThresKLane;
    GT_U32 midpointLargeThresCLane;
    GT_U32 midpointSmallThresCLane;

    /* Advanced (DFE) */
    GT_U32 dfeResF0aHighThresInitLane;
    GT_U32 dfeResF0aHighThresEndLane;

    GT_32  squelch;

    GT_U32 align90;
    GT_U32 sampler;
    GT_U32 slewRateCtrl0;
    GT_U32 slewRateCtrl1;
    GT_32  dfe[25];

    GT_U32  EO;

} CPSS_PORT_COMPHY_C28GP4_RX_CONFIG_STC;

/**
 * @struct CPSS_PORT_COMPHY_C56G_RX_CONFIG_STC
 *
 * @brief This struct include RX data
*/
typedef struct
{
    /* Basic (CTLE) */
    GT_U32 cur1Sel;
    GT_U32 rl1Sel;
    GT_U32 rl1Extra;
    GT_U32 res1Sel;
    GT_U32 cap1Sel;
    GT_U32 enMidfreq;
    GT_U32 cs1Mid;
    GT_U32 rs1Mid;
    GT_U32 cur2Sel;
    GT_U32 rl2Sel;
    GT_U32 rl2TuneG;
    GT_U32 res2Sel;
    GT_U32 cap2Sel;

    /* Advanced (CDR) */
    GT_U32 selmufi;
    GT_U32 selmuff;
    GT_U32 selmupi;
    GT_U32 selmupf;

    GT_32  squelch;
    GT_32  dfe[26];

} CPSS_PORT_COMPHY_C56G_RX_CONFIG_STC;

/**
 * @struct CPSS_PORT_COMPHY_C112G_RX_CONFIG_STC
 *
 * @brief This struct include RX data
*/
typedef struct
{
    /* Basic (CTLE) */
    /* 1st stage GM Main */
    GT_U32 current1Sel;
    GT_U32 rl1Sel;
    GT_U32 rl1Extra;
    GT_U32 res1Sel;
    GT_U32 cap1Sel;
    GT_U32 cl1Ctrl;
    GT_U32 enMidFreq;
    GT_U32 cs1Mid;
    GT_U32 rs1Mid;
    /* 1st stage TIA */
    GT_U32 rfCtrl;
    GT_U32 rl1TiaSel;
    GT_U32 rl1TiaExtra;
    GT_U32 hpfRSel1st;
    GT_U32 current1TiaSel;
    /* 2nd Stage */
    GT_U32 rl2Tune;
    GT_U32 rl2Sel;
    GT_U32 rs2Sel;
    GT_U32 current2Sel;
    GT_U32 cap2Sel;
    GT_U32 hpfRsel2nd;

    /* Advanced (CDR) */
    GT_U32 selmufi;
    GT_U32 selmuff;
    GT_U32 selmupi;
    GT_U32 selmupf;

    GT_U32  squelch;

    GT_U32 align90AnaReg;
    GT_32  align90;
    GT_U32 sampler;
    GT_U32 slewRateCtrl0;
    GT_U32 slewRateCtrl1;
    GT_32  dfe[40];

    GT_U32  EO;

} CPSS_PORT_COMPHY_C112G_RX_CONFIG_STC;

/**
* @struct CPSS_PORT_AVAGO_SERDES_RX_CONFIG_STC
 *
 * @brief Port AVAGO SERDES RX configuration parameters.
*/
typedef struct{



    /** @brief Threshold that trips the Squelch detector peak
    *   differential amplitude (refer to the SQ_THRESH field)
    *  In Caelum; Aldrin; AC3X; Bobcat3; Aldrin2; Pipe devices: mV [68...308]
    */
    GT_U32  sqlch;

    /** (APPLICABLE RANGES: 0..1)(APPLICABLE DEVICES, Aldrin, AC3X: Caelum, Pipe) */
    GT_U32 DC;

    /** (APPLICABLE RANGES: 0..1)(APPLICABLE DEVICES, Aldrin, AC3X: Caelum, Pipe) */
    GT_U32 LF;

    /** (APPLICABLE RANGES: 0..1)(APPLICABLE DEVICES, Aldrin, AC3X: Caelum, Pipe) */
    GT_U32 HF;

    /** (APPLICABLE RANGES: 0..1)(APPLICABLE DEVICES, Aldrin, AC3X: Caelum, Pipe) */
    GT_U32 BW;

    /** (APPLICABLE RANGES: 0..1)(APPLICABLE DEVICES, Aldrin, AC3X: Caelum, Pipe) */
    GT_U32 EO;

    /** @brief DFE values
    *   (APPLICABLE DEVICES: Caelum, Bobcat3, Aldrin, AC3X, Aldrin2;
    *   Pipe; Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: -15...15)
    */
    GT_32  DFE[CPSS_PORT_DFE_AVAGO_VALUES_ARRAY_SIZE_CNS];

    /** @brief CTLE gainshape1
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...3)
    */
    GT_U32  gainshape1;

    /** @brief CTLE gainshape2
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...3)
    */
    GT_U32  gainshape2;

    /** @brief CTLE Enable/Disable Short channel
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: GT_TRUE - enable, GT_FALSE - disable)
    */
    GT_U32  shortChannelEn;

    /** @brief DFE Gain Tap strength
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...255)
    */
    GT_U32  dfeGAIN;

    /** @brief DFE Gain Tap2 strength
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...255)
    */
    GT_U32  dfeGAIN2;

    /** @brief DFE BFLF
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...5)
    */
    GT_32  BFLF;

    /** @brief DFE BFHF
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...8)
    */
    GT_32  BFHF;

    /** @brief CTLE minLf
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...15)
    */
    GT_U32  minLf;

    /** @brief CTLE maxLf
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...15)
    */
    GT_U32  maxLf;

    /** @brief CTLE minHf
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...15)
    */
    GT_U32  minHf;

    /** @brief CTLE maxHf
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...15)
    */
    GT_U32  maxHf;

    /** @brief FFE minPre1
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...15)
    */
    GT_32   minPre1;

    /** @brief FFE maxPre1
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...15)
    */
    GT_32   maxPre1;

    /** @brief FFE minPre2
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: -10...10)
    */
    GT_32   minPre2;

    /** @brief FFE maxPre2
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: -10...10)
    */
    GT_32   maxPre2;

    /** @brief FFE minPost
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: -15...15)
    */
    GT_32   minPost;

    /** @brief FFE minPost
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: -15...15)
    */
    GT_32   maxPost;

    /** @brief FFE pre1
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: 0...15)
    */
    GT_32 pre1;

    /** @brief FFE pre2
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: -15...15)
    */
    GT_32 pre2;

    /** @brief FFE post1
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
    *   (APPLICABLE RANGES: -15...15)
    */
    GT_32 post1;

    /** @brief PAM4 values
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman;
    *   indices 0-2 : even (lower, middle, upper)
    *   indices 3-5 : odd  (lower, middle, upper)
    */
    GT_32  pam4EyesArr[CPSS_PAM4_EYES_ARRAY_SIZE_CNS];

    /** @brief termination
    *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman;
    *   (APPLICABLE RANGES: 0...2)
    */
    CPSS_SERDES_TERMINATION  termination;
    /** @brief coldEnvelope
     *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_U32                    coldEnvelope;
    /** @brief hotEnvelope
     *   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_U32                    hotEnvelope;
}CPSS_PORT_AVAGO_SERDES_RX_CONFIG_STC;

/**
* @union CPSS_PORT_SERDES_RX_CONFIG_UNT
 *
*  @brief includes a union for the different serdeses.
*/
typedef union {

    CPSS_PORT_COMPHY_SERDES_RX_CONFIG_STC       comphy;
    CPSS_PORT_COMPHY_C12GP41P2V_RX_CONFIG_STC   comphy_C12G;
    CPSS_PORT_COMPHY_C28GP4_RX_CONFIG_STC       comphy_C28G;
    CPSS_PORT_COMPHY_C56G_RX_CONFIG_STC         comphy_C56G;
    CPSS_PORT_COMPHY_C112G_RX_CONFIG_STC        comphy_C112G;
    CPSS_PORT_AVAGO_SERDES_RX_CONFIG_STC        avago;

}CPSS_PORT_SERDES_RX_CONFIG_UNT;

/**
* @struct CPSS_PORT_SERDES_RX_CONFIG_STC
 *
 * @brief Port SERDES RX configuration parameters.
*/
typedef struct{

    CPSS_PORT_SERDES_TYPE_ENT type;
    CPSS_PORT_SERDES_RX_CONFIG_UNT rxTune;

} CPSS_PORT_SERDES_RX_CONFIG_STC;

// * @retval GT_OK                    - on success.
// * @retval GT_BAD_PARAM             - on bad parameters
// * @retval GT_BAD_PTR               - serdesTxCfgPtr == NULL
// * @retval GT_NOT_APPLICABLE_DEVICE - on not applicable device
// * @retval GT_NOT_INITIALIZED       - if serdes was not intialized
extern GT_STATUS cpssDxChPortSerdesManualTxConfigSet
(
    GT_U8                                devNum,
    GT_PHYSICAL_PORT_NUM                 portNum,
    GT_U32                               laneNum,
    CPSS_PORT_SERDES_TX_CONFIG_STC  *serdesTxCfgPtr
);

// * @retval GT_OK                    - on success.
// * @retval GT_BAD_PARAM             - on bad parameters
// * @retval GT_NOT_APPLICABLE_DEVICE - on not applicable device
// * @retval GT_NOT_INITIALIZED       - if serdes was not intialized
// * @retval GT_OUT_OF_RANGE          - parameter out of range
extern GT_STATUS cpssDxChPortSerdesManualRxConfigSet
(
    GT_U8                                devNum,
    GT_PHYSICAL_PORT_NUM                 portNum,
    GT_U32                               laneNum,
    CPSS_PORT_SERDES_RX_CONFIG_STC  *serdesRxCfgPtr
);

#endif
