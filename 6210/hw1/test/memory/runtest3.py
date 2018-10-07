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

    print('Start testcase 3')
    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    iplist = []

    for vmname in vmlist:
        iplist.append(os.popen('uvt-kvm ip {}'.format(vmname)).read().strip())

    FNULL = open(os.devnull, 'w') 
 
    for i in range(len(vmlist)):
        if i == 0 or i == 1:
            print('{} start running job A.'.format(vmlist[i]))
            subprocess.Popen("ssh ubuntu@{} '~/testcases/3/run A'".format(iplist[i]), stdout=FNULL, shell=True)
        else:
            print('{} start running job B.'.format(vmlist[i]))
            subprocess.Popen("ssh ubuntu@{} '~/testcases/3/run B'".format(iplist[i]), stdout=FNULL, shell=True)

        

