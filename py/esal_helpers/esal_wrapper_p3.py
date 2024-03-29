#!/usr/bin/env python3.10

from typing import List, Tuple

import ctypes

################################################################################
#                            Not wrapped yet(18)
################################################################################

# esal_vendor_api_version_t VendorApiGetVersion (void);
# int VendorSetPortRate (uint16_t port, bool autoneg, vendor_speed_t speed, vendor_duplex_t duplex);
# int VendorGetPortRate (uint16_t port, vendor_speed_t *speed);
# int VendorGetPortDuplex (uint16_t port, vendor_duplex_t *duplex);
# int VendorSetPortStpState (uint16_t port, vendor_stp_state_t stpState);
# int VendorGetPortStpState (uint16_t port, vendor_stp_state_t *stpState);
# int VendorSetPortDoubleTagMode (uint16_t port, vendor_dtag_mode mode);
# int VendorGetPortDoubleTagMode (uint16_t port, vendor_dtag_mode *mode);
# int VendorSetIngressVlanTranslation (uint16_t port, vendor_vlan_translation_t trans);
# int VendorGetIngressVlanTranslation (uint16_t port, int *size, vendor_vlan_translation_t trans[]);
# int VendorDeleteIngressVlanTranslation (uint16_t port, vendor_vlan_translation_t trans);
# int VendorSetEgressVlanTranslation (uint16_t port, vendor_vlan_translation_t trans);
# int VendorGetEgressVlanTranslation (uint16_t port, int *size, vendor_vlan_translation_t trans[]);
# int VendorDeleteEgressVlanTranslation (uint16_t port, vendor_vlan_translation_t trans);
# int VendorRegisterRxCb (VendorRxCallback_fp_t cb, void *cbId);
# int VendorSetPortNniMode (uint16_t port, vendor_nni_mode_t mode);
# int VendorGetPortNniMode (uint16_t port, vendor_nni_mode_t *mode);
# int VendorRegisterL2ParamChangeCb (VendorL2ParamChangeCb_fp_t cb, void *cbId);

esai_vendor_api = ctypes.CDLL('libesal.so')

# int DllInit (void);
esai_vendor_api.DllInit.restype = ctypes.c_int
esai_vendor_api.DllInit.argtypes = None
def DllInit () -> Tuple[int]:
    ret = esai_vendor_api.DllInit()
    return {'rc': ret}

# int DllDestroy (void);
esai_vendor_api.DllDestroy.restype = ctypes.c_int
esai_vendor_api.DllDestroy.argtypes = None
def DllDestroy () -> Tuple[int]:
    ret = esai_vendor_api.DllDestroy()
    return {'rc': ret}

# void DllGetName (char *dllname);
esai_vendor_api.DllGetName.restype = None
esai_vendor_api.DllGetName.argtypes = [ctypes.POINTER(ctypes.c_char)]
def DllGetName (dllname: str) -> Tuple[None, str]:
    w_dllname = dllname.encode("utf-8")
    ret = esai_vendor_api.DllGetName(w_dllname)
    r_dllname = w_dllname.decode("utf-8")
    return {'rc': ret, 'dllname': r_dllname}

# int VendorRcToString (int rc, char *strErr);
esai_vendor_api.VendorRcToString.restype = ctypes.c_int
esai_vendor_api.VendorRcToString.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_char)]
def VendorRcToString (rc: int, strErr: str) -> Tuple[int, int, str]:
    w_rc = ctypes.c_int(rc)
    w_strErr = strErr.encode("utf-8")
    ret = esai_vendor_api.VendorRcToString(w_rc, w_strErr)
    r_rc = w_rc.value
    r_strErr = w_strErr.decode("utf-8")
    return {'rc': ret, 'rc': r_rc, 'strErr': r_strErr}

# int VendorBoardInit (void);
esai_vendor_api.VendorBoardInit.restype = ctypes.c_int
esai_vendor_api.VendorBoardInit.argtypes = None
def VendorBoardInit () -> Tuple[int]:
    ret = esai_vendor_api.VendorBoardInit()
    return {'rc': ret}

