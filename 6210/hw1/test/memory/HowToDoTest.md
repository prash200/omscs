### Configuration

1. Create 4 virtual machines.
2. Put their name in vmlist.conf
3. Shutdown all of them and run setallmaxmemory.py

### Run testcases

1. Run startallvm.py
2. Run makeall.sh
3. Run assignall.sh
4. Run your monitor tool, providing a simple monitor.py. Others are okay.
5. Run runtest1.py, runtest2.py, runtest3.py. (Note: using subprocess, experiment continues after main script exits.)

### Cleanup
1. Run shutdownallvm.py or destroyallvm.py 
