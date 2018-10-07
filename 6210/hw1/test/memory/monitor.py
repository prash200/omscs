#!/usr/bin/python

from __future__ import print_function
import libvirt
import sched, time
import os

CONFIG_FILE = '../vmlist.conf'

global conn
global vmlist
global vmobjlist
global s
global memoryinfolist

s = sched.scheduler(time.time, time.sleep)


def run(sc):
    for i in range(len(vmobjlist)):
        memoryinfolist[i] = vmobjlist[i].memoryStats()
        print("{}: {} {}"
            .format(vmlist[i], 
                    memoryinfolist[i]['actual'] / 1024.0,
                    memoryinfolist[i]['unused'] / 1024.0))
    print('-' * 80)
    
    t = os.popen('virsh nodememstats').read()
    print(t)
    print('-' * 80)

    s.enter(2, 1, run, (sc,))

if __name__ == '__main__':
    conn = libvirt.open('qemu:///system')
    vmlist = open(CONFIG_FILE, 'r').read().strip().split()

    vmobjlist = []   

    for vmname in vmlist:
        vm = conn.lookupByName(vmname)
        if vm:
            vmobjlist.append(vm)
        else:
            print('Unable to locate {}.'.format(vmnane))
            exit(-1)
    
    for vm in vmobjlist:
        vm.setMemoryStatsPeriod(1)   
 
    memoryinfolist = [None] * len(vmobjlist)

    s.enter(2, 1, run, (s,))
    s.run()
