#!/usr/bin/python

from __future__ import print_function
import libvirt
import os
import sys

CONFIG_FILE = 'vmlist.conf.cpu'
if len(sys.argv) > 1:
    print('USAGE: ./startallvm.py [memory]')
    exit(1)
elif len(sys.argv) == 1 and sys.argv[1] == 'memory':
    print('using CPU conf file')
    CONFIG_FILE = 'vmlist.conf.memory'
else:
    print('using cpu conf file, to use memory conf file use ./startallvm.py memory')

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: ./assignfiletoallvm.py [file]')
        exit(-1)

    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()
    iplist = []
    filename = sys.argv[1]

    for vmname in vmlist:
        iplist.append(os.popen('uvt-kvm ip {}'.format(vmname)).read().strip())
    for ip in iplist:
        print('Copy {} to {}.'.format(filename, ip))
        os.popen('scp -r {} ubuntu@{}:~/'.format(filename, ip))
