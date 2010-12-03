/* reduce.c - SUMP Pump(TM) regression test program that tests reduce
 *            (GROUP_BY_KEY) functionality.  Used in conjunction with
 *            runregtests.py.
 *            SUMP Pump is a trademark of Ordinal Technology Corp
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
 * Usage: reduce [sump pump directives]
 *
 */
#include "sump.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>


/* reduce pump func
 */
int reduce(sp_task_t t, void *unused)
{
    char        *rec;
    int         sum;
    char        last_rec[30];
        
    /* do any pre-group initialization */
    sum = 0;

    /* for each record in the group */
    while (pfunc_get_rec(t, &rec) > 0)
    {
        sum += atoi(rec + 12);   /* count field is always at byte 12 */
        strncpy(last_rec, rec, 12);
        last_rec[12] = '\0';
    }
        
    /* do any group post processing here */
    pfunc_printf(t, 0, "%s%d\n", last_rec, sum); /* write last rec, group sum*/

    return (SP_OK);
}


int main(int argc, char *argv[])
{
    sp_t                sp;
    int                 ret;

    if (sp_start(&sp, reduce, 
                 "UTF_8 GROUP_BY OUT_FILE[0]=rout.txt %s",
                 sp_argv_to_str(argv + 1, argc - 1)) != SP_OK)
        fprintf(stderr, "sp_start() error\n"), exit(1);

    if ((ret = sp_wait(sp)) != SP_OK)
        fprintf(stderr, "sp_wait: %s\n", sp_get_error_string(sp, ret)), exit(1); 
    
    return (0);
}

