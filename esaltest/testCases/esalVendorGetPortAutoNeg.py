#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorGetPortAutoNeg',
    'description' : 'TC to check esal attribute VendorGetPortAutoNeg',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorGetPortAutoNeg 0 0
VendorSetPortRate 0 False 5 0
VendorGetPortAutoNeg 0 0
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#

tcFlushStr = '''
VendorSetPortRate 0 True 5 0
'''

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorGetPortAutoNeg(uint16_t,bool*)0rc:0aneg:Trueport:0'
}
