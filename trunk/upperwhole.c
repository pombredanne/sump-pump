/* upperwhole.c - SUMP Pump(TM) regression test program that allows a
 *                variety of sump pump parameters to be specified when
 *                using a pump function on whole input buffers.  Used
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
 * Usage: upperwhole [sump pump directives]
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


/* uppercase - a pump function to operate on a whole input buffer.
 */
int uppercase(sp_task_t t, void *unused)
{
    unsigned char       *in;
    size_t              in_size;
    unsigned char       *p_in;
    unsigned char       *out;
    size_t              out_size;
    unsigned char       *p_out;
    int                 status;
        
    if ((status = pfunc_get_in_buf(t, (void **)&in, &in_size)) != 0)
        return (pfunc_error(t,
                            "uppercase, bad status from sp_get_in_buf: %d\n",
                            status));
    if ((status = pfunc_get_out_buf(t, 0, (void **)&out, &out_size)) != 0)
        return (pfunc_error(t,
                            "uppercase, bad status from sp_get_out_buf: %d\n",
                            status));
    p_out = out;
    for (p_in = in; (size_t)(p_in - in) < in_size; p_in++)
    {
        if (p_out - out == out_size)
        {
            if ((status = pfunc_put_out_buf_bytes(t, 0, p_out - out)) != 0)
                return (pfunc_error(t,
                                    "uppercase, bad status from sp_put_out_buf_bytes: %d\n",
                                    status));
            if ((status = pfunc_get_out_buf(t, 0, (void **)&out, &out_size)) != 0)
                return (pfunc_error(t,
                                    "uppercase, bad status from sp_get_out_buf: %d\n",
                                    status));
            p_out = out;
        }
        *p_out++ = toupper(*p_in);
    }
    if ((p_out - out) > 0 &&
        (status = pfunc_put_out_buf_bytes(t, 0, p_out - out)) != 0)
    {
        return (pfunc_error(t,
                            "uppercase, bad status from sp_put_out_buf_bytes: %d\n",
                            status));
    }
    return (SP_OK);
}

int main(int argc, char *argv[])
{
    sp_t                sp;
    int                 ret;

    ret = sp_start(&sp, uppercase, 
                   "-WHOLE_BUF -IN_FILE=rin1.txt -OUT_FILE[0]=rout.txt %s",
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

