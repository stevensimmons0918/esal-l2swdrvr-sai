import scapy.all as scapy_t
from time import sleep

#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : 'L2Packet',
    'tcName' : 'esalVendorDropTaggedPacketsOnIngress',
    'description' : 'TC to check esal attribute VendorDropTaggedPacketsOnIngress',
    'ingressPort' : ['29'],
    'egressPort' : ['30'],
    'pktAction' : 'FORWARD',
    'ingressTapIntf' : 'tap1',
    'egressTapIntf'  : ['tap2'],
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorCreateVlan 75
VendorAddPortsToVlan 75 2 [29,30]
VendorTagPacketsOnIngress 29
VendorDropTaggedPacketsOnIngress 29 
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#            specific networking scenario.


tcFlushStr = '''
'''

packet_info = scapy_t.Ether(src="00:00:11:00:11:00",dst="00:00:11:00:11:23")/scapy_t.Dot1Q(vlan =75)/scapy_t.IP()

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
}
