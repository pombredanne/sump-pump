#!/usr/bin/env python
#
# Mapper program as suggested in Pete Warden's MapReduce class
#
# $Revision$
#
import sys
import re

for line in sys.stdin:
  words = re.findall('[A-Za-z]+', line)
  for word in words:
    print word.lower()+'\t'+str(1)    