# uint16_t VendorGetMaxPorts (void);
esai_vendor_api.VendorGetMaxPorts.restype = ctypes.c_uint16
esai_vendor_api.VendorGetMaxPorts.argtypes = None
def VendorGetMaxPorts () -> Tuple[int]:
    ret = esai_vendor_api.VendorGetMaxPorts()
    return {'rc': ret}

# int VendorCreateVlan (uint16_t vlanid);
esai_vendor_api.VendorCreateVlan.restype = ctypes.c_int
esai_vendor_api.VendorCreateVlan.argtypes = [ctypes.c_uint16]
def VendorCreateVlan (vlanid: int) -> Tuple[int, int]:
    w_vlanid = ctypes.c_uint16(vlanid)
    ret = esai_vendor_api.VendorCreateVlan(w_vlanid)
    r_vlanid = w_vlanid.value
    return {'rc': ret, 'vlanid': r_vlanid}

# int VendorDeleteVlan (uint16_t vlanid);
esai_vendor_api.VendorDeleteVlan.restype = ctypes.c_int
esai_vendor_api.VendorDeleteVlan.argtypes = [ctypes.c_uint16]
def VendorDeleteVlan (vlanid: int) -> Tuple[int, int]:
    w_vlanid = ctypes.c_uint16(vlanid)
    ret = esai_vendor_api.VendorDeleteVlan(w_vlanid)
    r_vlanid = w_vlanid.value
    return {'rc': ret, 'vlanid': r_vlanid}

# int VendorAddPortsToVlan (uint16_t vlanid, uint16_t numPorts, const uint16_t ports[]);
esai_vendor_api.VendorAddPortsToVlan.restype = ctypes.c_int
esai_vendor_api.VendorAddPortsToVlan.argtypes = [ctypes.c_uint16, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16)]
def VendorAddPortsToVlan (vlanid: int, numPorts: int, ports: List[int]) -> Tuple[int, int, int, List[int]]:
    w_vlanid = ctypes.c_uint16(vlanid)
    w_numPorts = ctypes.c_uint16(numPorts)
    w_ports = (ctypes.c_uint16*len(ports))(*ports)
    ret = esai_vendor_api.VendorAddPortsToVlan(w_vlanid, w_numPorts, w_ports)
    r_vlanid = w_vlanid.value
    r_numPorts = w_numPorts.value
    r_ports = list(w_ports)
    return {'rc': ret, 'vlanid': r_vlanid, 'numPorts': r_numPorts, 'ports': r_ports}

# int VendorDeletePortsFromVlan (uint16_t vlanid, uint16_t numPorts, const uint16_t ports[]);
esai_vendor_api.VendorDeletePortsFromVlan.restype = ctypes.c_int
esai_vendor_api.VendorDeletePortsFromVlan.argtypes = [ctypes.c_uint16, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16)]
def VendorDeletePortsFromVlan (vlanid: int, numPorts: int, ports: List[int]) -> Tuple[int, int, int, List[int]]:
    w_vlanid = ctypes.c_uint16(vlanid)
    w_numPorts = ctypes.c_uint16(numPorts)
    w_ports = (ctypes.c_uint16*len(ports))(*ports)
    ret = esai_vendor_api.VendorDeletePortsFromVlan(w_vlanid, w_numPorts, w_ports)
    r_vlanid = w_vlanid.value
    r_numPorts = w_numPorts.value
    r_ports = list(w_ports)
    return {'rc': ret, 'vlanid': r_vlanid, 'numPorts': r_numPorts, 'ports': r_ports}

# int VendorGetPortsInVlan (uint16_t vlanId, uint16_t *numPorts, uint16_t ports[]);
esai_vendor_api.VendorGetPortsInVlan.restype = ctypes.c_int
esai_vendor_api.VendorGetPortsInVlan.argtypes = [ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16), ctypes.POINTER(ctypes.c_uint16)]
def VendorGetPortsInVlan (vlanId: int, numPorts: int, ports: List[int]) -> Tuple[int, int, int, List[int]]:
    w_vlanId = ctypes.c_uint16(vlanId)
    w_numPorts = ctypes.c_uint16(numPorts)
    w_ports = (ctypes.c_uint16*len(ports))(*ports)
    ret = esai_vendor_api.VendorGetPortsInVlan(w_vlanId, w_numPorts, w_ports)
    r_vlanId = w_vlanId.value
    r_numPorts = w_numPorts.value
    r_ports = list(w_ports)
    return {'rc': ret, 'vlanId': r_vlanId, 'numPorts': r_numPorts, 'ports': r_ports}

