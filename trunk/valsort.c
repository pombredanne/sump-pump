/* valsort.c - SUMP Pump(TM) demo version of the output validator program 
 *             for sort benchmarks.
 *             SUMP Pump is a trademark of Ordinal Technology Corp.
 *
 * $Revision$
 *
 * Copyright (C) 2010, Ordinal Technology Corp, http://www.ordinal.com
 *
 *   Note: This version of the valsort program has been functionally
 *         simplified to better illustrate the use of the sump pump
 *         infrastructure.  
 *         For the normal, full-function version of valsort see:
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
#include <zlib.h>   /* use crc32() function */
#include "rand16.h"
#define NO_EXTRA_THREADS  (-1)

# include <fcntl.h>
# include "sump.h"
# define BLK_RECS       10240

#define REC_SIZE 100
#define SUM_SIZE (sizeof(struct summary))

/* Comparison routine, either memcmp() or strcasecmp() */
int     (*Compare)(const unsigned char *a, const unsigned char *b, size_t n) =
    (int (*)(const unsigned char *a, const unsigned char *b, size_t n))memcmp;

/* struct used to summarize a partition of sort output
 */
struct summary
{
    u16             first_unordered;     /* index of first unordered record,
                                          * or 0 if no unordered records */
    u16             unordered_count;     /* total number of unordered records*/
    u16             rec_count;           /* total number of records */
    u16             dup_count;           /* total number of duplicate keys */
    u16             checksum;            /* checksum of all records */
    unsigned char   first_rec[REC_SIZE]; /* first record */
    unsigned char   last_rec[REC_SIZE];  /* last record */
};

struct summary Summary;


/* next_rec - get the next record to be validated
 *
 */
unsigned char *next_rec(void *in, struct summary *sum)
{
    int                 read_size;
    unsigned char       *rec = NULL;
    u16                 temp16 = {0LL, 0LL};

    /* get the record from the sump pump infrastructure */
    read_size = (int)pfunc_get_rec(in, &rec);
    
    if (read_size == REC_SIZE)
    {
        temp16.lo8 = crc32(0, rec, REC_SIZE);
        sum->checksum = add16(sum->checksum, temp16);
    }
    else if (read_size == 0)
        return (NULL);
    else if (read_size < 0)
    {
        fprintf(stderr, "record read error\n");
        exit(1);
    }
    else
    {
        fprintf(stderr, "partial record found at end\n");
        exit(1);
    }
    return (rec);
}


/* summarize_records - pump function that reads a block of input records
 *                     and produces a struct summary to summarize the
 *                     input records.
 */
int summarize_records(sp_task_t t, void *unused)
{
    struct summary      *sum;
    int                 diff;
    u16                 one = {0LL, 1LL};
    unsigned char       *rec;
    unsigned char       prev[REC_SIZE];
    struct summary      local_summary;

    sum = &local_summary;
    memset(sum, 0, sizeof(struct summary));

    if ((rec = next_rec(t, sum)) == NULL)
    {
        fprintf(stderr, "there must be at least one record to be validated\n");
        exit(1);
    }
    memcpy(sum->first_rec, rec, REC_SIZE);
    memcpy(prev, rec, REC_SIZE);
    sum->rec_count = add16(sum->rec_count, one);
    
    while ((rec = next_rec(t, sum)) != NULL)
    {
        /* make sure the record key is equal to or more than the
         * previous key
         */
        diff = (*Compare)(prev, rec, 10);
        if (diff == 0)
            sum->dup_count = add16(sum->dup_count, one);
        else if (diff > 0)
        {
            if (sum->first_unordered.hi8 == 0 &&
                sum->first_unordered.lo8 == 0)
            {
                sum->first_unordered = sum->rec_count;
            }
            sum->unordered_count = add16(sum->unordered_count, one);
        }

        sum->rec_count = add16(sum->rec_count, one);
        memcpy(prev, rec, REC_SIZE);
    }
    memcpy(sum->last_rec, prev, REC_SIZE);  /* set last record for summary */

    pfunc_write(t, 0, sum, SUM_SIZE);
    return (SP_OK);
}


/* next_sum - get the next partition summary
 */
int next_sum(void *in, struct summary *sum)
{
    int                 ret;

    ret = (int)sp_read_output(in, 0, sum, SUM_SIZE);  /* get from sump pump */
    
    if (ret == 0)
        return (0);
    else if (ret < 0)
    {
        fprintf(stderr,
                "summary read error: %s\n", sp_get_error_string(in, 0));
        exit(1);
    }
    else if (ret != SUM_SIZE)
    {
        fprintf(stderr, "partial partition summary found at end\n");
        exit(1);
    }
    return (ret);
}


