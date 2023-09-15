#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorSetPortRate',
    'description' : 'TC to check esal attributes VendorGetPortRate, VendorSetPortRate',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorGetPortRate 0 0
VendorSetPortRate 0 False 4 0
VendorGetPortRate 0 0
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#

tcFlushStr = '''
VendorSetPortRate 0 False 5 0
'''

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorSetPortRate(uint16_t,bool,vendor_speed_t,vendor_duplex_t)0rc:0autoneg:Falseduplex:0port:0speed:4'
}
