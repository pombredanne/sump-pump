/* billing.c - SUMP Pump(TM) example program that uses mapreduce-like
 *             functionality to produce a phone bill from an input
 *             file containing records of individual phone calls.
 *             SUMP Pump is a trademark of Ordinal Technology Corp.
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
 * Linking SUMP Pump statically or dynamically with other modules is
 * making a combined work based on SUMP Pump.  Thus, the terms and
 * conditions of the GNU General Public License v.2 cover the whole
 * combination.
 *
 * In addition, as a special exception, the copyright holders of SUMP Pump
 * give you permission to combine SUMP Pump program with free software
 * programs or libraries that are released under the GNU LGPL and with
 * independent modules that communicate with SUMP Pump solely through
 * Ordinal Technology Corp's Nsort Subroutine Library interface as defined
 * in the Nsort User Guide, http://www.ordinal.com/NsortUserGuide.pdf.
 * You may copy and distribute such a system following the terms of the
 * GNU GPL for SUMP Pump and the licenses of the other code concerned,
 * provided that you include the source code of that other code when and
 * as the GNU GPL requires distribution of source code.
 *
 * Note that people who make modified versions of SUMP Pump are not
 * obligated to grant this special exception for their modified
 * versions; it is their choice whether to do so.  The GNU General
 * Public License gives permission to release a modified version without
 * this exception; this exception also makes it possible to release a
 * modified version which carries forward this exception.
 *
 *
 * Usage: billing input_file_name
 *
 *       This Sump Pump performance example program is hardwired to
 *       read a file of phone call records, billing_input.txt, and
 *       write a series of phone bills to billing_output.txt.
 *
 *       The file billing_input.txt contains 10,001,830 phone call
 *       records with fields for the originating phone number,
 *       destination phone number, date/time, minutes of call
 *       duration, and an unnecessary field:
 *           ...
 *           6502075186,1738108607,2010:01:15:04:44:00,8,10000274
 *           6502879128,2760604299,2010:01:19:21:55:00,23,10000275
 *           6502612994,2977419483,2010:01:08:18:26:00,28,1000032
 *           6502245279,8483380882,2010:01:01:17:24:00,9,10000328
 *           ...
 *       These records are sent through a "map" pump function,
 *       map_pump(), that performs the trivial, 1-to-1 transformation
 *       of simply dropping the unnecessary field.  Note that more
 *       complex transformations (1-to-many, 1-to-none) are possible
 *       here.  The records then look like:
 *           ...
 *           6502075186,1738108607,2010:01:15:04:44:00,8
 *           6502879128,2760604299,2010:01:19:21:55:00,23
 *           6502612994,2977419483,2010:01:08:18:26:00,28
 *           6502245279,8483380882,2010:01:01:17:24:00,9
 *           ...
 *       The records are sorted by the Nsort subroutine library using
 *       the originating phone number as the first key, and the
 *       date/time as the second key.  The sort is also done with the
 *       SP_KEY_DIFF flag which causes the sort to produce a
 *       one-character record prefix that indicates how many key values
 *       in the record match the previous record.  The sort output looks
 *       like:
 *           ...
 *           16502000001,6122014655,2010:01:16:12:18:00,11
 *           06502000002,6045401969,2010:01:01:00:44:00,11
 *           16502000002,3527948477,2010:01:02:05:57:00,11
 *           16502000002,4232810775,2010:01:03:22:38:00,19
 *           16502000002,4492155117,2010:01:04:09:39:00,1
 *           16502000002,4009615215,2010:01:05:05:38:00,12
 *           06502000003,4729657570,2010:01:01:09:42:00,13
 *           ...
 *       This sorted output feeds to a "reduce" sump pump - one that
 *       uses a REDUCE_BY_KEYS directive to insure that all records with
 *       equal group-by keys (just the originating phone number in this
 *       case) end up in the same record group. The sump pump
 *       infrastructure uses the one-character record prefix to group
 *       the records, and also consumes the prefix so that it is not
 *       visible by the reduce pump functions.  The reduce pump function,
 *       reduce_pump(), reads all the records for each orginating
 *       phone and produces a "phone bill" for that originating
 *       number. For example, the bill for the records for 6502000002
 *       shown above would be:
 *           ...
 *           (650)200-0002
 *             (604)540-1969  Jan  1 00:44   11
 *             (352)794-8477  Jan  2 05:57   11
 *             (423)281-0775  Jan  3 22:38   19
 *             (449)215-5117  Jan  4 09:39    1
 *             (400)961-5215  Jan  5 05:38   12
 *           Total minutes                   54
 *           Average minutes                 10
 *           Minimum                          1
 *           Maximum                         19
 *           ...
 */

