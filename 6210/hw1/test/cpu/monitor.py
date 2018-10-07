#!/usr/bin/python

from __future__ import print_function
import libvirt
import sched, time

CONFIG_FILE = '../vmlist.conf'

global conn
global vmlist
global vmobjlist
global cpulist
global vminfolist
global numpcpu
global s

s = sched.scheduler(time.time, time.sleep)

def get_pcpu():
    hostinfo = conn.getInfo()
    return hostinfo[4] * hostinfo[5] * hostinfo[6] * hostinfo[7]

def which_cpu(vcpuinfo):
    return vcpuinfo[0][0][3]

def which_usage(newinfo, oldinfo):
    return ( newinfo[0][0][2] - oldinfo[0][0][2]) * 1.0 / (10 ** 9)

def run(sc):
    for i in range(numpcpu):
        cpulist[i] = {}
        cpulist[i]['mapping'] = []
        cpulist[i]['usage'] = 0.0

    for i in range(len(vmobjlist)):
        newinfo = vmobjlist[i].vcpus()
        print('{}: {}'.format(vmlist[i], newinfo))
        if vminfolist[i]:
            cpu = which_cpu(newinfo)
            usage = which_usage(newinfo, vminfolist[i])
            cpulist[cpu]['mapping'].append(vmlist[i])
            cpulist[cpu]['usage'] += usage
        vminfolist[i] = newinfo
    
    print('-' * 80)
    for i in range(numpcpu):
        print('{}: {} {}'.format(i, cpulist[i]['usage'] * 100, cpulist[i]['mapping']))

    print('-' * 80)

    s.enter(1, 1, run, (sc,))

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
    
    vminfolist = [None] * len(vmobjlist)
    numpcpu = get_pcpu()
    cpulist = [None] * numpcpu 
    
    s.enter(1, 1, run, (s,))
    s.run()
