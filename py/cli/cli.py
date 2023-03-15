#!/usr/bin/env python2.7

from curses import erasechar
from py.tests import test
from py.esal_helpers import esal_wrapper_p2 as esal

from inspect import getmembers, isfunction, getargspec
import readline

class bcolors:
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    WHITE = '\033[0m'

class TabCompliter():
    def __init__(self, funcs_names, funcs_args):
        self.funcs_names = funcs_names
        self.funcs_args = funcs_args
        self.candidates = []

    def completer(self, text, state):
        response = None
        if state == 0:
            buffer = readline.get_line_buffer()
            begin = readline.get_begidx()
            end = readline.get_endidx()
            current_word = buffer[begin:end]
            words = buffer.split()

            if not words:
                self.candidates = self.funcs_names
            else:
                if begin == 0:
                    self.candidates = self.funcs_names
                    self.candidates = filter(lambda x: x.startswith(current_word), self.candidates)
                else:
                    func_name = words[0]
                    func_args = next((fname_args[1] for fname_args in zip(self.funcs_names, self.funcs_args)
                                if fname_args[0] == func_name), [])
                    self.candidates = [' '.join(func_args)]
                    self.candidates.append(' ')
        try:
            response = self.candidates[state]
        except IndexError:
            response = None
        return response

def command_handler(fns_name_link, cmd):
    try:
        name, args = cmd.split(' ', 1)
    except:
        name = cmd
        args = ''

    fn = next((nm_ln[1] for nm_ln in fns_name_link if nm_ln[0] == name), None)

    if not fn:
        return 'Function "{}" is unavailable'.format(name)

    if args:
        args = ', '.join(args.split(' '))
        e_args = eval('({},)'.format(args))
        return fn(*e_args)
    else:
        return fn()

def print_ret (ret):
    print('{}rc:{} {}'.format(bcolors.CYAN, bcolors.WHITE, ret['rc']))
    for key in sorted(ret):
        if key != 'rc':
            print('{}: {}'.format(key, ret[key]))

def main():
    functions_names = map(lambda x: x[0], getmembers(esal, isfunction))
    functions_links = map(lambda x: x[1], getmembers(esal, isfunction))
    functions_args = map(lambda x: getargspec(x).args, functions_links)
    fns_name_link = zip(functions_names, functions_links)

    readline.set_completer(TabCompliter(functions_names, functions_args).completer)
    readline.parse_and_bind('tab: complete')

    try:
        while True:
            cmd = raw_input('{}CLI: {}'.format(bcolors.RED, bcolors.WHITE))
            if cmd == 'exit': break
            elif cmd == '?':
                print 'Use TAB for interactive help OR type exit to leave\n' \
                      '<func_name> <arg1> <arg2> ... <argn>'
            else:
                try:
                    ret = command_handler(fns_name_link, cmd)
                    print_ret(ret)
                except TypeError:
                    print 'Error! Check arguments!'
        print 'Stop CLI'
    except KeyboardInterrupt:
        print '\nStop CLI'


# test.run_tests()
main()
