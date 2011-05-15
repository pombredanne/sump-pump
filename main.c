/* main.c - SUMP Pump(TM) program that executes external programs
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
 * Usage: main [sump pump directives] -exec program_name ...
 *
 */
#include "sump.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
    sp_t                sp;
    int                 ret;

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

