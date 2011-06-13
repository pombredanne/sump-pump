#!/usr/bin/python
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
#
# runregtests - run the sump pump regression tests
#
# $Revision$
#
# Usage: runregtests [iteration_count]
#        If the iteration_count parameter is not present, a count of
#        1,000,000 iterations is used.
#        The are 4 sump pump regression test programs that are run.
#        All tests read the file rin.txt and write the file rout.txt.
#        The input buffer size, output buffer size, input file read size,
#        number of input buffers and number of tasks are all varied to test
#        for various internal sump pump occurences.  The regression tests are:
#           upper       Reads the rin.txt file as lines of text and changes
#                       each lower case character to upper case.
#           upperfixed  Same as "upper" but input records are fixed-size,
#                       not lines of text.
#           upperwhole  Same as upper, but entire input buffers are operated
#                       on a once instead of record at a time.
#           reduce      Tests a sump pump "reduce" operation for lines of text.
#           reducefixed Tests a sump pump "reduce" operation for fixed-width
#                       records.
#
import os
import sys
import random
from random import randint

iteration_count = 10000
if (len(sys.argv) == 2):
    iteration_count = int(sys.argv[1])
# print sump pump svn version
os.system('sumpversion')
# run the "oneshot" test just once */
print 'running one shot test'
ret = os.system('oneshot')
if ret != 0:
    print 'oneshot test failed'
    sys.exit()
print 'oneshot test succeeded'
print 'running randomized tests...'
for i in range(iteration_count):
    insize = randint(1,50)
    outsize = randint(1,50)
    rwsize = randint(0,50)  # if 0, then test normal file read input
                            # otherwise, test sp_write_input() input
    inbufs = randint(1,3)
    tasks = randint(1,3)
    threads = randint(1,20)
    rec_size = ''
    reduce_input_file = ''
    if randint(0, 3) != 0:
        # perform a not-word-count test
        if randint(0, 1) == 0:
            if randint(0, 1) == 0:
                testprog = 'reduce'
            else:
                testprog = 'reducefixed'
            match_keys = str(randint(1, 3))
            correctoutput = 'rout' + match_keys + '_correct.txt'
            reduce_input_file = ' -IN_FILE=rin' + match_keys + '.txt'
        else:
            testindex = randint(1,3)
            if testindex == 1:
                testprog = 'upper' 
                correctoutput = 'upper_correct.txt'
                if randint(0,1) == 0:
                    testprog = testprog + ' onebyone'
            elif testindex == 2:
                testprog = 'upperfixed' 
                correctoutput = 'upper_correct.txt'
                rec_size = ' -REC_SIZE=' + str(randint(2,12))
                if randint(0,1) == 0:
                    testprog = testprog + ' onebyone'
            elif testindex == 3:
                testprog = 'upperwhole' 
                correctoutput = 'upper_correct.txt'
        cmd = './' + testprog + rec_size + reduce_input_file + \
              ' -OUT_BUF_SIZE[0]=' + str(outsize) + \
              ' -IN_BUF_SIZE=' + str(insize) + \
              ' -RW_TEST_SIZE=' + str(rwsize) + \
              ' -IN_BUFS=' + str(inbufs) + \
              ' -TASKS=' + str(tasks) + \
              ' -THREADS=' + str(threads) 
    else:
        cmd = './sump -in_buf_size=' + str(randint(100,10000)) + \
              ' ./map < hounds.txt | ' \
              'nsort -format:sep=tab -field=word,count,decimal,max=' + \
              str(randint(1,4)) + \
              ' -key:word -sum:count -nowarn -match | ' \
              './sump -in_buf_size=' + str(randint(100,1000)) +\
              ' -group ./red > rout.txt'
        correctoutput = 'correct_hounds_wc.txt'
    print i, ' ', cmd
    ret = os.system(cmd)
    if ret != 0:
        print 'error: ', cmd, ' returned: ', ret
        sys.exit()
    ret = os.system('diff -b rout.txt ' + correctoutput)
    if ret != 0:
        print 'error: cmp returned: ', ret
        sys.exit()
print 'sump pump regression test succeeded'
