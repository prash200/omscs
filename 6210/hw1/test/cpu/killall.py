#!/usr/bin/python

from __future__ import print_function
import libvirt
import os
import sys
import subprocess

CONFIG_FILE = '../vmlist.conf'

if __name__ == '__main__':

    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    iplist = []

    for vmname in vmlist:
        iplist.append(os.popen('uvt-kvm ip {}'.format(vmname)).read().strip())

    FNULL = open(os.devnull, 'w') 
    
    for i in range(len(vmlist)):
        print('{} stop running.'.format(vmlist[i]))
        subprocess.Popen("ssh ubuntu@{} 'killall iambusy'".format(iplist[i]), stdout=FNULL, shell=True)

