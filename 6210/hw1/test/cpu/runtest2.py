#!/usr/bin/python

from __future__ import print_function
import libvirt
import os
import sys
import subprocess

CONFIG_FILE = '../vmlist.conf'

def get_pcpu(conn):
    hostinfo = conn.getInfo()
    return hostinfo[4] * hostinfo[5] * hostinfo[6] * hostinfo[7]

def get_default_pin_tuple(conn):
    pinlist = [False] * get_pcpu(conn)
    pinlist[0] = True
    return tuple(pinlist)

def get_default_pin_tuple2(conn):
    pinlist = [False] * get_pcpu(conn)
    pinlist[-1] = True
    return tuple(pinlist)

if __name__ == '__main__':

    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    iplist = []

    pinlist1 = get_default_pin_tuple(conn)
    pinlist2 = get_default_pin_tuple2(conn)

    flip = True
    for vmname in vmlist:
        vm = conn.lookupByName(vmname)
        if vm:
            if flip:
                print('Pin {} to {}.'.format(vmname, pinlist1))
                vm.pinVcpu(0, pinlist1)
                flip = False
            else:
                print('Pin {} to {}.'.format(vmname, pinlist2))
                vm.pinVcpu(0, pinlist2)
                flip = True
        else:
            print('Unable to locate {}.'.format(vmname))    

    for vmname in vmlist:
        iplist.append(os.popen('uvt-kvm ip {}'.format(vmname)).read().strip())

    FNULL = open(os.devnull, 'w') 
    
    for i in range(len(vmlist)):
        print('{} start running.'.format(vmlist[i]))
        subprocess.Popen("ssh ubuntu@{} '~/cpu/testcases/2/iambusy 100000'".format(iplist[i]), stdout=FNULL, shell=True)