#include "sump.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#if defined(win_nt)
# include <string.h>
#else
# include <unistd.h>
# include <strings.h>
#endif


/* map_pump - map pump function that performs a simple 1-1 mapping to
 *            drop the last comma-separated field in the record.
 */
int map_pump(sp_task_t t, void *unused)
{
    char        *rec;
    char        *last_comma;
        
    /* for each record in the group */
    while (pfunc_get_rec(t, &rec) > 0)
    {
#if defined(win_nt)
        last_comma = strrchr(rec, ',');
#else
        last_comma = rindex(rec, ',');
#endif
        /* ignore malformed records.  we could alternatively write
         * malformed records to a separate output file.
         */
        if (last_comma == NULL) 
            continue;
        /* write the record chars up to last comma.  i.e. strip last field */
        if (pfunc_printf(t, 0, "%.*s\n", last_comma - rec, rec) == 0)
            return (SP_WRITE_ERROR);
    }
    return (SP_OK);
}


/* format_phone_number - support routine to convert in-place a phone number
 *                       to a format appropriate for the United States.
 */
char *format_phone_number(char *s)
{
    /* convert in place "9876543210" to "(987)654-3210"
     */
    s[13] = '\0';
    s[12] = s[9];
    s[11] = s[8];
    s[10] = s[7];
    s[9] = s[6];
    s[8] = '-';
    s[7] = s[5];
    s[6] = s[4];
    s[5] = s[3];
    s[4] = ')';
    s[3] = s[2];
    s[2] = s[1];
    s[1] = s[0];
    s[0] = '(';
    return (s);
}


/* format_datetime - support routine format in-place a date/time.
 */
char *format_datetime(char *s)
{
    /* Convert in place "2010:01:05:21:41:00" to "Jan  5 21:41".
     * NOTE: This format function is really lazy and limited.
     * It simply assumes the month is Jan.
     */
    s[0] = 'J';
    s[1] = 'a';
    s[2] = 'n';
    s[3] = ' ';
    s[4] = s[8] == '0' ? ' ' : s[8];
    s[5] = s[9];
    s[6] = ' ';
    s[7] = s[11];
    s[8] = s[12];
    s[9] = s[13];
    s[10] = s[14];
    s[11] = s[15];
    s[12] = '\0';
    return (s);
}


/* reduce_pump - reduce pump function that reads phone records for an
 *               originating phone number and produces a phone bill.
 */
int reduce_pump(sp_task_t t, void *unused)
{
    char        *rec;
    char        account[100], called[100], datetime[100];
    int         minutes;
    int         total_minutes = 0;
    int         call_count = 0;
    int         minimum = 0x7FFFFFFF;
    int         maximum = 0;
        
    pfunc_get_rec(t, &rec);     /* get first record in key group */
    sscanf(rec, "%[^,]", account);
    /* print originating phone number */
    pfunc_printf(t, 0, "%s\n", format_phone_number(account));
    do
    {
        sscanf(rec, "%[^,],%[^,],%[^,],%d",
               account, called, datetime, &minutes);
        /* print information for a phone call from the originating number */
        pfunc_printf(t, 0, "  %s  %s   %2d\n",
                     format_phone_number(called),
                     format_datetime(datetime),
                     minutes);
        call_count++;
        total_minutes += minutes;
        if (minutes < minimum)
            minimum = minutes;    /* new minimum */
        if (minutes > maximum)
            maximum = minutes;    /* new maximum */
    } while (pfunc_get_rec(t, &rec) > 0);
    pfunc_printf(t, 0, "Total minutes                 %4d\n", total_minutes);
    pfunc_printf(t, 0, "Average minutes               %4d\n",
                 total_minutes / call_count);
    pfunc_printf(t, 0, "Minimum                       %4d\n", minimum);
    pfunc_printf(t, 0, "Maximum                       %4d\n\n", maximum);
    return (SP_OK);
}


