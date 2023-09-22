#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorRcToString',
    'description' : 'TC to check esal attribute VendorRcToString',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorRcToString 0 '0'*64
VendorRcToString 1 '1'*64
VendorRcToString -1 '0'*64
VendorRcToString -2 '0'*64
VendorRcToString -3 '0'*64
VendorRcToString 2 '0'*32
'''

tcFlushStr = '''
'''


#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorRcToString(int,char*)2rc:2strErr:UnknownReason'
}
