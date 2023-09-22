import scapy.all as scapy_t
from time import sleep

#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : 'L2Packet',
    'tcName' : 'esalVendorSetFrameMax_I',
    'description' : 'TC to check esal attributes VendorSetFrameMax, VendorGetFrameMax (part-1)',
    'ingressPort' : ['28'],
    'egressPort' : ['30'],
    'pktAction' : 'DROP',
    'ingressTapIntf' : 'tap0',
    'egressTapIntf'  : ['tap1'],
    'count' : 1,             # expected data count
}

tcProgramStr = '''
VendorCreateVlan 75
VendorAddPortsToVlan 75 2 [28 30]
VendorGetFrameMax 28 0
VendorSetFrameMax 28 70
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#            specific networking scenario.


tcFlushStr = '''
'''

packet_info = scapy_t.Ether(src="00:00:11:00:11:00",dst="00:00:11:00:11:23")/scapy_t.Dot1Q(vlan =75)/scapy_t.IP()/scapy_t.Raw('12345678910111213141516171819202122232425')

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
}
