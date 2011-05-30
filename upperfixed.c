/* upperfixed.c - SUMP Pump(TM) regression test program that allows a
 *                variety of sump pump parameters to be specified when
 *                using a pump function on fixed-length records.  Used
 *                in conjunction with runregtests.py.
 *                SUMP Pump is a trademark of Ordinal Technology Corp
 *
 * $Revision$
 *
 * Copyright (C) 2010, Ordinal Technology Corp, http://www.ordinal.com
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
 * Usage: upperfixed [sump pump directives]
 *
 */
#include "sump.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#if !defined(win_nt)
# include <ctype.h>
#endif

int     Rec_size = 1;

int uppercase_while(sp_task_t t, void *unused)
{
    unsigned char        *rec;
        
    while (pfunc_get_rec(t, &rec) > 0)
    {
        unsigned char   *p;

        for (p = rec; (p - rec) < Rec_size; p++)
            *p = toupper(*p);
        pfunc_write(t, 0, rec, Rec_size);
    }
    return (SP_OK);
}

int uppercase_justone(sp_task_t t, void *unused)
{
    unsigned char       *rec;
    int                 ret;
    unsigned char       *p;

    /* get just one input record and force sump pump infrastructure to
     * call this pump function again to get next record in the task
     * input (if any).
     */
    ret = (int)pfunc_get_rec(t, &rec);
    if (ret <= 0)
        return (pfunc_error(t, "first sp_get_rec() returned rec of size: %d\n",
                            ret));

    for (p = rec; (p - rec) < Rec_size; p++)
        *p = toupper(*p);
    pfunc_write(t, 0, rec, Rec_size);
    return (SP_OK);
}

int main(int argc, char *argv[])
{
    sp_t                sp;
    int                 ret;
    int                 onebyone = 0;

    if (argc > 1 && !strcmp(argv[1], "onebyone"))
    {
        onebyone = 1;
        argc--;
        argv++;
    }

    if (argc > 1 && !strncmp(argv[1], "-REC_SIZE=", 10))
        Rec_size = atoi(argv[1] + 10);
    ret = sp_start(&sp,
                   onebyone ? uppercase_justone : uppercase_while,
                   "-IN_FILE=rin1.txt -OUT_FILE[0]=rout.txt %s",
                   sp_argv_to_str(argv + 1, argc - 1));
    if (ret != SP_OK)
    {
        fprintf(stderr, "sp_start: %s\n", sp_get_error_string(sp, ret));
        return (1);
    }
    if ((ret = sp_wait(sp)) != SP_OK)
    {
        fprintf(stderr, "sp_wait: %s\n", sp_get_error_string(sp, ret));
        return (1);
    }
    return (0);
}

