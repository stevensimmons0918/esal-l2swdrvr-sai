import scapy.all as scapy_t
from time import sleep

#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorPurgeMacEntriesPerPort_II',
    'description' : 'TC to check esal attribute VendorPurgeMacEntriesPerPort (part-2)',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorGetMacTbl 28 0 '/'*6
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#            specific networking scenario.


tcFlushStr = '''
VendorPurgeMacEntriesPerPort 28
'''

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorGetMacTbl(uint16_t,uint16_t*,unsignedchar*)28rc:0macs:\x00\x00\x11\x00\x11\x00numMacs:1port:28'
}
