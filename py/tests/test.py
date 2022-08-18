#!/usr/bin/env python2.7

from py.esal_helpers import esal_wrapper_p2 as esal

# ret is (return_code, arg_1, arg2, ..., argn)
# so you can check ur args if they were changed

def run_tests():

    print '================== Start Testing =================='

    vlan = 100
    ret = esal.VendorCreateVlan(vlan)
    if ret['rc'] != 0:
        print 'error'

    ports = [24, 25, 26, 5]
    ret = esal.VendorAddPortsToVlan(vlan, 4, ports)
    if ret['rc'] != 0:
        print 'error'

    ret = esal.VendorEnablePort(24)
    if ret['rc'] != 0:
        print 'error'

    ret = esal.VendorEnablePort(25)
    if ret['rc'] != 0:
        print 'error'

    num_ports = 0
    ports1 = [0]*512
    ret = esal.VendorGetPortsInVlan(vlan, num_ports, ports1)
    if ret['rc'] != 0:
        print 'error'
    print 'num ports = {}'.format(ret['numPorts'])
    print 'ports = {}'.format([port for port in ret['ports'] if port])

    print '================== Stop Testing ==================='
