/* lookup.c - SUMP Pump(TM) example program to lookup a field value.
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
 * Usage: lookup
 *
 *        This sump pump performance example program is hardwired to
 *        read the reference file "lookupref.txt", build an internal
 *        hash table with the values in that file, then read the input
 *        file "lookupin.txt" into a sump pump that looks for a matching
 *        key in the hash table.  If a matching key is found in the hash
 *        table, the non-key fields of the reference record are appended
 *        to the input record and sent the output file "match.txt".  If
 *        no match is found, the input record is sent to "nomatch.txt".
 *        Both the input and reference files contain comma-separated
 *        fields with the key being the first field.
 *
 * Example:
 *       The reference file, lookupref.txt, contains:
 *           apple,sauce
 *           mango,pulp
 *       The input file, lookupin.txt, contains:
 *           lemon,012,345,678
 *           mango,987,654,321
 *           apple,555,555,555
 *           berry,222,222,222
 *           apple,888,888,888
 *       Then the match.txt output file will contain:
 *           mango,987,654,321,pulp
 *           apple,555,555,555,sauce
 *           apple,888,888,888,sauce
 *       And the nomatch.txt output file will contain:
 *           lemon,012,345,678
 *           berry,222,222,222
 */

#include "sump.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32)
# include <io.h>
# include <windows.h>
# define __inline__   /* empty definition */
# define open _open
#else
#include <sys/mman.h>
#endif


#define HASH_ENTRY_COUNT (0x400000 - 1)
#define HASH_INDEX_FROM_CRC(x) ((x) % HASH_ENTRY_COUNT)
unsigned char    *Hash[HASH_ENTRY_COUNT];


/* get_key_length - get the number of characters from the beginning of a
 *              record until the first comma character (or the end of record
 *              if no comma is present).
 */
static __inline__ int get_key_length(unsigned char *rec)
{
    unsigned char  *p = rec;

    while (*p != ',' && *p != '\0')  /* keep searching until comma or NULL */
        p++;
    return ((int)(p - rec));
}


static __inline__ void fatal_file_error(char *cmd, char *filename)
{
#if defined(_WIN32)
    fprintf(stderr, "can't %s %s: %d\n", cmd, filename, GetLastError());
#else
    fprintf(stderr, "can't %s %s: %s\n", cmd, filename, strerror(errno));
#endif
    exit(1);
}

/* build_hash_table - this single-threaded function builds the hash table
 */
void build_hash_table(char *filename)
{
    unsigned char   *refrecs;
    unsigned char   *rec;
    unsigned char   *p;
    int             index;
    int             key_len;
    unsigned        crc;
    ssize_t         filesize;

    /* map the file */
#if defined(_WIN32)
    HANDLE      fh, mh;
    
    fh = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE)
        fatal_file_error("CreateFile", filename);
    mh = CreateFileMapping(fh, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mh == INVALID_HANDLE_VALUE)
        fatal_file_error("CreateFileMapping", filename);
    refrecs = (unsigned char *)MapViewOfFile(mh, FILE_MAP_COPY, 0, 0, 0);
    if (refrecs == NULL)
        fatal_file_error("MapViewOfFile", filename);
    filesize = (ssize_t)GetFileSize(fh, NULL);
#else
    struct stat statbuf;
    int         fd;

    if ((fd = open(filename, O_RDONLY)) < 0)
        fatal_file_error("open", filename);
    if (fstat(fd, &statbuf) < 0)
        fatal_file_error("fstat", filename);
    refrecs = (unsigned char *)mmap(NULL, statbuf.st_size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE, fd, 0);
    if (refrecs == NULL)
        fatal_file_error("mmap", filename);
    filesize = (ssize_t)statbuf.st_size;
#endif
    for (rec = refrecs; rec < refrecs + filesize; rec = p + 1)
    {
        p = rec;
        while (*p != '\n')
            p++;
        *p = '\0';
        key_len = get_key_length(rec);
        crc = crc32(0, rec, key_len);
        index = HASH_INDEX_FROM_CRC(crc);

        while (Hash[index] != NULL)  /* keep probing until NULL entry found */
            index = (index == HASH_ENTRY_COUNT - 1 ? 0 : index + 1);
        Hash[index] = rec;
    }
}

/* lookup_pump - This pump function simply reads one input record at a
 *               time and looks up the key value in the hash table. If a
 *               match is found it writes the combination of the input
 *               record and the non-key remainder of the reference
 *               record to output 0, the match.txt file. If no match is
 *               found the input record is written to output 1, the
 *               nomatch.txt file.
 */
int lookup_pump(sp_task_t t, void *unused)
{
    unsigned char       *rec;
    unsigned            crc;
    unsigned char       *entry;
    int                 match_found;
    int                 index;
    int                 key_len;
    int                 ref_key_len;
        
    /* for each record in the task input */
    while (pfunc_get_rec(t, &rec) > 0)
    {
        rec[strlen((char *)rec) - 1] = '\0';  /* strip off trailing newline */
        key_len = get_key_length(rec);
        crc = crc32(0, rec, key_len);
        match_found = 0;        /* assume no match for now */
        for (index = HASH_INDEX_FROM_CRC(crc);
             Hash[index] != NULL;
             index = (index == HASH_ENTRY_COUNT - 1 ? 0 : index + 1))
        {
            entry = Hash[index];
            ref_key_len = get_key_length(entry);
            if (key_len == ref_key_len &&
                strncmp((char *)rec, (char *)entry, key_len) == 0)
            {
                match_found = 1;
                /* output entire record in input file to match.txt */
                pfunc_printf(t, 0, "%s", rec);
                /* print post-key remainder of reference record to match.txt */
                pfunc_printf(t, 0, "%s\n", entry + ref_key_len);  
            }
        }
        if (match_found == 0) /* no match found, print record to nomatch.txt */
            pfunc_printf(t, 1, "%s\n", rec);
    }
    return (SP_OK);
}


int main(int argc, char *argv[])
{
    sp_t                sp;
    int                 ret;
    /* sp_time_t           begin_time = sp_get_time_us(); */

    build_hash_table("lookupref.txt");
    /*printf("%lld us to build hash table\n", sp_get_time_us() - begin_time);*/

    /* start sump pump to read lookup.txt file and lookup key values */
    if ((ret = sp_start(&sp, lookup_pump,
                        "-ASCII "
                        "-IN=lookupin.txt -OUTPUTS=2 "
                        "-OUT[0]=match.txt -OUT[1]=nomatch.txt "
                        "-IN_BUF_SIZE=4m "
                        "-OUT_BUF_SIZE[0]=8m "  /* 2x bigger */
                        "-OUT_BUF_SIZE[1]=8m "  /* 2x bigger */
                        "%s",
                        sp_argv_to_str(argv + 1, argc - 1))) != SP_OK)
    {
        fprintf(stderr, "lookup: sp_start failed: %s\n",
                sp_get_error_string(sp, ret));
        exit(1);
    }

    if ((ret = sp_wait(sp)) != SP_OK)
    {
        fprintf(stderr, "lookup: sp_wait: %s\n", sp_get_error_string(sp, ret));
        exit(1);
    }
    return (0);
}