# int VendorSetPortDefaultVlan (uint16_t port, uint16_t vlanId);
esai_vendor_api.VendorSetPortDefaultVlan.restype = ctypes.c_int
esai_vendor_api.VendorSetPortDefaultVlan.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
def VendorSetPortDefaultVlan (port: int, vlanId: int) -> Tuple[int, int, int]:
    w_port = ctypes.c_uint16(port)
    w_vlanId = ctypes.c_uint16(vlanId)
    ret = esai_vendor_api.VendorSetPortDefaultVlan(w_port, w_vlanId)
    r_port = w_port.value
    r_vlanId = w_vlanId.value
    return {'rc': ret, 'port': r_port, 'vlanId': r_vlanId}

# int VendorGetPortDefaultVlan (uint16_t port, uint16_t *vlanId);
esai_vendor_api.VendorGetPortDefaultVlan.restype = ctypes.c_int
esai_vendor_api.VendorGetPortDefaultVlan.argtypes = [ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16)]
def VendorGetPortDefaultVlan (port: int, vlanId: int) -> Tuple[int, int, int]:
    w_port = ctypes.c_uint16(port)
    w_vlanId = ctypes.c_uint16(vlanId)
    ret = esai_vendor_api.VendorGetPortDefaultVlan(w_port, w_vlanId)
    r_port = w_port.value
    r_vlanId = w_vlanId.value
    return {'rc': ret, 'port': r_port, 'vlanId': r_vlanId}

# int VendorDeletePortDefaultVlan (uint16_t port, uint16_t vlanId);
esai_vendor_api.VendorDeletePortDefaultVlan.restype = ctypes.c_int
esai_vendor_api.VendorDeletePortDefaultVlan.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
def VendorDeletePortDefaultVlan (port: int, vlanId: int) -> Tuple[int, int, int]:
    w_port = ctypes.c_uint16(port)
    w_vlanId = ctypes.c_uint16(vlanId)
    ret = esai_vendor_api.VendorDeletePortDefaultVlan(w_port, w_vlanId)
    r_port = w_port.value
    r_vlanId = w_vlanId.value
    return {'rc': ret, 'port': r_port, 'vlanId': r_vlanId}

# int VendorSetPortEgress (uint16_t port, uint16_t numPorts, const uint16_t ports[]);
esai_vendor_api.VendorSetPortEgress.restype = ctypes.c_int
esai_vendor_api.VendorSetPortEgress.argtypes = [ctypes.c_uint16, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16)]
def VendorSetPortEgress (port: int, numPorts: int, ports: List[int]) -> Tuple[int, int, int, List[int]]:
    w_port = ctypes.c_uint16(port)
    w_numPorts = ctypes.c_uint16(numPorts)
    w_ports = (ctypes.c_uint16*len(ports))(*ports)
    ret = esai_vendor_api.VendorSetPortEgress(w_port, w_numPorts, w_ports)
    r_port = w_port.value
    r_numPorts = w_numPorts.value
    r_ports = list(w_ports)
    return {'rc': ret, 'port': r_port, 'numPorts': r_numPorts, 'ports': r_ports}

# int VendorGetPortAutoNeg (uint16_t port, bool *aneg);
esai_vendor_api.VendorGetPortAutoNeg.restype = ctypes.c_int
esai_vendor_api.VendorGetPortAutoNeg.argtypes = [ctypes.c_uint16, ctypes.POINTER(ctypes.c_bool)]
def VendorGetPortAutoNeg (port: int, aneg: bool) -> Tuple[int, int, bool]:
    w_port = ctypes.c_uint16(port)
    w_aneg = ctypes.c_bool(aneg)
    ret = esai_vendor_api.VendorGetPortAutoNeg(w_port, w_aneg)
    r_port = w_port.value
    r_aneg = w_aneg.value
    return {'rc': ret, 'port': r_port, 'aneg': r_aneg}

