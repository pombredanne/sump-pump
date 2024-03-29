/* main.c - SUMP Pump(TM) program that executes external programs in parallel
 *
 *          SUMP Pump is a trademark of Ordinal Technology Corp
 *
 * $Revision$
 *
 * Copyright (C) 2011, Ordinal Technology Corp, http://www.ordinal.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * Usage: sump [sump pump directives] program_name [program arguments]
 *
 */
#include "sump.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

char Sump_usage[] =
    "sump usage:\n"
    "  sump [sump pump directives] program_name [program arguments]\n"
    "\n"
    "The SUMP (Scale Up MultiProcessor) Pump program reads its input and\n"
    "breaks it into partitions that are approximately the size of its input\n"
    "buffers. Each input partition becomes the standard input of a separate,\n"
    "external invocation of the program specified on the sump command line.\n"
    "The standard outputs of the program invocations are concatenated to\n"
    "form the output of the sump program. For an animated model, see:\n"
    "http://www.ordinal.com/sump.html\n"
    "\n"
    "Directives (keywords are case insensitive, '_' is optional):\n"
    "  -GROUP_BY or -GROUP Group input records for the purpose of reducing\n"
    "                      them. The input should be coming from an nsort\n"
    "                      instance where the \"-match\" directive has\n"
    "                      been declared. This directive prevents records\n"
    "                      with equal keys from being dispersed to more\n"
    "                      than one external program invocation, although\n"
    "                      each program invocation may need to process\n"
    "                      records with more than one key value.\n"
    "\n"
    "  -IN=%s or           Input file name. If not specified, the input\n"
    "    -IN_FILE=%s       is read from standard input.\n"
    "                      The input file name can be followed by one or\n"
    "                      more of the following nsort-style modifiers \n"
    "                      without spaces:\n"
    "                      ,{BUFFERED,BUF} The file will be read with \n"
    "                                normal buffered (not direct) reads.\n"
    "                      ,{DIRECT,DIR} The file will be read with\n"
    "                                direct and asynchronous reads.\n"
    "                      ,{TRANSFER,TRANS,TR}=%d{k,m,g}\n"
    "                                The transfer size (read request size)\n"
    "                                is specified in kilo, mega or giga\n"
    "                                bytes.\n"
    "                      ,{COUNT,CO}=%d The count of the maximum number\n"
    "                                of outstanding asynchronous read \n"
    "                                requests is given.\n"
    "                      Example:\n"
    "                        -in=myfilename,dir,trans=4m,co=4\n"
    "                                The above example specifies an input\n"
    "                                file name of \"myfilename\", with\n"
    "                                direct and asynchronous reads, a\n"
    "                                request size of 4 MB, and a maximum\n"
    "                                of 4 outstanding asynchronous read\n"
    "                                requests at any time.\n"
    "\n"
    "  -IN_BUF_SIZE=%d[k,m,g] Overrides default input buffer size (1MB).\n"
    "                      If a 'k', 'm' or 'g' suffix is specified, the\n"
    "                      specified size is multiplied by 2^10, 2^20 or\n"
    "                      2^30 respectively.\n"
    "\n"
    "  -OUT=%s or          The output file name.  If not defined, the\n"
    "    -OUT_FILE=%s      output is written to standard output.\n"
    "                      The output file name can be followed by one or\n"
    "                      more modifiers (see input file modifiers).\n"
    "\n"
    "  -OUT_BUF_SIZE=%d[x,k,m,g] Overrides default output buffer size \n"
    "                      (2x the input buffer size). If the size ends\n"
    "                      with a suffix of 'x', the size is used as a\n"
    "                      multiplier of the input buffer size. If a\n"
    "                      'k', 'm' or 'g' suffix is specified, the\n"
    "                      specified size is multiplied by 2^10, 2^20 or\n"
    "                      2^30 respectively. It is not an error if the\n"
    "                      output of a program invocation exceeds the output\n"
    "                      buffer size, but it can potentially result in\n"
    "                      loss of parallelism.\n"
    "\n"
    "  -REC_SIZE=%d        Defines the input record size in bytes. If not\n"
    "                      specified, records must consist of ascii or\n"
    "                      utf-8 characters and be terminated by a newline\n"
    "                      character.\n"
    "\n"
    "  -THREADS=%d         Defines the maximum number of simultaneous\n"
    "                      invocations of the external program.  The\n"
    "                      default maximum is the number of logical\n"
    "                      processors in the system.\n"
    "\n"
    "  -WHOLE or           Overrides the default input record type of ascii\n"
    "    -WHOLE_BUF        or utf-8 records delimited by newline character.\n"
    "                      Instead, the input is treated as raw data and\n"
    "                      the partitioning is done strictly by the input\n"
    "                      buffer size without regard to lines of text.\n"
    "\n"
    "  -DEFAULT_FILE_MODE={BUFFERED,BUF,DIRECT,DIR}  Set the default file\n"
    "                      access mode for both input and output files.  If\n"
    "                      none is specified, the direct mode is used to\n"
    "                      access input and output files for which a\n"
    "                      BUFFERED file modifier is not specified.\n"
    "\n"
    "In addition to being used on the sump program command line, these\n"
    "directives may also be specified in a 'SUMP_PUMP' environment variable.\n"
    "\n"
    "Copyright (C) 2011, Ordinal Technology Corp, http://www.ordinal.com\n"
    "\n"
    "This program is free software; you can redistribute it and/or\n"
    "modify it under the terms of Version 2 of the GNU General Public\n"
    "License as published by the Free Software Foundation.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program; if not, write to the Free Software Foundation,\n"
    "Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ;

int main(int argc, char *argv[])
{
    sp_t                sp;
    int                 ret;

    if (argc == 1 || (argc > 1 && !strcmp(argv[1], "-?")))
    {
        fprintf(stderr, "%s", Sump_usage);
        fprintf(stderr, "\nSUMP Pump version %s\n", sp_get_version());
        return (1);
    }
    
    ret = sp_start(&sp, NULL,
                   "-utf_8 "
                   "-in_file=<stdin> "
                   "-in_buf_size=1m "
                   "-out_file[0]=<stdout> "
#if defined(SUMP_PIPE_STDERR)
                   "-outputs=2 "
                   "-out_file[1]=<stderr> "
                   "-out_buf_size[1]=4k "
#endif
                   "%s", sp_argv_to_str(argv + 1, argc - 1));
    if (ret != SP_OK)
    {
        fprintf(stderr, "sump: %s\n", sp_get_error_string(sp, ret));
        return (1);
    }
    if ((ret = sp_wait(sp)) != SP_OK)
    {
        fprintf(stderr, "sump: %s\n", sp_get_error_string(sp, ret));
        return (1);
    }
    return (0);
}

