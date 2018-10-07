#!/usr/bin/python

from __future__ import print_function
import libvirt

CONFIG_FILE = 'vmlist.conf'
#CURRENT_MEMORY = 256
MAX_MEMORY = 2 * 1024

if __name__ == '__main__':
    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    for vmname in vmlist:
        vm = conn.lookupByName(vmname)
        if vm:
            vm.setMaxMemory(MAX_MEMORY * 1024)
#            vm.setMemory(CURRENT_MEMORY * 1024)
            print('Set {} maxmemory -> {}.'.format(vmname, MAX_MEMORY))
        else:
            print('Unable to locate {}.'.format(vmname))

