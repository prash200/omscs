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
    pinlist = [True] * get_pcpu(conn)
    return tuple(pinlist)

if __name__ == '__main__':

    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    iplist = []

    pinlist = get_default_pin_tuple(conn)
    for vmname in vmlist:
        vm = conn.lookupByName(vmname)
        if vm:
            print('Pin {} to {}.'.format(vmname, pinlist))
            vm.pinVcpu(0, pinlist)
        else:
            print('Unable to locate {}.'.format(vmname))    

    for vmname in vmlist:
        iplist.append(os.popen('uvt-kvm ip {}'.format(vmname)).read().strip())

    FNULL = open(os.devnull, 'w') 
    
    for i in range(len(vmlist)):
        print('{} start running.'.format(vmlist[i]))
        subprocess.Popen("ssh ubuntu@{} '~/cpu/testcases/4/iambusy 100000'".format(iplist[i]), stdout=FNULL, shell=True)

