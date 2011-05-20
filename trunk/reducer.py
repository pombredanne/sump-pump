#!/usr/bin/env python
#
# Reducer program as suggested in Pete Warden's MapReduce class
# 
# $Revision$
#
import sys

previous_key = None
total = 0

def output(previous_key, total):
  if previous_key is not None:
    print previous_key+' was found '+str(total)+' times'

for line in sys.stdin:
  key, value = line.split('\t', 1)
  if key != previous_key:
    output(previous_key, total)
    previous_key = key
    total = 0
  total += int(value)
output(previous_key, total)

