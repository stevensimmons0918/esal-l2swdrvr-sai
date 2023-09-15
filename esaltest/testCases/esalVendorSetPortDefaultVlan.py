#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorSetPortDefaultVlan',
    'description' : 'TC to check esal attributes VendorSetPortDefaultVlan, VendorGetPortDefaultVlan, VendorDeletePortDefaultVlan',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorGetPortDefaultVlan 0 0
VendorCreateVlan 75
VendorSetPortDefaultVlan 0 75
VendorGetPortDefaultVlan 0 0
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
# 

tcFlushStr = '''
VendorDeletePortDefaultVlan 0 75
VendorDeleteVlan 75
'''

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorGetPortDefaultVlan(uint16_t,uint16_t*)0rc:0port:0vlanId:75',
}
