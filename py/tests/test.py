#!/usr/bin/env python2.7

from py.esal_helpers import esal_wrapper_p2 as esal

# rc is (return_code, arg_1, arg2, ..., argn)
# so you can check ur args if they were changed

def run_tests():

    print '================== Start Testing =================='

    vlan = 100
    rc = esal.WrappedVendorCreateVlan(vlan)
    if rc[0] != 0:
        print 'error'

    ports = [24, 25, 26, 5]
    rc = esal.WrappedVendorAddPortsToVlan(vlan, 4, ports)
    if rc[0] != 0:
        print 'error'

    rc = esal.WrappedVendorEnablePort(24)
    if rc[0] != 0:
        print 'error'

    rc = esal.WrappedVendorEnablePort(25)
    if rc[0] != 0:
        print 'error'

    num_ports = 0
    ports1 = [0]*512
    rc = esal.WrappedVendorGetPortsInVlan(vlan, num_ports, ports1)
    if rc[0] != 0:
        print 'error'
    print 'num ports = {}'.format(rc[2])
    print 'ports = {}'.format([port for port in rc[3] if port])

    print '================== Stop Testing ==================='
