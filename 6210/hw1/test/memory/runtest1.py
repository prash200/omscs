#!/usr/bin/python

from __future__ import print_function
import libvirt
import os
import sys
import subprocess

CONFIG_FILE = '../vmlist.conf'

if __name__ == '__main__':

    p = subprocess.Popen('cd .. && ./setallmemory.py 512', shell=True)
    p.communicate()

    print('Start testcase 1')
    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    iplist = []

    for vmname in vmlist:
        iplist.append(os.popen('uvt-kvm ip {}'.format(vmname)).read().strip())

    FNULL = open(os.devnull, 'w') 
    print('{} start running.'.format(vmlist[0]))
    subprocess.Popen("ssh ubuntu@{} '~/testcases/1/run'".format(iplist[0]), stdout=FNULL, shell=True)

