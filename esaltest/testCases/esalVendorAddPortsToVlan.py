#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorAddPortsToVlan',
    'description' : 'TC to check esal attributes VendorCreateVlan, VendorAddPortsToVlan, VendorGetPortsInVlan, VendorDeletePortsFromVlan, VendorDeleteVlan',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorCreateVlan 75
VendorAddPortsToVlan 75 1 [28]
VendorAddPortsToVlan 75 3 [30 39 44]
VendorGetPortsInVlan 75 0 [0]*5
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
# 

tcFlushStr = '''
VendorDeletePortsFromVlan 75 4 [28 30 39 44]
VendorDeleteVlan 75
'''

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorGetPortsInVlan(uint16_t,uint16_t*,uint16_t*)75rc:0numPorts:4ports:[28,30,39,44,0]vlanId:75'
}
