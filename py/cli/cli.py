#!/usr/bin/env python2.7

from py.tests import test
from py.esal_helpers import esal_wrapper_p2 as esal

from inspect import getmembers, isfunction, getargspec

test.run_tests()

def cmd_handler (cmd):
    if cmd == '?':
        print '\n'.join(function_defs)
    if cmd == 'exit':
        exit(0)

try:
    function_names = map(lambda x: x[0], getmembers(esal, isfunction))
    function_links = map(lambda x: x[1], getmembers(esal, isfunction))
    function_args = map(lambda x: getargspec(x).args, function_links)

    function_defs = map(lambda x: '{}({})'.format(x[0], ' ,'.join(x[1])), zip(function_names, function_args))

    while True:
        cmd = raw_input('CLI: ')
        cmd_handler(cmd)
except KeyboardInterrupt:
    print '\nStop CLI'