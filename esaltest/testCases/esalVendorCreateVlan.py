#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorCreateVlan',
    'description' : 'TC to check esal attributes VendorCreateVlan, VendorDeleteVlan, VendorGetPortsInVlan',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorCreateVlan 75
VendorGetPortsInVlan 75 0 [0]*5
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
# 

tcFlushStr = '''
VendorDeleteVlan 75
'''

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorGetPortsInVlan(uint16_t,uint16_t*,uint16_t*)75rc:0numPorts:0ports:[0,0,0,0,0]vlanId:75'
}
