#!/usr/bin/python
#
# gencsv.py - generate synthetic, comma-separated data
#
# $Revision$
#
# Usage: gencsv.py random_seed record_count [str,u4,seq]+
#     random_seed     a number to be used as the random number generator seed
#     record_count    the number of records to be generated
#     str             indicates a string field should be generated
#     u4              an unsigned 4-byte integer field should be generated
#     seq             a sequence number field should be generated
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
from random import randrange, seed, randint

recnum = 0

def gen_rec(fmtlist):
    cols = []
    for fmt in fmtlist:
        if fmt == 'str':
            len = randrange(3,6)
            if (len == 3):
                cols.append('%s%s%s' % 
                    (chr(97 + randrange(26)), 
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26))))
            elif (len == 4):
                cols.append('%s%s%s%s' % \
                    (chr(97 + randrange(26)), 
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26))))
            elif (len == 5):
                cols.append('%s%s%s%s%s' % \
                    (chr(97 + randrange(26)), 
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26))))
            else:
                cols.append('%s%s%s%s%s%s' % \
                    (chr(97 + randrange(26)), 
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26)),
                     chr(97 + randrange(26))))
        elif fmt == 'seq':
            cols.append('%d' % (recnum))
        elif fmt == 'i1':
            cols.append('%d' % (randint(-128, 127)))
        elif fmt == 'i2':
            cols.append('%d' % (randint(-32768, 32767)))
        elif fmt == 'i4':
            cols.append('%d' % (randint(-2147483648, 2147483647)))
        elif fmt == 'i8':
            cols.append('%d' % (randint(-9223372036854775808L, 9223372036854775807L)))
        elif fmt == 'u1':
            cols.append('%d' % (randint(0, 255)))
        elif fmt == 'u2':
            cols.append('%d' % (randint(0, 65535)))
        elif fmt == 'u4':
            cols.append('%d' % (randint(0, 4294967296)))
        elif fmt == 'u8':
            cols.append('%d' % (randint(0, 18446744073709551616L)))
        else:
            sys.exit('unknown column format: %s' % (fmt))
    print ','.join(cols)

if (len(sys.argv) < 4):
    print 'usage: gencsv seed number_records format'
else:
    seed(int(sys.argv[1]))
    fmtlist = sys.argv[3 : ]
    for i in range(int(sys.argv[2])):
        gen_rec(fmtlist)
        recnum = recnum + 1
