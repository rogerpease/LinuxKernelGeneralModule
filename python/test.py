#!/usr/bin/env python3 

import time
import re 

def clean(i):
   return re.search('([0-9]+)',i)[0]  

def selftest():

   with open("/proc/generalmodule_processlist",'r') as fp:
      d = fp.readlines()
   assert len(d) > 20


   with open("/proc/generalmodule_intcount",'r') as fp:
      d = fp.readlines()
   assert len(d) == 1
   intcount_init = int(clean(d[0])) 
   time.sleep(5) 
   with open("/proc/generalmodule_intcount",'r') as fp:
      d = fp.readlines()
   intcount_fin  = int(clean(d[0])) 
   assert (intcount_fin > intcount_init),intcount_init 
   print("PASS")


selftest()