void usage(void)
{
    fprintf(stderr, "usage: billing input_file_name [sump pump args]\n");
    exit(1);
}


int main(int argc, char *argv[])
{
    sp_t                sp_map;
    sp_t                sp_sort;
    sp_t                sp_reduce;
    const char          *sp_arg;
    int                 ret;
    int                 print_stats = 0;
    int                 n_threads;
# if defined(win_nt)
    SYSTEM_INFO         si;

    GetSystemInfo(&si);
    n_threads = si.dwNumberOfProcessors;
# else
    n_threads = sysconf(_SC_NPROCESSORS_ONLN);
# endif

    /* command line is:
     * billing [-stat] input_file output_file [sump_pump_args]
     */
    if (argc >= 2 && !strcmp(argv[1], "-stat"))
    {
        print_stats = 1;
        argv++;
        argc--;
    }
    if (argc < 2)
        usage();
    sp_arg = sp_argv_to_str(argv + 3, argc - 3);

    /* start map sump pump */
    ret = sp_start(&sp_map, map_pump,
                   "ASCII IN_FILE=%s %s", argv[1], sp_arg);
    if (ret != SP_OK)
    {
        fprintf(stderr, "sp_start for map failed: %s\n",
                sp_get_error_string(sp_map, ret));
        return(1);
    }

    /* start sort */
    ret = sp_start_sort(&sp_sort,
                        /* input is lines of text, comma separated fields */
                        "-match=1 "    /* match records only by first key */
                        "-format:delim=nl,separator=comma "
                        "-key=char,position=1 " /* account is primary key */
                        "-key=char,position=3 " /* datetime is secondary key */
                        "-memory=700m "   /* use 700mb of memory */
                        "-process=%d ", n_threads);   /* sort thread count */
    if (ret != SP_OK)
    {
        fprintf(stderr, "sp_start_sort failed: %s\n",
                sp_get_error_string(sp_sort, ret));
        return(1);
    }

    /* start reduce sump pump */
    ret = sp_start(&sp_reduce, reduce_pump, 
                   "ASCII GROUP_BY "
                   "OUT_FILE[0]=%s "
                   "IN_BUF_SIZE=500000 "
                   "OUT_BUF_SIZE[0]=700000 "  /* allow for output expansion */
                   "%s ",
                   argv[2], sp_arg);
    if (ret != SP_OK)
    {
        fprintf(stderr, "sp_start for reduce failed: %s\n",
                sp_get_error_string(sp_reduce, ret));
        return(1);
    }

    /* link map sump pump to sort, and sort to reduce sump pump */
    if ((ret = sp_link(sp_map, 0, sp_sort)) != SP_OK)
        fprintf(stderr, "sp_link(sp_map, 0, sp_sort): %s\n",
                sp_get_error_string(NULL, ret)), exit(1); 
    if ((ret = sp_link(sp_sort, 0, sp_reduce)) != SP_OK)
        fprintf(stderr, "sp_link(sp_sort, 0, sp_reduce): %s\n",
                sp_get_error_string(NULL, ret)), exit(1); 

    /* wait for sump pumps from upstream to downstream */
    if ((ret = sp_wait(sp_map)) != SP_OK)
        fprintf(stderr, "sp_map: %s\n",
                sp_get_error_string(sp_map, ret)), exit(1); 
    if ((ret = sp_wait(sp_sort)) != SP_OK)
        fprintf(stderr, "sp_sort: %s\n",
                sp_get_error_string(sp_sort, ret)), exit(1); 
    else if (print_stats)
        fprintf(stderr, "sort stats:\n%s\n", sp_get_sort_stats(sp_sort));
    if ((ret = sp_wait(sp_reduce)) != SP_OK)
        fprintf(stderr, "sp_reduce: %s\n",
                sp_get_error_string(sp_reduce, ret)), exit(1); 

    return (0);
}