/* sum_summaries - validate a sequence of partition summaries
 */
void sum_summaries(void *in)
{
    int                 diff;
    u16                 one = {0LL, 1LL};
    unsigned char       prev[REC_SIZE];
    char                sumbuf[U16_ASCII_BUF_SIZE];
    struct summary      local_sum;

    if (next_sum(in, &local_sum) == 0)
    {
        fprintf(stderr, "there must be at least one record to be validated\n");
        exit(1);
    }
    memcpy(&Summary, &local_sum, SUM_SIZE);
    memcpy(prev, Summary.last_rec, REC_SIZE);
    
    while (next_sum(in, &local_sum))
    {
        /* make sure the record key is equal to or more than the
         * previous key
         */
        diff = (*Compare)(prev, local_sum.first_rec, 10);
        if (diff == 0)
            Summary.dup_count = add16(Summary.dup_count, one);
        else if (diff > 0)
        {
            if (Summary.first_unordered.hi8 == 0 &&
                Summary.first_unordered.lo8 == 0)
            {
                fprintf(stderr, "First unordered record is record %s\n",
                        u16_to_dec(Summary.rec_count, sumbuf));
                Summary.first_unordered = Summary.rec_count;
            }
            Summary.unordered_count = add16(Summary.unordered_count, one);
        }

        if ((Summary.first_unordered.hi8 == 0 &&
             Summary.first_unordered.lo8 == 0) &&
            !(local_sum.first_unordered.hi8 == 0 &&
              local_sum.first_unordered.lo8 == 0))
        {
            Summary.first_unordered =
                add16(Summary.rec_count, local_sum.first_unordered);
            fprintf(stderr, "First unordered record is record %s\n",
                    u16_to_dec(Summary.first_unordered, sumbuf));
        }

        Summary.unordered_count =
            add16(Summary.unordered_count, local_sum.unordered_count);
        Summary.rec_count = add16(Summary.rec_count, local_sum.rec_count);
        Summary.dup_count = add16(Summary.dup_count, local_sum.dup_count);
        Summary.checksum = add16(Summary.checksum, local_sum.checksum);
        memcpy(prev, local_sum.last_rec, REC_SIZE);
    }
    memcpy(Summary.last_rec, prev, REC_SIZE); /* get last rec of last summary */
}


void usage(void)
{
    fprintf(stderr,
            "valsort sump pump demo program\n"
            "usage: valsort sorted_output_file_name [sump pump args]\n");
    exit(1);
}


int main(int argc, char *argv[])
{
    char                sumbuf[U16_ASCII_BUF_SIZE];
    sp_t                sp_val;
    int                 ret;

    if (argc < 2)
        usage();
    
    /* start a sump pump to validate the correct order of records.  the
     * sump pump infrastructure will break the sort output file into
     * blocks of BLK_RECS records that will be summarized by separate
     * threads.  this thread will then validate the summaries.
     */
    ret = sp_start(&sp_val, summarize_records,
                   "-IN_FILE=%s -IN_BUF_SIZE=%d "
                   "-REC_SIZE=%d -OUT_BUF_SIZE[0]=%d %s",
                   argv[1],
                   4 * 1024 * 1024,              /* input buffer size */
                   REC_SIZE,                     /* input record size */
                   sizeof(struct summary),       /* output buf size */
                   sp_argv_to_str(argv + 2, argc - 2));
    if (ret)
    {
        fprintf(stderr, "valsort: sp_start failed: %s\n",
                sp_get_error_string(sp_val, ret));
        return (ret);
    }

    /* sum the summaries that are the output of the sump pump */
    sum_summaries(sp_val);

    fprintf(stdout, "Records: %s\n",
            u16_to_dec(Summary.rec_count, sumbuf));
    fprintf(stdout, "Checksum: %s\n",
            u16_to_hex(Summary.checksum, sumbuf));
    if (Summary.unordered_count.hi8 | Summary.unordered_count.lo8)
    {
        fprintf(stdout, "ERROR - there are %s unordered records\n",
                u16_to_dec(Summary.unordered_count, sumbuf));
    }
    else
    {
        fprintf(stdout, "Duplicate keys: %s\n",
                u16_to_dec(Summary.dup_count, sumbuf));
        fprintf(stdout, "SUCCESS - all records are in order\n");
    }
    
    /* return non-zero if there are any unordered records */
    return (Summary.unordered_count.hi8 | Summary.unordered_count.lo8) ? 1 : 0;
}
