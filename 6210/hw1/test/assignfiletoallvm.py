#!/usr/bin/python

from __future__ import print_function
import libvirt
import os
import sys

CONFIG_FILE = ''
if len(sys.argv) == 3 and sys.argv[1] == 'memory':
    print('using memory conf file')
    CONFIG_FILE = 'vmlist.conf.memory'
elif len(sys.argv) == 3 and sys.argv[1] == 'cpu':
    print('using cpu conf file')
    CONFIG_FILE = 'vmlist.conf.cpu'

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: ./assignfiletoallvm.py [memory/cpu] [file]')
        exit(-1)

    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    iplist = []
    filename = sys.argv[2]

    for vmname in vmlist:
        iplist.append(os.popen('uvt-kvm ip {}'.format(vmname)).read().strip())
    for ip in iplist:
        print('Copy {} to {}.'.format(filename, ip))
        os.popen('scp -r {} ubuntu@{}:~/'.format(filename, ip))
