import scapy.all as scapy_t

#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : 'test',
    'tcName' : 'esalVendorGetPortLinkState_I',
    'description' : 'TC to check esal attribute VendorGetPortLinkState (part-1)',
    'ingressPort' : ['28'],
    'egressPort' : [''],
    'pktAction' : '',
    'ingressTapIntf' : 'tap0',
    'egressTapIntf'  : ['tap1'],
    'count' : 0,             # expected data count
}

tcProgramStr = '''
VendorGetPortLinkState 28 0
'''

#
#tcFlushStr: This string contains chain of xpShell commands to be used, in order to remove
#

tcFlushStr = '''
'''

packet_info = scapy_t.Ether(src="00:00:11:00:11:00",dst="00:00:11:00:11:23")/scapy_t.Dot1Q(vlan =75)/scapy_t.IP()

#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
}
