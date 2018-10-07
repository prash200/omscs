#!/usr/bin/python

from __future__ import print_function
import libvirt
import time

CONFIG_FILE = 'vmlist.conf'

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
