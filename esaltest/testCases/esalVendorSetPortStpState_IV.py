import scapy.all as scapy_t
from time import sleep

#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : 'L2Packet',
    'tcName' : 'esalVendorSetPortStpState',
    'description' : 'TC to check esal attribute VendorSetPortStpState (part-4)',
    'ingressPort' : ['28'],
    'egressPort' : ['29'],
    'pktAction' : 'FORWARD',
    'ingressTapIntf' : 'tap0',
    'egressTapIntf'  : ['tap1'],
    'count' : 1,             # expected data count
}

tcProgramStr = '''
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#            specific networking scenario.


tcFlushStr = '''
VendorSetPortStpState 28 5
VendorDeletePortsFromVlan 75 2 [28,29]
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

