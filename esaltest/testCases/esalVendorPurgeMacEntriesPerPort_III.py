import scapy.all as scapy_t
from time import sleep

#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorPurgeMacEntriesPerPort_III',
    'description' : 'TC to check esal attribute VendorPurgeMacEntriesPerPort (part-3)',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorPurgeMacEntriesPerPort 28
VendorGetMacTbl 28 0 '/'*6
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#            specific networking scenario.


tcFlushStr = '''
VendorDeletePortsFromVlan 75 2 [28,29]
VendorDeleteVlan 75
'''

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorGetPortAutoNeg(uint16_t,bool*)0rc:0aneg:Trueport:0'
}
