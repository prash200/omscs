#!/usr/bin/python

from __future__ import print_function
import libvirt

CONFIG_FILE = 'vmlist.conf'


def get_pcpu(conn):
    hostinfo = conn.getInfo()
    return hostinfo[4] * hostinfo[5] * hostinfo[6] * hostinfo[7]

def get_default_pin_tuple(conn):
    pinlist = [False] * get_pcpu(conn)
    pinlist[0] = True
    return tuple(pinlist)

if __name__ == '__main__':
    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    pinlist = get_default_pin_tuple(conn)
    for vmname in vmlist:
        vm = conn.lookupByName(vmname)
        if vm:
            print('Pin {} to {}.'.format(vmname, pinlist))
            vm.pinVcpu(0, pinlist)
        else:
            print('Unable to locate {}.'.format(vmname))

