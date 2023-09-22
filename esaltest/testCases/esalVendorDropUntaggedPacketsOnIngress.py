import scapy.all as scapy_t
from time import sleep

#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : 'L2Packet',
    'tcName' : 'esalVendorDropUntaggedPacketsOnIngress',
    'description' : 'TC to check esal attribute VendorDropUntaggedPacketsOnIngress',
    'ingressPort' : ['30'],
    'egressPort' : ['28'],
    'pktAction' : 'DROP',
    'ingressTapIntf' : 'tap1',
    'egressTapIntf'  : ['tap0'],
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorCreateVlan 75
VendorTagPacketsOnIngress 30
VendorAddPortsToVlan 75 2 [28 30]
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#            specific networking scenario.


tcFlushStr = '''
VendorResetPort 30
VendorDeletePortsFromVlan 75 2 [28 30]
VendorDeleteVlan 75
'''

packet_info = scapy_t.Ether(src="00:00:11:00:11:00",dst="00:00:11:00:11:23")/scapy_t.Dot1Q(vlan =75)/scapy_t.IP()

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
       'expect1':'''
'''
}
