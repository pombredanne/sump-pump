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
# runperftests - run the sump pump performance tests
#
# $Revision$
#
# Usage: runperftests
#   There are 5 sump pump performance test programs that are run. Each
#   test is run with a increasing series of processors, starting with 1
#   and increasing by factors of 2 until the number of processors
#   present in the machine is reached. For each processor count, the
#   performance test program is run 5 times.  The median elapsed time
#   is used a the final result for that number of processors.  The
#   performance tests are:
#
#   lookup      This program is roughly equivalent to the Unix
#               join program in functionality, except that is does
#               not require that its inputs be sorted.  The file
#               lookupin.txt as a series on lines with comma-separted
#               lines.  If the first field in a line does not match
#               one of the first-field values in the file
#               lookupref.txt, then the lookupin.txt line is
#               written to the file nomatch.txt.  If there is a
#               match, then the non-first fields in the lookupref.txt
#               line are appended to lookupin.txt line and written
#               to the match.txt file.
#
#   billing     This program reads an input file of synthetic
#               phone call records and produces a phone bill for
#               each originating phone number.  This is done
#               using a "map" sump pump that reads the input file
#               and performs a trivial editing of each phone call
#               record. The output of the map sump pump goes to an
#               instance of the Nsort subroutine library that sorts
#               the phone records primarily on originating phone
#               number, and secondarily on date and time of the call.
#               The Nsort output goes to a "reduce" sump pump where
#               the pump functions read the sorted output in
#               groups organized by the primary key, the originating
#               phone numbers.  The reduce pump functions format a 
#               "phone bill" for each originating phone number.
#
#   gensort     A sump pump implementation of sort input file
#               generator program used by the sort benchmarks
#               defined at sortbenchmark.org.  Each pump function
#               reads a single instruction structure sent from the
#               main thread. The pump function then generates the
#               number of records specified in the instruction structure,
#               starting with the specified beginning record number.
#
#   valsort     A sump pump implementation of sort output file
#               validation program used by the sort benchmarks
#               defined at sortbenchmark.org.  Each pump function
#               reads a block of records and produces a structure
#               to summarize the records in the block.  The main
#               main program thread reads the summary structures
#               to validate the correct order and checksum of records
#               in the file.
#             
#   spgzip      A sump pump implementation of the gzip program.
#               Reads the spgzipinput file, compresses it 512KB
#               block at a time (in parallel), and writes the 
#               compressed output to spgzipinput.gz
#
import os
import sys
import time
import platform


n_processors = 0
run_count = 5

def clear_buffered_files():
    # write dummy file size of main memory to clear other files from memory
    os.system('dd if=/dev/zero of=dummy count=3072718 bs=4096 2> /dev/null')
    os.system('sync')
    return

def run_program(cmd_base, cmd_suffix, processors_used, run_count, silent, io_bytes):
    results = []
    cmd = cmd_base + ' -threads=' + str(processors_used) + cmd_suffix
    
    # if less than all processors are used, then use processor
    if (processors_used < n_processors):   
        cmd = 'taskset -c 0-' + str(processors_used - 1) + ' ' + cmd
    if (silent == False):
        print 'testing: ' + cmd     
        print 'Run     Elapsed     User   System  IO MB/sec'
    for i in range(run_count):
        begin_times = os.times()
        ret = os.system(cmd)
        end_times = os.times()
        if (ret != 0):
            print cmd + ' failed: ' + str(ret)
            sys.exit(ret)
        elapsed = end_times[4] - begin_times[4]
        user = end_times[2] - begin_times[2]
        system = end_times[3] - begin_times[3]
        if (silent == False):
            if (elapsed > 0):
                io_speed_str = '%10.1f' % (io_bytes / 1000000 / elapsed)
            else:
                io_speed_str = '          -'
            print '%3d    %8.2f %8.2f %8.2f %s' % \
                  (i, elapsed, user, system, io_speed_str)
        results.append([elapsed, user, system])
        os.system('sync')  # flush writes to disk, but don't measure time
    results.sort()
    if (silent == False):
        if (results[run_count / 2][0] > 0):
            io_speed_str = '%10.1f' % \
                           (io_bytes / 1000000 / results[run_count / 2][0])
        else:
            io_speed_str = '          -'
        print 'Median %8.2f %8.2f %8.2f %s' % \
              (results[run_count / 2][0],
               results[run_count / 2][1],
               results[run_count / 2][2],
               io_speed_str)
    return

def run_program_set(cmd_base, cmd_suffix, io_bytes):
    print  # print empty line
    # make intial run to get input file in memory
    print 'initial, to be discarded, run of: ' + cmd_base + cmd_suffix
    run_program(cmd_base, cmd_suffix, n_processors, 1, True, io_bytes)
    proc_count_list = [n_processors]
    test_procs = n_processors
    while (test_procs > 1):
        test_procs = (test_procs + 1) / 2
        proc_count_list.insert(0, test_procs)
    for test_procs in proc_count_list:
        run_program(cmd_base, cmd_suffix, test_procs, run_count, False, io_bytes)
    return


sysstr = platform.system()
if ((len(sysstr) >= 6 and sysstr[0:6] == 'CYGWIN') or (len(sysstr) >= 7 and sysstr[0:7] == 'Windows')):
    print 'SUMP Pump performance tests currently do not work on Cygwin or Windows, sorry.'
    # The main problem here is that 'taskset', used to restrict the number 
    # of processors used, is not available on Windows.  The other minor 
    # problem is that zlib is only available as a 32-bit library.
    sys.exit(1)
print 'SUMP Pump In-Memory Performance Tests'
os.system('date')
os.system('sumpversion')
n_processors = os.sysconf('SC_NPROCESSORS_ONLN')
print str(n_processors) + ' CPUs'
if (os.popen('uname', 'r').readline() == 'Linux\n'):
    proc_lines = os.popen('grep \"model name\" /proc/cpuinfo', 'r').readlines()
    print proc_lines[0][0:-1]
    os.system('grep \"MemTotal\" /proc/meminfo')
run_program_set('lookup', '', 913363767 + 609324195 + 364785068)
run_program_set('billing billing_input.txt billing_output.txt', '', 535985261 + 505064050)
run_program_set('wordcount.sh', '', 105311535 + 3101779)
run_program_set('gensort 10000000 sortinput.txt', '', 0 + 1000000000)
run_program_set('valsort sortoutput.txt', ' > /dev/null', 1000000000 + 0)
run_program_set('spgzip', ' < spgzipinput > spgzipinput.gz', 274747890 + 139887849)
