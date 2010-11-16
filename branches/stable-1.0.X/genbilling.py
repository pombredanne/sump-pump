#!/usr/bin/python
#
# genbilling.py - program to generate the input file and correct output
#                 to the chaingang billing example.
#
# $Revision$
#
# Usage: genbilling.py 1000000 | sort -k 5,5 -t , > billing_input.txt
#        This will also create billing_correct_output.txt which can be
#        used to verify the correctness of the billing_output.txt file
#        created by the chaingang billing example.
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


def format_phone_number(x):
    ten_digits = '%10d' % (x)
    return '(%s)%s-%s' % (ten_digits[0:3], ten_digits[3:6], ten_digits[6:10])

# gen_account_calls - generate a set a phone call records for the next
#                     sequential account number.
#
def gen_account_calls(acc_num):
    num_calls = randint(1,19)  # number of calls range from 1 to 19
    print >> mr_output, format_phone_number(acc_num)
    total_minutes = 0
    minimum = 200
    maximum = 0
    for i in range(num_calls):
        called_num = randint(1000000000, 9999999999) # phone number called
        day = i + 1                   # day of phone call
        hour = randint(0, 23)         # hour of phone call
        minute = randint(0, 59)       # minute of phone call
        length = randint(1, 30)       # call length in minutes
        print '%d,%d,2010:01:%02d:%02d:%02d:00,%d,%d' % \
            (acc_num, 
             called_num,
             day,
             hour,
             minute,
             length,
             randint(0,99999999))       # random number used to shuffle call
                                        # records into a pseudo-random order
        print >> mr_output, '  %s  Jan %2d %02d:%02d   %2d' % \
            (format_phone_number(called_num),
             day,
             hour,
             minute,
             length)
        total_minutes = total_minutes + length
        if (length < minimum):
            minimum = length
        if (length > maximum):
            maximum = length
    print >> mr_output, 'Total minutes                 %4d' % (total_minutes)
    print >> mr_output, 'Average minutes               %4d' % \
                                                   (total_minutes / num_calls)
    print >> mr_output, 'Minimum                       %4d' % (minimum)
    print >> mr_output, 'Maximum                       %4d\n' % (maximum)
    # sort list
    # produce correct output
        

if (len(sys.argv) != 2):
    print 'usage: genbilling number_records'
else:
    seed(0)  # generate same records each time
    mr_output = open('billing_correct_output.txt', 'w')
    for i in range(int(sys.argv[1])):
        # generate the account phone call records for phone numbers starting
        # with 650-200-0000
        gen_account_calls(6502000000 + i)  