# int VendorGetPortLinkState (uint16_t port, bool *ls);
esai_vendor_api.VendorGetPortLinkState.restype = ctypes.c_int
esai_vendor_api.VendorGetPortLinkState.argtypes = [ctypes.c_uint16, ctypes.POINTER(ctypes.c_bool)]
def VendorGetPortLinkState (port: int, ls: bool) -> Tuple[int, int, bool]:
    w_port = ctypes.c_uint16(port)
    w_ls = ctypes.c_bool(ls)
    ret = esai_vendor_api.VendorGetPortLinkState(w_port, w_ls)
    r_port = w_port.value
    r_ls = w_ls.value
    return {'rc': ret, 'port': r_port, 'ls': r_ls}

# int VendorMirrorPort (uint16_t srcPort, uint16_t dstPort);
esai_vendor_api.VendorMirrorPort.restype = ctypes.c_int
esai_vendor_api.VendorMirrorPort.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
def VendorMirrorPort (srcPort: int, dstPort: int) -> Tuple[int, int, int]:
    w_srcPort = ctypes.c_uint16(srcPort)
    w_dstPort = ctypes.c_uint16(dstPort)
    ret = esai_vendor_api.VendorMirrorPort(w_srcPort, w_dstPort)
    r_srcPort = w_srcPort.value
    r_dstPort = w_dstPort.value
    return {'rc': ret, 'srcPort': r_srcPort, 'dstPort': r_dstPort}

# int VendorRemoveMirrorPort (uint16_t srcPort, uint16_t dstPort);
esai_vendor_api.VendorRemoveMirrorPort.restype = ctypes.c_int
esai_vendor_api.VendorRemoveMirrorPort.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
def VendorRemoveMirrorPort (srcPort: int, dstPort: int) -> Tuple[int, int, int]:
    w_srcPort = ctypes.c_uint16(srcPort)
    w_dstPort = ctypes.c_uint16(dstPort)
    ret = esai_vendor_api.VendorRemoveMirrorPort(w_srcPort, w_dstPort)
    r_srcPort = w_srcPort.value
    r_dstPort = w_dstPort.value
    return {'rc': ret, 'srcPort': r_srcPort, 'dstPort': r_dstPort}

