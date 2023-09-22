#from py.esal_helpers import esal_wrapper_p2 as esal
from time import sleep

#
#tcParams: This dictionary contains parameters to be used, in order to configure specific
#          networking scenario, in future it can be used to auto generate spirent streams.

tcParams = {
    'ingressPacket' : '',
    'tcName' : 'esalVendorSetPortDoubleTagMode',
    'description' : 'TC to check esal attributes VendorSetPortDoubleTagMode, VendorGetPortDoubleTagMode',
    'ingressPort' : [''],
    'egressPort' : [''],
    'pktAction' : '',
    'count' : 1,             # expected data count
}

tcProgramStr = '''
NYI
'''

tcFlushStr = '''
'''


#
#expectedData: This dictionary expected egress stream for each egress port.
#

expectedData = {
        'expect1':'intVendorGetPortAutoNeg(uint16_t,bool*)0rc:0aneg:Trueport:0'
}
