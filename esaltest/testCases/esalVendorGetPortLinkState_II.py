#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorGetPortLinkState_II',
    'description' : 'TC to check esal attribute VendorGetPortLinkState (part-2)',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorGetPortLinkState 28 0
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#

tcFlushStr = '''
'''

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorGetPortLinkState(uint16_t,bool*)lPort=28rc:0ls:Trueport:28'
}
