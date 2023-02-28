#ifndef _ESAL_CPSS_FDB_H_
#define _ESAL_CPSS_FDB_H_


/**
* @enum CPSS_MAC_TABLE_CMD_ENT
 *
 * @brief Enumeration of MAC Table entry actions taken when a packet's
 * MAC address (DA/SA) matches this entry
*/
typedef enum{

    /** forward (if address is automatically learned) */
    CPSS_MAC_TABLE_FRWRD_E = 0,

    /** drop (filtering on destination/source address) */
    CPSS_MAC_TABLE_DROP_E,

    /** @brief intervention to CPU (may be dropped by other
     *  mechanisms)
     */
    CPSS_MAC_TABLE_INTERV_E,

    /** control (unconditionally trap to CPU) */
    CPSS_MAC_TABLE_CNTL_E,

    /** @brief mirror to the CPU (in addition to sending
     *  the packet to its destination) not
     *  supported in ExMx device
     */
    CPSS_MAC_TABLE_MIRROR_TO_CPU_E,

    /** @brief soft drop (does not prevent the packet from
     *  being sent to the CPU) supported in
     *  DxCh devices
     */
    CPSS_MAC_TABLE_SOFT_DROP_E

} CPSS_MAC_TABLE_CMD_ENT;

/**
* @enum CPSS_MAC_ENTRY_EXT_TYPE_ENT
 *
 * @brief This enum defines the type of the FDB Entry
*/
typedef enum{

    /** @brief The entry is MAC Address
     *  entry (hushed by Mac Address and vlan ID).
     */
    CPSS_MAC_ENTRY_EXT_TYPE_MAC_ADDR_E = 0,

    /** @brief The entry is IPv4 Multicast
     *  entry (IGMP Snooping).
     */
    CPSS_MAC_ENTRY_EXT_TYPE_IPV4_MCAST_E,

    /** @brief The entry is IPv6 Multicast
     *  entry (MLD Snooping).
     */
    CPSS_MAC_ENTRY_EXT_TYPE_IPV6_MCAST_E,

    /** @brief The entry is IPv4 Unicast entry.
     *  (APPLICABLE DEVICES: Bobcat2; Caelum; Aldrin; AC3X; Bobcat3; Aldrin2; Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_MAC_ENTRY_EXT_TYPE_IPV4_UC_E,

    /** @brief The entry is IPv6 Unicast address entry
     *  (APPLICABLE DEVICES: Bobcat2; Caelum; Aldrin; AC3X; Bobcat3; Aldrin2; Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_MAC_ENTRY_EXT_TYPE_IPV6_UC_ADDR_ENTRY_E,

    /** @brief The entry is IPv6 Unicast data entry
     *  (APPLICABLE DEVICES: Bobcat2; Caelum; Aldrin; AC3X; Bobcat3; Aldrin2; Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_MAC_ENTRY_EXT_TYPE_IPV6_UC_DATA_ENTRY_E,

    /** @brief The entry is MAC Address entry
     *  (hashed by Mac Address, FID and VID1).
     *  (APPLICABLE DEVICES: Bobcat2; Caelum; Aldrin; AC3X; Bobcat3; Aldrin2; Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_MAC_ENTRY_EXT_TYPE_MAC_ADDR_FID_VID1_E,

    /** @brief The entry is IPv4 Multicast
     *  entry. Hashed by SIP, DIP, FID, and VID1.
     *  (APPLICABLE DEVICES: Bobcat2; Caelum; Aldrin; AC3X; Bobcat3; Aldrin2)
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_MAC_ENTRY_EXT_TYPE_IPV4_MCAST_FID_VID1_E,

    /** @brief The entry is IPv6 Multicast
     *  entry. Hashed by SIP, DIP, FID, and VID1.
     *  (APPLICABLE DEVICES: Bobcat2; Caelum; Aldrin; AC3X; Bobcat3; Aldrin2)
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_MAC_ENTRY_EXT_TYPE_IPV6_MCAST_FID_VID1_E

} CPSS_MAC_ENTRY_EXT_TYPE_ENT;

/**
* @struct CPSS_MAC_ENTRY_EXT_KEY_MAC_VLAN_STC
 *
*  @brief Mac entry key parameters of MAC VLAN structure
*/
typedef struct
{

   GT_ETHERADDR     macAddr;
   GT_U16           vlanId;

}CPSS_MAC_ENTRY_EXT_KEY_MAC_VLAN_STC;


/**
* @struct CPSS_MAC_ENTRY_EXT_KEY_IP_MCAST_STC
 *
*  @brief Mac entry key parameters of IP MultiCast structure
*/
typedef struct
{

   GT_U8            sip[4];
   GT_U8            dip[4];
   GT_U16           vlanId;

}CPSS_MAC_ENTRY_EXT_KEY_IP_MCAST_STC;

/**
* @struct CPSS_MAC_ENTRY_EXT_KEY_IPV4_UNICAST_STC
 *
*  @brief Mac entry key parameters of IPv4 Unicast structure
*/
typedef struct
{

   GT_U8            dip[4];
   GT_U32           vrfId;

}CPSS_MAC_ENTRY_EXT_KEY_IPV4_UNICAST_STC;

/**
* @struct CPSS_MAC_ENTRY_EXT_KEY_IPV6_UNICAST_STC
 *
*  @brief Mac entry key parameters of IPv6 Unicast structure
*/
typedef struct
{

   GT_U8            dip[16];
   GT_U32           vrfId;

}CPSS_MAC_ENTRY_EXT_KEY_IPV6_UNICAST_STC;


/**
* @union CPSS_MAC_ENTRY_EXT_KEY_UNT
 *
 * @brief Union Key parameters of the MAC Address Entry
 *
*/
typedef union{

    CPSS_MAC_ENTRY_EXT_KEY_MAC_VLAN_STC macVlan;

    CPSS_MAC_ENTRY_EXT_KEY_IP_MCAST_STC ipMcast;

    CPSS_MAC_ENTRY_EXT_KEY_IPV4_UNICAST_STC ipv4Unicast;

    CPSS_MAC_ENTRY_EXT_KEY_IPV6_UNICAST_STC ipv6Unicast;

} CPSS_MAC_ENTRY_EXT_KEY_UNT;


/**
* @struct CPSS_MAC_ENTRY_EXT_KEY_STC
 *
 * @brief Key parameters of the MAC Address Entry
*/
typedef struct
{
    CPSS_MAC_ENTRY_EXT_TYPE_ENT         entryType;
    GT_U32                              vid1;
    CPSS_MAC_ENTRY_EXT_KEY_UNT          key;

}CPSS_MAC_ENTRY_EXT_KEY_STC;

/**
* @enum CPSS_FDB_UC_ROUTING_TYPE_ENT
*
* @brief This enum defines the location on the Next Hop or
*        Multipath (ECMP/QOS) information
*   (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
*/
typedef enum{

    /** @brief The Next Hop information is located in the FDB entry
     */
    CPSS_FDB_UC_ROUTING_TYPE_NH_FDB_E,

    /** @brief The Multipath (ECMP or QOS) information is located in
     * ECMP/QOS Table in the Router
     */
    CPSS_FDB_UC_ROUTING_TYPE_MULTIPATH_ROUTER_E,

    /** @brief Points to the Multicast Next Hop Entry located in the
     *         Router Hext-Hop table.
     *  This is used for the SIP lookup (G,S) in the fdb for IP multicast.
     *  (APPLICABLE DEVICES: AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_FDB_UC_ROUTING_TYPE_MULTICAST_NH_ENTRY_E
} CPSS_FDB_UC_ROUTING_TYPE_ENT;

/**
* @struct CPSS_FDB_UC_ROUTING_INFO_STC
 *
 * @brief Routing Address Entry
*/
typedef struct{

    /** @brief Enable TTL/Hop Limit Decrement */
    GT_BOOL ttlHopLimitDecEnable;

    /** @brief Enable TTL/HopLimit Decrement and option/Extention
     *         check bypass.
     */
    GT_BOOL ttlHopLimDecOptionsExtChkByPass;

    /** @brief mirror to ingress analyzer.
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_BOOL ingressMirror;

    /** @brief One of seven possible analyzers. Relevant
     *  when ingressMirror is GT_TRUE.
     *  If a previous engine in the pipe assigned a different
     *  analyzer index, the higher index wins.
     *  (APPLICABLE RANGES: 0..6)
     *  qosProfileMarkingEnable- Enable Qos profile assignment.
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_U32 ingressMirrorToAnalyzerIndex;

    /** @brief Enable the remarking of the QoS Profile assigned
     *  to the packet.
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_BOOL qosProfileMarkingEnable;

    /** @brief The qos profile index.
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_U32 qosProfileIndex;

    /** @brief whether this packet Qos parameters can be overridden
     *  after this assingment.
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_PACKET_ATTRIBUTE_ASSIGN_PRECEDENCE_ENT qosPrecedence;

    /** @brief whether to change the packets UP and how.
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_PACKET_ATTRIBUTE_MODIFY_TYPE_ENT modifyUp;

    /** @brief whether to change the packets DSCP and how.
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_PACKET_ATTRIBUTE_MODIFY_TYPE_ENT modifyDscp;

    /** @brief he counter set this route entry is linked to */
    CPSS_IP_CNT_SET_ENT countSet;

    /** @brief enable Trap/Mirror ARP Broadcasts with DIP matching
     *  this entry
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_BOOL trapMirrorArpBcEnable;

    /** @brief The security level associated with the DIP.
     *  (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_U32 dipAccessLevel;

    /** @brief Enable performing ICMP Redirect Exception
     *         Mirroring. */
    GT_BOOL ICMPRedirectEnable;

    /** @brief One of the global configurable MTU sizes
     *  (APPLICABLE RANGES: Falcon, AC5P, AC5X, Harrier, Ironman 0..1)
     *  (APPLICABLE RANGES: Bobcat2, Caelum, Bobcat3, Aldrin, AC3X, Aldrin2 0..7)
     */
    GT_U32 mtuProfileIndex;

    /** @brief weather this nexthop is tunnel start enrty, in which
     *  case the outInteface & mac are irrlevant and the tunnel
     *  id is used.
     *  only if isTunnelStart = GT_FALSE then dstInterface in
     *  CPSS_MAC_ENTRY_EXT_STC is relevant
     *  isTunnelStart = GT_TRUE
     */
    GT_BOOL isTunnelStart;

    /** @brief the output vlan id (used also for SIP RPF check, and
     *  ICMP check)
     */
    GT_U16 nextHopVlanId;

    /** @brief The ARP Pointer indicating the routed packet MAC DA,
     *  relevant only if the isTunnelStart = GT_FALSE
     */
    GT_U32 nextHopARPPointer;

    /** @brief the tunnel pointer in case this is a tunnel start */
    GT_U32 nextHopTunnelPointer;

    /** @brief The bank number of the associated IPv6
     *  NOTE: Relevant only for entries of type CPSS_MAC_ENTRY_EXT_TYPE_IPV6_UC_ADDR_ENTRY_E
     * (NOT APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_U32 nextHopDataBankNumber;

    /** @brief Enable IPv6 Scope Checking. */
    GT_BOOL scopeCheckingEnable;

    /** @brief The site id of this route entry. */
    CPSS_IP_SITE_ID_ENT siteId;

    /** @brief The ECMP/QOS or NextHop Routing Type
     *  (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     *  only if routingType==CPSS_FDB_UC_ROUTING_TYPE_NH_FDB_E then
     *  dstInterface in CPSS_MAC_ENTRY_EXT_STC is relevant.
     *  If routingType==CPSS_FDB_UC_ROUTING_TYPE_MULTIPATH_ROUTER_E
     *  then all above fields are not relevant.
     */
    CPSS_FDB_UC_ROUTING_TYPE_ENT routingType;

    /** @brief The multipathPointer is the index of the ECMP or QOS Entry in
     *  the ECMP/QOS Table in case routingType==CPSS_FDB_UC_ROUTING_TYPE_MULTIPATH_ROUTER_E
     *  Otherwise - multipathPointer is ignored
     *  (APPLICABLE DEVICES: Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_U32 multipathPointer;

    /** @brief Points to the Multicast Next Hop Entry located in FDB.
     *  Note: this parameter is relavent in case of CPSS_FDB_UC_ROUTING_TYPE_MULTICAST_NH_ENTRY_E
     *  (APPLICABLE DEVICES: AC5P; AC5X; Harrier; Ironman)
     */
    GT_U32 nextHopMcPointer;

} CPSS_FDB_UC_ROUTING_INFO_STC;

/**
* @struct CPSS_MAC_ENTRY_EXT_STC
 *
 * @brief Extension to MAC Address Entry
*/
typedef struct{

    /** @brief key data, depends on the type of the MAC entry
     *  MAC entry -> mac address + vlan ID
     *  IP Multicast entry -> srcIP + dstIP + vlan ID
     *  IPv4 UC entry -> dstIP + prefixLen + VRF_ID
     *  IPv6 UC Full entry -> dstIP + prefixLen + VRF_ID
     */
    CPSS_MAC_ENTRY_EXT_KEY_STC key;

    /** @brief destination interface : port/trunk/vidx
     *  For IPv4 and IPv6 multicast entries only VLAN or
     *  VIDX is used.
     *  Relevant for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST/IPV4_UC/IPV6_UC
     *  for key of type IPV4_UC/IPV6_UC this field is the next hop interface
     */
    CPSS_INTERFACE_INFO_STC dstInterface;

    /** @brief Age flag that is used for the two
     *  GT_FALSE - The entry will be aged out in the next pass.
     *  GT_TRUE - The entry will be aged-out in two age-passes.
     *  Notes: used only for DxCh devices.
     *  IP UC entries are not aged out, but only marked as such
     *  Relevant for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST/IPV4_UC/IPV6_UC
     */
    GT_BOOL age;

    /** @brief static/dynamic entry
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_BOOL isStatic;

    /** @brief action taken when a packet's DA matches this entry
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    CPSS_MAC_TABLE_CMD_ENT daCommand;

    /** @brief action taken when a packet's SA matches this entry.
     *  For IPv4 and IPv6 multicast entries these field is not used,
     *  it is ignored.
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    CPSS_MAC_TABLE_CMD_ENT saCommand;

    /** @brief GT_TRUE, and packet's DA matches this entry,
     *  send packet to the IPv4 or MPLS
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_BOOL daRoute;

    /** @brief GT_TRUE, and packet's DA matches this entry,
     *  mirror packet to a configurable analyzer port.
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_BOOL mirrorToRxAnalyzerPortEn;

    /** @brief Source ID. For IPv4 and IPv6 multicast entries these field
     *  is not used, it is ignored.
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_U32 sourceID;

    /** @brief user defined field [0..0xF].
     *  for Lion2 devices: if the use of <MyCoreId> Field In Fdb
     *  Entry is enabled then udb0 is used for <myCoreId> and the
     *  number of bits of udb field is decreased by 1
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_U32 userDefined;

    /** @brief Qos attribute set applied to the packet if there is a
     *  destination lookup mode. If both FDB lookups find a matching
     *  entry, whose attribute index is not NULL, a global
     *  FDB QOS Marking Conflict Resolution command selects one
     *  of the Qos attributes source or destination [0..0x7].
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_U32 daQosIndex;

    /** @brief Qos attribute set applied to the packet if there is a
     *  source lookup mode. If both FDB lookups find a matching
     *  entry, whose attribute index is not NULL, a global
     *  FDB QOS Marking Conflict Resolution command selects one
     *  of the Qos attributes source or destination [0..0x7].
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     *  Cheetah2 fields:
     */
    GT_U32 saQosIndex;

    /** @brief security level assigned to the MAC DA that matches
     *  this entry [0..0x7].
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_U32 daSecurityLevel;

    /** @brief security level assigned to the MAC SA that matches
     *  this entry [0..0x7].
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_U32 saSecurityLevel;

    /** @brief If set, and the entry <DA command> is TRAP or MIRROR
     *  then the CPU code may be overwritten according
     *  to the Application Specific CPU Code assignment
     *  mechanism.
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_BOOL appSpecificCpuCode;

    /** @brief Relevant when auto
     *  New Address (NA) storm prevention is enabled
     *  GT_FALSE - Regular Entry;
     *  GT_TRUE - Storm Prevention Entry;
     *  This is a storm prevention entry indicating
     *  that a NA message has been sent to the CPU
     *  but the CPU has not yet learned this MAC Address
     *  on its current location. The device does not send
     *  further NA messages to the CPU for this source
     *  MAC Address. Should a MAC DA lookup match this entry,
     *  it is treated as an unknown Unicast packet.
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     */
    GT_BOOL spUnknown;

    /** @brief GT_TRUE, and packet's SA matches this entry,
     *  mirror packet to a configurable analyzer port.
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     *  (APPLICABLE DEVICES: Bobcat2; Caelum; Aldrin; AC3X; Bobcat3; Aldrin2; Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_BOOL saMirrorToRxAnalyzerPortEn;

    /** @brief GT_TRUE, and packet's DA matches this entry,
     *  mirror packet to a configurable analyzer port.
     *  Relevant only for key of type MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     *  (APPLICABLE DEVICES: Bobcat2; Caelum; Aldrin; AC3X; Bobcat3; Aldrin2; Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    GT_BOOL daMirrorToRxAnalyzerPortEn;

    /** @brief A muxed field in case of MAC_ADDR/IPV4_MCAST/IPV6_MCAST
     *         Its type depends on the bridge config cpssDxChBrgFdbEpgConfigSet and packet's DA matches this entry,
     *         - Represents EPG only, in case of IPv4_UC/IPv6_UC
     *  (APPLICABLE RANGES: 0..0xFFF)
     *  (APPLICABLE DEVICES: Ironman)
     */
    GT_U32  epgNumber;

    /** @brief All information needed for FDB Routing
     *  Relevant only for key of type IPV4_UC/IPV6_UC
     *  (APPLICABLE DEVICES: Bobcat2; Caelum; Aldrin; AC3X; Bobcat3; Aldrin2; Falcon; AC5P; AC5X; Harrier; Ironman)
     */
    CPSS_FDB_UC_ROUTING_INFO_STC fdbRoutingInfo;

} CPSS_MAC_ENTRY_EXT_STC;

#endif