# int VendorEnablePort (uint16_t port);
esai_vendor_api.VendorEnablePort.restype = ctypes.c_int
esai_vendor_api.VendorEnablePort.argtypes = [ctypes.c_uint16]
def VendorEnablePort (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorEnablePort(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorDisablePort (uint16_t port);
esai_vendor_api.VendorDisablePort.restype = ctypes.c_int
esai_vendor_api.VendorDisablePort.argtypes = [ctypes.c_uint16]
def VendorDisablePort (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorDisablePort(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorTagPacketsOnIngress (uint16_t port);
esai_vendor_api.VendorTagPacketsOnIngress.restype = ctypes.c_int
esai_vendor_api.VendorTagPacketsOnIngress.argtypes = [ctypes.c_uint16]
def VendorTagPacketsOnIngress (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorTagPacketsOnIngress(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorStripTagsOnEgress (uint16_t port);
esai_vendor_api.VendorStripTagsOnEgress.restype = ctypes.c_int
esai_vendor_api.VendorStripTagsOnEgress.argtypes = [ctypes.c_uint16]
def VendorStripTagsOnEgress (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorStripTagsOnEgress(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorDropTaggedPacketsOnIngress (uint16_t port);
esai_vendor_api.VendorDropTaggedPacketsOnIngress.restype = ctypes.c_int
esai_vendor_api.VendorDropTaggedPacketsOnIngress.argtypes = [ctypes.c_uint16]
def VendorDropTaggedPacketsOnIngress (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorDropTaggedPacketsOnIngress(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorDropUntaggedPacketsOnIngress (uint16_t port);
esai_vendor_api.VendorDropUntaggedPacketsOnIngress.restype = ctypes.c_int
esai_vendor_api.VendorDropUntaggedPacketsOnIngress.argtypes = [ctypes.c_uint16]
def VendorDropUntaggedPacketsOnIngress (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorDropUntaggedPacketsOnIngress(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorAddPacketFilter (const char *filter, uint16_t length);
esai_vendor_api.VendorAddPacketFilter.restype = ctypes.c_int
esai_vendor_api.VendorAddPacketFilter.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.c_uint16]
def VendorAddPacketFilter (filter: str, length: int) -> Tuple[int, str, int]:
    w_filter = filter.encode("utf-8")
    w_length = ctypes.c_uint16(length)
    ret = esai_vendor_api.VendorAddPacketFilter(w_filter, w_length)
    r_filter = w_filter.decode("utf-8")
    r_length = w_length.value
    return {'rc': ret, 'filter': r_filter, 'length': r_length}

# int VendorDeletePacketFilter (const char *filterName);
esai_vendor_api.VendorDeletePacketFilter.restype = ctypes.c_int
esai_vendor_api.VendorDeletePacketFilter.argtypes = [ctypes.POINTER(ctypes.c_char)]
def VendorDeletePacketFilter (filterName: str) -> Tuple[int, str]:
    w_filterName = filterName.encode("utf-8")
    ret = esai_vendor_api.VendorDeletePacketFilter(w_filterName)
    r_filterName = w_filterName.decode("utf-8")
    return {'rc': ret, 'filterName': r_filterName}

# int VendorSendPacket (uint16_t port, uint16_t length, const void *packet);
esai_vendor_api.VendorSendPacket.restype = ctypes.c_int
esai_vendor_api.VendorSendPacket.argtypes = [ctypes.c_uint16, ctypes.c_uint16, ctypes.c_void_p]
def VendorSendPacket (port: int, length: int, packet: str) -> Tuple[int, int, int, str]:
    w_port = ctypes.c_uint16(port)
    w_length = ctypes.c_uint16(length)
    w_packet = packet.encode("utf-8")
    ret = esai_vendor_api.VendorSendPacket(w_port, w_length, w_packet)
    r_port = w_port.value
    r_length = w_length.value
    r_packet = w_packet.decode("utf-8")
    return {'rc': ret, 'port': r_port, 'length': r_length, 'packet': r_packet}

# int VendorPurgeMacEntriesPerPort (uint16_t port);
esai_vendor_api.VendorPurgeMacEntriesPerPort.restype = ctypes.c_int
esai_vendor_api.VendorPurgeMacEntriesPerPort.argtypes = [ctypes.c_uint16]
def VendorPurgeMacEntriesPerPort (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorPurgeMacEntriesPerPort(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorPurgeMacEntries (void);
esai_vendor_api.VendorPurgeMacEntries.restype = ctypes.c_int
esai_vendor_api.VendorPurgeMacEntries.argtypes = None
def VendorPurgeMacEntries () -> Tuple[int]:
    ret = esai_vendor_api.VendorPurgeMacEntries()
    return {'rc': ret}

# int VendorGetMacTbl (uint16_t port, uint16_t *numMacs, unsigned char *macs);
esai_vendor_api.VendorGetMacTbl.restype = ctypes.c_int
esai_vendor_api.VendorGetMacTbl.argtypes = [ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16), ctypes.POINTER(ctypes.c_char)]
def VendorGetMacTbl (port: int, numMacs: int, macs: str) -> Tuple[int, int, int, str]:
    w_port = ctypes.c_uint16(port)
    w_numMacs = ctypes.c_uint16(numMacs)
    w_macs = macs.encode("utf-8")
    ret = esai_vendor_api.VendorGetMacTbl(w_port, w_numMacs, w_macs)
    r_port = w_port.value
    r_numMacs = w_numMacs.value
    r_macs = w_macs.decode("utf-8")
    return {'rc': ret, 'port': r_port, 'numMacs': r_numMacs, 'macs': r_macs}

# int VendorDisableMacLearningPerVlan (uint16_t vlan);
esai_vendor_api.VendorDisableMacLearningPerVlan.restype = ctypes.c_int
esai_vendor_api.VendorDisableMacLearningPerVlan.argtypes = [ctypes.c_uint16]
def VendorDisableMacLearningPerVlan (vlan: int) -> Tuple[int, int]:
    w_vlan = ctypes.c_uint16(vlan)
    ret = esai_vendor_api.VendorDisableMacLearningPerVlan(w_vlan)
    r_vlan = w_vlan.value
    return {'rc': ret, 'vlan': r_vlan}

# int VendorEnableMacLearningPerVlan (uint16_t vlan);
esai_vendor_api.VendorEnableMacLearningPerVlan.restype = ctypes.c_int
esai_vendor_api.VendorEnableMacLearningPerVlan.argtypes = [ctypes.c_uint16]
def VendorEnableMacLearningPerVlan (vlan: int) -> Tuple[int, int]:
    w_vlan = ctypes.c_uint16(vlan)
    ret = esai_vendor_api.VendorEnableMacLearningPerVlan(w_vlan)
    r_vlan = w_vlan.value
    return {'rc': ret, 'vlan': r_vlan}

# int VendorDisableMacLearningPerPort (uint16_t port);
esai_vendor_api.VendorDisableMacLearningPerPort.restype = ctypes.c_int
esai_vendor_api.VendorDisableMacLearningPerPort.argtypes = [ctypes.c_uint16]
def VendorDisableMacLearningPerPort (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorDisableMacLearningPerPort(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorEnableMacLearningPerPort (uint16_t port);
esai_vendor_api.VendorEnableMacLearningPerPort.restype = ctypes.c_int
esai_vendor_api.VendorEnableMacLearningPerPort.argtypes = [ctypes.c_uint16]
def VendorEnableMacLearningPerPort (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorEnableMacLearningPerPort(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorSetFrameMax (uint16_t port, uint16_t size);
esai_vendor_api.VendorSetFrameMax.restype = ctypes.c_int
esai_vendor_api.VendorSetFrameMax.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
def VendorSetFrameMax (port: int, size: int) -> Tuple[int, int, int]:
    w_port = ctypes.c_uint16(port)
    w_size = ctypes.c_uint16(size)
    ret = esai_vendor_api.VendorSetFrameMax(w_port, w_size)
    r_port = w_port.value
    r_size = w_size.value
    return {'rc': ret, 'port': r_port, 'size': r_size}

# int VendorGetFrameMax (uint16_t port, uint16_t *size);
esai_vendor_api.VendorGetFrameMax.restype = ctypes.c_int
esai_vendor_api.VendorGetFrameMax.argtypes = [ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16)]
def VendorGetFrameMax (port: int, size: int) -> Tuple[int, int, int]:
    w_port = ctypes.c_uint16(port)
    w_size = ctypes.c_uint16(size)
    ret = esai_vendor_api.VendorGetFrameMax(w_port, w_size)
    r_port = w_port.value
    r_size = w_size.value
    return {'rc': ret, 'port': r_port, 'size': r_size}

# int VendorSetPortAdvertAbility (uint16_t port, uint16_t cap);
esai_vendor_api.VendorSetPortAdvertAbility.restype = ctypes.c_int
esai_vendor_api.VendorSetPortAdvertAbility.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
def VendorSetPortAdvertAbility (port: int, cap: int) -> Tuple[int, int, int]:
    w_port = ctypes.c_uint16(port)
    w_cap = ctypes.c_uint16(cap)
    ret = esai_vendor_api.VendorSetPortAdvertAbility(w_port, w_cap)
    r_port = w_port.value
    r_cap = w_cap.value
    return {'rc': ret, 'port': r_port, 'cap': r_cap}

# int VendorGetPortAdvertAbility (uint16_t port, uint16_t *advert);
esai_vendor_api.VendorGetPortAdvertAbility.restype = ctypes.c_int
esai_vendor_api.VendorGetPortAdvertAbility.argtypes = [ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16)]
def VendorGetPortAdvertAbility (port: int, advert: int) -> Tuple[int, int, int]:
    w_port = ctypes.c_uint16(port)
    w_advert = ctypes.c_uint16(advert)
    ret = esai_vendor_api.VendorGetPortAdvertAbility(w_port, w_advert)
    r_port = w_port.value
    r_advert = w_advert.value
    return {'rc': ret, 'port': r_port, 'advert': r_advert}

# int VendorWarmRestartRequest (void);
esai_vendor_api.VendorWarmRestartRequest.restype = ctypes.c_int
esai_vendor_api.VendorWarmRestartRequest.argtypes = None
def VendorWarmRestartRequest () -> Tuple[int]:
    ret = esai_vendor_api.VendorWarmRestartRequest()
    return {'rc': ret}

# int VendorResetPort (uint16_t port);
esai_vendor_api.VendorResetPort.restype = ctypes.c_int
esai_vendor_api.VendorResetPort.argtypes = [ctypes.c_uint16]
def VendorResetPort (port: int) -> Tuple[int, int]:
    w_port = ctypes.c_uint16(port)
    ret = esai_vendor_api.VendorResetPort(w_port)
    r_port = w_port.value
    return {'rc': ret, 'port': r_port}

# int VendorGetL2Pm (uint16_t* used_len, uint16_t max_len, char* msg);
esai_vendor_api.VendorGetL2Pm.restype = ctypes.c_int
esai_vendor_api.VendorGetL2Pm.argtypes = [ctypes.POINTER(ctypes.c_uint16), ctypes.c_uint16, ctypes.POINTER(ctypes.c_char)]
def VendorGetL2Pm (used_len: int, max_len: int, msg: str) -> Tuple[int, int, int, str]:
    w_used_len = ctypes.c_uint16(used_len)
    w_max_len = ctypes.c_uint16(max_len)
    w_msg = msg.encode("utf-8")
    ret = esai_vendor_api.VendorGetL2Pm(w_used_len, w_max_len, w_msg)
    r_used_len = w_used_len.value
    r_max_len = w_max_len.value
    r_msg = w_msg.decode("utf-8")
    return {'rc': ret, 'used_len': r_used_len, 'max_len': r_max_len, 'msg': r_msg}

# int VendorReadReg (uint16_t port, uint16_t reg, uint16_t *val);
esai_vendor_api.VendorReadReg.restype = ctypes.c_int
esai_vendor_api.VendorReadReg.argtypes = [ctypes.c_uint16, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16)]
def VendorReadReg (port: int, reg: int, val: int) -> Tuple[int, int, int, int]:
    w_port = ctypes.c_uint16(port)
    w_reg = ctypes.c_uint16(reg)
    w_val = ctypes.c_uint16(val)
    ret = esai_vendor_api.VendorReadReg(w_port, w_reg, w_val)
    r_port = w_port.value
    r_reg = w_reg.value
    r_val = w_val.value
    return {'rc': ret, 'port': r_port, 'reg': r_reg, 'val': r_val}

# int VendorWriteReg (uint16_t port, uint16_t reg, uint16_t val);
esai_vendor_api.VendorWriteReg.restype = ctypes.c_int
esai_vendor_api.VendorWriteReg.argtypes = [ctypes.c_uint16, ctypes.c_uint16, ctypes.c_uint16]
def VendorWriteReg (port: int, reg: int, val: int) -> Tuple[int, int, int, int]:
    w_port = ctypes.c_uint16(port)
    w_reg = ctypes.c_uint16(reg)
    w_val = ctypes.c_uint16(val)
    ret = esai_vendor_api.VendorWriteReg(w_port, w_reg, w_val)
    r_port = w_port.value
    r_reg = w_reg.value
    r_val = w_val.value
    return {'rc': ret, 'port': r_port, 'reg': r_reg, 'val': r_val}

# void VendorDbg (const char *cmd);
esai_vendor_api.VendorDbg.restype = None
esai_vendor_api.VendorDbg.argtypes = [ctypes.POINTER(ctypes.c_char)]
def VendorDbg (cmd: str) -> Tuple[None, str]:
    w_cmd = cmd.encode("utf-8")
    ret = esai_vendor_api.VendorDbg(w_cmd)
    r_cmd = w_cmd.decode("utf-8")
    return {'rc': ret, 'cmd': r_cmd}
