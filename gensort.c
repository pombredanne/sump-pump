/* gensort.c - SUMP Pump(TM) demo version of the data generator program 
 *             for sort benchmarks.
 *             SUMP Pump is a trademark of Ordinal Technology Corp.
 *
 * $Revision$
 *
 * Copyright (C) 2010, Ordinal Technology Corp, http://www.ordinal.com
 *
 *   Note: This version of the gensort program has been functionally
 *         simplified to better illustrate the use of the sump pump
 *         infrastructure.  This version is hardwired to produce the same
 *         result as running the full-functionality version of gensort as:
 *             gensort -a -c 10000000 sortinput.txt
 *         For the normal, full-function version of gensort see:
 *             http://www.ordinal.com/gensort.html
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rand16.h"
#include <zlib.h>   /* use crc32() function in zlib */
#include "sump.h"
#include <fcntl.h>

/* Number of records to be generated per output block */
#define BLK_RECS        100000

/* "instruction" structure that is passed to sump pump threads */
struct gen_instruct
{
    u16         starting_rec;   /* starting record number */
    u8          num_recs;       /* the number of records to generate */
};

#define REC_SIZE 100
#define HEX_DIGIT(x) ((x) >= 10 ? 'A' + (x) - 10 : '0' + (x))

u16         Sum16;              /* record checksum */


/* gen_ascii_rec = generate an ascii record suitable for all sort
 *              benchmarks including PennySort.
 */
void gen_ascii_rec(unsigned char *rec_buf, u16 rand, u16 rec_number)
{
    int         i;
    u8          temp;
    
    /* generate the 10-byte ascii key using mostly the high 64 bits.
     */
    temp = rand.hi8;
    rec_buf[0] = (unsigned char)(' ' + (temp % 95));
    temp /= 95;
    rec_buf[1] = (unsigned char)(' ' + (temp % 95));
    temp /= 95;
    rec_buf[2] = (unsigned char)(' ' + (temp % 95));
    temp /= 95;
    rec_buf[3] = (unsigned char)(' ' + (temp % 95));
    temp /= 95;
    rec_buf[4] = (unsigned char)(' ' + (temp % 95));
    temp /= 95;
    rec_buf[5] = (unsigned char)(' ' + (temp % 95));
    temp /= 95;
    rec_buf[6] = (unsigned char)(' ' + (temp % 95));
    temp /= 95;
    rec_buf[7] = (unsigned char)(' ' + (temp % 95));
    temp = rand.lo8;
    rec_buf[8] = (unsigned char)(' ' + (temp % 95));
    temp /= 95;
    rec_buf[9] = (unsigned char)(' ' + (temp % 95));
    temp /= 95;

    /* add 2 bytes of "break" */
    rec_buf[10] = ' ';
    rec_buf[11] = ' ';
    
    /* convert the 128-bit record number to 32 bits of ascii hexadecimal
     * as the next 32 bytes of the record.
     */
    for (i = 0; i < 16; i++)
        rec_buf[12 + i] =
            (unsigned char)(HEX_DIGIT((rec_number.hi8 >> (60 - 4 * i)) & 0xF));
    for (i = 0; i < 16; i++)
        rec_buf[28 + i] =
            (unsigned char)(HEX_DIGIT((rec_number.lo8 >> (60 - 4 * i)) & 0xF));

    /* add 2 bytes of "break" data */
    rec_buf[44] = ' ';
    rec_buf[45] = ' ';

    /* add 52 bytes of filler based on low 48 bits of random number */
    rec_buf[46] = rec_buf[47] = rec_buf[48] = rec_buf[49] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 48) & 0xF));
    rec_buf[50] = rec_buf[51] = rec_buf[52] = rec_buf[53] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 44) & 0xF));
    rec_buf[54] = rec_buf[55] = rec_buf[56] = rec_buf[57] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 40) & 0xF));
    rec_buf[58] = rec_buf[59] = rec_buf[60] = rec_buf[61] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 36) & 0xF));
    rec_buf[62] = rec_buf[63] = rec_buf[64] = rec_buf[65] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 32) & 0xF));
    rec_buf[66] = rec_buf[67] = rec_buf[68] = rec_buf[69] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 28) & 0xF));
    rec_buf[70] = rec_buf[71] = rec_buf[72] = rec_buf[73] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 24) & 0xF));
    rec_buf[74] = rec_buf[75] = rec_buf[76] = rec_buf[77] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 20) & 0xF));
    rec_buf[78] = rec_buf[79] = rec_buf[80] = rec_buf[81] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 16) & 0xF));
    rec_buf[82] = rec_buf[83] = rec_buf[84] = rec_buf[85] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >> 12) & 0xF));
    rec_buf[86] = rec_buf[87] = rec_buf[88] = rec_buf[89] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >>  8) & 0xF));
    rec_buf[90] = rec_buf[91] = rec_buf[92] = rec_buf[93] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >>  4) & 0xF));
    rec_buf[94] = rec_buf[95] = rec_buf[96] = rec_buf[97] = 
        (unsigned char)(HEX_DIGIT((rand.lo8 >>  0) & 0xF));

    /* add 2 bytes of "break" data */
    rec_buf[98] = '\r'; /* nice for Windows */
    rec_buf[99] = '\n';
}


