#!/usr/bin/python

from __future__ import print_function
import libvirt
import time

CONFIG_FILE = 'vmlist.conf.cpu'
if len(sys.argv) > 1:
    print('USAGE: ./startallvm.py [memory]')
    exit(1)
elif sys.argv[1] == 'memory':
    print('using CPU conf file')
    CONFIG_FILE = 'vmlist.conf.memory'
else:
    print('using cpu conf file, to use memory conf file use ./startallvm.py memory')

if __name__ == '__main__':
    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    for vmname in vmlist:
        vm = conn.lookupByName(vmname)
        if vm:
            print('Shutdown {}.'.format(vmname))
            vm.shutdown()
        else:
            print('Unable to locate {}.'.format(vmname))

    time.sleep(5)
