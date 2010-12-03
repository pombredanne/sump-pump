#!/usr/bin/python
#
# genreduce.py - program to generate the input file and correct output files
#                for the SUMP Pump reduce (not mapreduce) regression test.
#
# $Revision$
#
# Usage: genreduce.py
#     This will create the following files:
#         rin1.txt          - Currently the input file for most SUMP Pump
#                             regression tests.  Its format matches what
#                             would come out of Nsort when using the
#                             -match=1 directive.  There is a an initial
#                             character for each record that indicates 
#                             whether 1 key (starting the first key) in the 
#                             record matchs the corresponding key in the
#                             previous record.  This format is required to
#                             to test "reduce" SUMP Pumps.
#                             This file is also exactly 27720 bytes, allowing
#                             it to be used to test all fixed-length record
#                             sizes from 1 to 12 bytes.
#         rin2.txt          - Similar to rin1.txt, except as if using -match=2
#         rin3.txt          - Similar to rin1.txt, except as if using -match=3
#         upper_correct.txt - This is the correct output for the non-reduce
#                             tests (upper, upperfixed, upperwhole) that
#                             simply change all lower case characters to
#                             upper case.
#         rout1_correct.txt - This is the correct output of the reduce test
#                             when rin1.txt is used as the input file.
#         rout2_correct.txt - This is the correct output of the reduce test
#                             when rin2.txt is used as the input file.
#         rout3_correct.txt - This is the correct output of the reduce test
#                             when rin3.txt is used as the input file.
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

def printrec(up, m, x):
    line = '%s %s %s %d' % \
           (alpha3(key[0]), alpha3(key[1]), alpha3(key[2]), x)
    mc = '0'
    if (m >= 1):
        mc = '1'
    print >> rin1, '%s%s' % (mc, line)
    print >> up, '%s%s' % (mc, line.upper())
    if (m == 1):
        mc = '0'
    print >> rin2, '%s%s' % (mc, line)
    if (m == 2):
        mc = '0'
    print >> rin3, '%s%s' % (mc, line)
    
seed(0)     # generate same records each time
up = open('upper_correct.txt', 'w')
rin1 = open('rin1.txt', 'w')
rin2 = open('rin2.txt', 'w')
rin3 = open('rin3.txt', 'w')
rout1 = open('rout1_correct.txt', 'w')
rout2 = open('rout2_correct.txt', 'w')
rout3 = open('rout3_correct.txt', 'w')
dgt = 5
sum[0] = dgt
sum[1] = dgt
sum[2] = dgt
sum[3] = dgt
printrec(up, 0, dgt)
for i in range(1847):  # carefully chosen range so that rin.txt is 27720 bytes
    match = randint(0, 3)
    if (match == 0):
        # 0 matching keys relative to previous record
        printsum(rout1, sum[1])
        sum[1] = 0
        printsum(rout2, sum[2])
        sum[2] = 0
        printsum(rout3, sum[3])
        sum[3] = 0
        key[0] = key[0] + 1
        key[1] = 0
        key[2] = 0
    elif (match == 1):
        # 1 matching keys relative to previous record
        printsum(rout2, sum[2])
        sum[2] = 0
        printsum(rout3, sum[3])
        sum[3] = 0
        key[1] = key[1] + 1
        key[2] = 0
    elif (match == 2):
        # 2 matching keys relative to previous record
        printsum(rout3, sum[3])
        sum[3] = 0
        key[2] = key[2] + 1
    dgt = randint(1, 9)
    sum[0] = sum[0] + dgt
    sum[1] = sum[1] + dgt
    sum[2] = sum[2] + dgt
    sum[3] = sum[3] + dgt
    printrec(up, match, dgt)
printsum(rout1, sum[1])
printsum(rout2, sum[2])
printsum(rout3, sum[3])

