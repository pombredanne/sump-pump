/* spgzip.c - SUMP Pump(TM) compression performance example.
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
 * Usage: spgzip < uncompressed_input_file > compressed_output_file
 *
 *        The program relies on the fact that gzip-compressed files can
 *        be concatenated.  The sump pump infrastructure breaks the
 *        uncompressed input into 512KB blocks.  Each block is
 *        compressed separately by a pump function using standard zlib
 *        functions.  The compresed outputs of the pump functions are
 *        concatenated by the sump pump infrastructure and written to
 *        the output file.
 */
#include "sump.h"
#include "zlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* gzip_pump - This pump function will be executed in parallel by
 *             however many threads are in the sump pump.  The function
 *             simply gets the entire input buffer and pointer to the
 *             output buffer and passes them to zlib routines
 *             deflateInit2() and deflate().
 */
int gzip_pump(sp_task_t t, void *unused)
{
    z_stream    strm;
    char        *in;
    size_t      in_size;
    char        *out;
    size_t      out_size;
    int         ret;

    /* get input buffer */
    if ((ret = pfunc_get_in_buf(t, (void **)&in, &in_size)) != 0)
        return (pfunc_error(t, "gzip_pump: "
                            "bad ret from sp_get_in_buf: %d\n", ret));
    /* get output buffer */
    if ((ret = pfunc_get_out_buf(t, 0, (void **)&out, &out_size)) != 0)
        return (pfunc_error(t, "gzip_pump: "
                            "bad ret from sp_get_out_buf: %d\n", ret));
    /* setup strm structure for zlib's deflate */
    bzero(&strm, sizeof(strm));
    strm.next_in = in;
    strm.avail_in = in_size;
    strm.next_out = out;
    strm.avail_out = out_size;
    ret = deflateInit2(&strm,
                       Z_DEFAULT_COMPRESSION, /* level */
                       Z_DEFLATED,            /* method */
                       16+15, /* generate gzip header and use 2**15 window */
                       8,                     /* memLevel */
                       Z_DEFAULT_STRATEGY);   /* strategy */
    if (ret != Z_OK)
        return (pfunc_error(t, "gzip_pump: " 
                            "deflateInit2 returns: %d\n", ret));
    if ((ret = deflate(&strm, Z_FINISH)) != Z_STREAM_END)
        return (pfunc_error(t, "gzip_pump: " 
                            "deflate returns: %d\n", ret));
    if ((ret = deflateEnd(&strm)) != Z_OK)
        return (pfunc_error(t, "gzip_pump: " 
                            "deflateEnd returns: %d\n", ret));
    /* commit output bytes */
    if ((ret = pfunc_put_out_buf_bytes(t, 0, strm.total_out)) != 0)
        return (pfunc_error(t, "gzip_pump: " 
                            "bad ret from sp_put_out_buf_bytes: %d\n", ret));
    return (SP_OK);
}
                 

int main(int argc, char *argv[])
{
    sp_t                sp;
    size_t              in_buf_size = (1 << 19);        /* 512K */
    size_t              out_buf_size;
    int                 ret;

    /* max expansion is 0.03% + gzip header size (+1 more for rounding up) */
    out_buf_size = (size_t)((double)in_buf_size * 1.0003) + 512;
    ret = sp_start(&sp, gzip_pump,
                   "WHOLE_BUF "
                   "IN_FILE=<stdin> OUT_FILE[0]=<stdout> "             
                   "IN_BUF_SIZE=%d OUT_BUF_SIZE[0]=%d %s",
                   in_buf_size, out_buf_size,
                   sp_argv_to_str(argv + 1, argc - 1));
    if (ret != SP_OK)
    {
        fprintf(stderr, "sp_start failed: %s\n", sp_get_error_string(sp, ret));
        return (1);
    }
    /* wait for sump pump to complete */
    if ((ret = sp_wait(sp)) != SP_OK)
    {
        fprintf(stderr, "sp_wait: %s\n", sp_get_error_string(sp, ret));
        return (1);
    }
    return (0);
}
