#!/usr/bin/python
#
# genreduce.py - program to generate the input file and correct output files
#                for the chaingang reduce (not mapreduce) regression test.
#
# $Revision$
#
# Usage: genreduce.py
#     This will create the following files:
#         rin.txt           - Currently the input file for all chaingang
#                             regression tests.  Its format matches what
#                             would come out of Nsort when using the
#                             CG_KEY_DIFF flag.  There is a an initial
#                             character that indicates number of sequential
#                             keys (starting the first key) in the record
#                             that match the corresponding keys in the
#                             previous record.  This format is required to
#                             to test "reduce" chaingangs, i.e. those that
#                             use the GROUP_BY_KEYS=n directive.
#                             This file is also exactly 27720 bytes, allowing
#                             it to be used to test all fixed-length record
#                             sizes from 1 to 12 bytes.
#         upper_correct.txt - This is the correct output for the non-reduce
#                             tests (upper, upperfixed, upperwhole) that
#                             simply change all lower case characters to
#                             upper case.
#         rout0_correct.txt - This is the correct output of the reduce test
#                             when GROUP_BY_KEYS=0 is used.
#         rout1_correct.txt - This is the correct output of the reduce test
#                             when GROUP_BY_KEYS=1 is used.
#         rout2_correct.txt - This is the correct output of the reduce test
#                             when GROUP_BY_KEYS=2 is used.
#         rout3_correct.txt - This is the correct output of the reduce test
#                             when GROUP_BY_KEYS=3 is used.
#
# Copyright (C) 2010, Ordinal Technology Corp, http://www.ordinal.com
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of Version 2 of the GNU General Public
# License as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
import os
import sys
from random import randint, seed


key = [0, 0, 0]
sum = [0, 0, 0, 0]

# alpha3 - convert an integer into 3 lower-case letters followed by a space
#
#  for instance: alpha3(0)  --> 'aaa'
#                alpha3(1)  --> 'aab'
#                alpha3(26) --> 'aba'
#                alpha3(27) --> 'abb'
#
def alpha3(x):
    return '%s%s%s' % (chr(97 + ((x / (26 * 26)) % 26)),
                       chr(97 + ((x / 26) % 26)),
                       chr(97 + (x % 26)))

def printsum(f, s):
    print >> f, '%s %s %s %d' % \
          (alpha3(key[0]), alpha3(key[1]), alpha3(key[2]), s)

def printrec(rin, up, d, x):
    line = '%d%s %s %s %d' % \
           (d, alpha3(key[0]), alpha3(key[1]), alpha3(key[2]), x)
    print >> rin, line
    print >> up, line.upper()
    
seed(0)     # generate same records each time
rin = open('rin.txt', 'w')
up = open('upper_correct.txt', 'w')
rout0 = open('rout0_correct.txt', 'w')
rout1 = open('rout1_correct.txt', 'w')
rout2 = open('rout2_correct.txt', 'w')
rout3 = open('rout3_correct.txt', 'w')
dgt = 5
sum[0] = dgt
sum[1] = dgt
sum[2] = dgt
sum[3] = dgt
printrec(rin, up, 0, dgt)
for i in range(1847):  # carefully chosen range so that rin.txt is 27720 bytes
    diff = randint(0, 3)
    if (diff == 0):
        printsum(rout1, sum[1])
        sum[1] = 0
        printsum(rout2, sum[2])
        sum[2] = 0
        printsum(rout3, sum[3])
        sum[3] = 0
        key[0] = key[0] + 1
        key[1] = 0
        key[2] = 0
    elif (diff == 1):
        printsum(rout2, sum[2])
        sum[2] = 0
        printsum(rout3, sum[3])
        sum[3] = 0
        key[1] = key[1] + 1
        key[2] = 0
    elif (diff == 2):
        printsum(rout3, sum[3])
        sum[3] = 0
        key[2] = key[2] + 1
    dgt = randint(1, 9)
    sum[0] = sum[0] + dgt
    sum[1] = sum[1] + dgt
    sum[2] = sum[2] + dgt
    sum[3] = sum[3] + dgt
    printrec(rin, up, diff, dgt)
printsum(rout0, sum[0])
printsum(rout1, sum[1])
printsum(rout2, sum[2])
printsum(rout3, sum[3])

