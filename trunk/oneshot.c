/* oneshot.c - SUMP Pump(TM) regression test for error handling and other
 *             one time tests.
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
 * Usage: oneshot
 */

#include "sump.h"
#include <stdio.h>
#include <string.h>


char    Pump_func_error_message[] = "A pump function error occurred";

/* error_pump - error prone pump function
 */
int error_pump(sp_task_t t, void *unused)
{
    /* declare pump function error and return */
    return (pfunc_error(t, Pump_func_error_message));
}


int main(int argc, char *argv[])
{
    sp_t                sp;
    int                 ret;
    const char          *error_str;
    char		*longline = "A really long line -------------------\n";
    int                 longlinelen;
    char                *def;
    char                buf[4];
    int                 i;
    char                dummy;

    /* have pump function raise an error that causes a sp_write_input() call
     * to fail.
     */
    def = "THREADS=1 TASKS=1 IN_BUF_SIZE=1 REC_SIZE=1";
    ret = sp_start(&sp, error_pump, def);
    if (ret != SP_OK)
    {
        fprintf(stderr, "return status from sp_start(&sp, error_pump, "
                "\"%s\") is not SP_OK: %d\n", def, ret);
        return (1);
    }
    /* should get an error before loop completes */
    for (i = 0; i < 10; i++)  
    {
        ret = sp_write_input(sp, &dummy, 1);
        if (ret == 1)   /* if successful */
        {
            if (i == 9)
            {
                fprintf(stderr, "return status from sp_start(&sp, error_pump"
                        ", \"%s\") never failed\n", def);
                return (1);
            }
            continue;  /* try again */
        }
        if (ret < 0)
        {
            error_str = sp_get_error_string(sp, ret);
            if (strstr(error_str, Pump_func_error_message) == NULL)
            {
                fprintf(stderr, "error string after sp_write_input() failure "
                        "does not include sp_error() string:\n%s\n",
                        error_str);
                return (1);
            }
            break;  /* success! */
        }
    }
                   
    /* have pump function raise an error that causes a sp_read_output() call
     * to fail.
     */
    
    /* force Nsort licensing error then verify error return value and string
     * THIS TEST MUST BE THE FIRST SORT!  The empty license string trick 
     * only works if there has not yet been a successfully licensed sort.
     */
    ret = sp_start_sort(&sp,
                        "-license=\"\" ");
    if (ret == SP_NSORT_LINK_FAILURE)
    {
        fprintf(stderr, "Warning: Nsort library cannot be linked to.  "
                "Skipping one-shot sort tests.\n");
    }
    else
    {
        if (ret != SP_SORT_DEF_ERROR)
        {
            fprintf(stderr, "return status from sp_start_sort() with a"
                    " licensing error is not SP_SORT_DEF_ERROR: %d\n", ret);
            return (1);
        }
        error_str = sp_get_error_string(sp, ret);
        if (strstr(error_str, "New license info for") == NULL)
        {
            fprintf(stderr, "licensing failure error string does not include "
                    "new license info:\n%s\n", error_str);
            return (1);
        }
    
        /* force record max size exceeded error that will occur either on the 
         * EOF or on the first sp_read_output() call.
         */
        def = "-format:max=10 -memory:20m";
        ret = sp_start_sort(&sp, def);
        longlinelen = strlen(longline);
        if (ret != SP_OK)
        {
            fprintf(stderr, "return status from sp_start_sort(&sp, \"%s\") "
                    "is not SP_OK: %d\n", def, ret);
            return (1);
        }
        ret = sp_write_input(sp, longline, longlinelen);
        if (ret != longlinelen)
        {
            fprintf(stderr, "return status from sp_write_input() "
                    "is not longlinelen(%d): %d\n", longlinelen, ret);
            return (1);
        }
        ret = sp_write_input(sp, NULL, 0);
        if (ret != 0)
        {
            if (ret > 0)
            {
                fprintf(stderr, "return status from sp_write_input(0) "
                        "is not 0: %d\n", ret);
                return (1);
            }
        }
        else
        {
            ret = sp_read_output(sp, 0, buf, sizeof(buf));
            if (ret >= 0)
            {
                fprintf(stderr, "return status from sp_read_output() "
                        "is non-negative %d\n", ret);
                return (1);
            }
        }
        error_str = sp_get_error_string(sp, sp_get_error(sp));
        if (strstr(error_str, "DELIM_MISSING") == NULL)
        {
            fprintf(stderr, "Too-long line failure error string does not "
                    "include \"DELIM_MISSING\":\n%s\n", error_str);
            return (1);
        }
        
        /* force licensing failure for hitting free licensing capacity limit.
         * XXX
         */
    }

    return (0);
}