/* gen_block -  pump function that reads a task instruction from the
 *              process's main thread.  The instruction consists of the
 *              beginning record number and the number of records to be
 *              generated.  The records are written to the task output.
 *              The sump pump infrastructure will run multiple instances
 *              of this function in parallel, and concatenate their
 *              outputs to form the output of the sump pump.
 */
int gen_block(sp_task_t t, void *unused)
{
    struct gen_instruct *ip;
    struct gen_instruct instruct;
    u8                  j;
    u16                 rand;
    u16                 temp16 = {0LL, 0LL};
    u16                 sum16 = {0LL, 0LL};
    u16                 rec_number;
    unsigned char       rec_buf[100];

    /* read instruction from the thread's input */
    if (pfunc_get_rec(t, &ip) != sizeof(instruct))
        return (pfunc_error(t, "sp_get_rec() error"));
    instruct = *ip;

    rec_number = instruct.starting_rec;
    rand = skip_ahead_rand(rec_number);

    /* generate records and write them to the task output */
    for (j = 0; j < instruct.num_recs; j++)
    {
        rand = next_rand(rand);
        gen_ascii_rec(rec_buf, rand, rec_number);
        temp16.lo8 = crc32(0, rec_buf, REC_SIZE);
        sum16 = add16(sum16, temp16);
        pfunc_write(t, 0, rec_buf, REC_SIZE);
        if (++rec_number.lo8 == 0)
            ++rec_number.hi8;
    }

    /* add the checksum for the block of records just generated to
     * the global checksum.
     *
     * this could be done without a mutex by outputing the checksum
     * to a second output stream that is read by the main thread,
     * but this way is much simpler.
     */
    pfunc_mutex_lock(t);
    Sum16 = add16(Sum16, sum16);  
    pfunc_mutex_unlock(t);
    
    return (SP_OK);
}


void usage(void)
{
    fprintf(stderr,
            "gensort sump pump demo program\n"
            "usage: gensort number_of_records file_name [sump pump args]\n");
    exit(1);
}


int main(int argc, char *argv[])
{
    u8                  j;                      /* should be a u16 someday */
    u16                 starting_rec_number;
    u16                 num_recs;
    u8                  blk_recs;
    struct gen_instruct instruct;
    int                 ret;            /* return value */
    sp_t                sp_gen;         /* handle for sump pump */

    if (argc < 3)
        usage();

    starting_rec_number.hi8 = 0;
    starting_rec_number.lo8 = 0;
    num_recs = dec_to_u16(argv[1]);
    
    /* start a sump pump to generate records using the gen_block()
     * function.  Making the input buffer size the size of an
     * instruction structure insures that each sump pump task executes
     * exactly one instruction.  The output buffer size will hold the
     * maximum number of records that will be generated per instruction.
     */
    ret = sp_start(&sp_gen, gen_block,
                   "IN_BUF_SIZE=%d REC_SIZE=%d OUT_BUF_SIZE[0]=%d "
                   "OUT_FILE[0]=%s %s",
                   sizeof(struct gen_instruct),  /* input buf size */
                   sizeof(struct gen_instruct),  /* input record size */
                   BLK_RECS * REC_SIZE,          /* output buf size */
                   argv[2],                      /* file name */
                   sp_argv_to_str(argv + 3, argc - 3)); /* sump pump args */
    if (ret)
    {
        fprintf(stderr, "sp_start failed: %s\n",
                sp_get_error_string(sp_gen, ret));
        return (ret);
    }

    /* Feed generate instruction structures to the sump pump threads.
     * Each instruction struct will handled by a sump pump thread
     * executing the gen_block() pump function.
     */
    instruct.starting_rec = starting_rec_number;
    for (j = 0; (j * BLK_RECS) < num_recs.lo8; j++)
    {
        /* set starting rec and number of recs to generate */
        /* instruct.starting_rec.lo8 = (j * BLK_RECS); */
        blk_recs = num_recs.lo8 - (j * BLK_RECS);
        if (blk_recs > BLK_RECS)
            blk_recs = BLK_RECS;
        instruct.num_recs = blk_recs;
        if (sp_write_input(sp_gen, &instruct, sizeof(instruct)) != sizeof(instruct))
            fprintf(stderr, "sp_write_input: %s\n",
                    sp_get_error_string(sp_gen, SP_WRITE_ERROR)), exit(1); 
        instruct.starting_rec.lo8 += BLK_RECS;
    }
    /* write EOF to sump pump so it will wind down */
    if (sp_write_input(sp_gen, NULL, 0) != 0)
        fprintf(stderr, "sp_write_input(0): %s\n",
                sp_get_error_string(sp_gen, SP_WRITE_ERROR)), exit(1); 

    /* wait for sump pump to finish */
    if ((ret = sp_wait(sp_gen)) != SP_OK)
        fprintf(stderr, "sp_wait: %s\n",
                sp_get_error_string(sp_gen, ret)), exit(1); 

    return (0);
}
