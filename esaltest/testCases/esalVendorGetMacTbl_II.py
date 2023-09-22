import scapy.all as scapy_t
from time import sleep

#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorGetMacTbl_II',
    'description' : 'TC to check esal attribute VendorGetMacTbl (part-2)',
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
VendorDeletePortsFromVlan 75 2 [28 30]
VendorDeleteVlan 75
'''

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorGetMacTbl(uint16_t,uint16_t*,unsignedchar*)lPort=1crc:0macs:[0,0,17,0,17,0]numMacs:6port:28'
}
