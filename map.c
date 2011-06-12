/* map.c - SUMP Pump(TM) C programming language version of mapper.py
 *
 *         SUMP Pump is a trademark of Ordinal Technology Corp
 *
 * $Revision: 42 $
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
 * Usage: map [sump pump directives] < input_file > output_file
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#if !defined(win_nt)
# include <ctype.h>
#endif



int main(int argc, char *argv[])
{
    char        *begin, *end;
    char        buf[200];

    for (;;)
    {
        /* if can't read another input line, stop. */
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;
        for (begin = buf; ; )
        {
            /* skip over any non-alphabetic chars */
            while (*begin != '\0' && !isalpha(*begin))
                begin++;
            if (*begin == '\0')  /* if end of line */
                break;
            end = begin;
            do  /* for each character in the word */
            {
                *end = tolower(*end);
                end++;
            } while (isalpha(*end));
            /* output the word, tab character and digit '1' */
            printf("%.*s\t1\n", end - begin, begin);
            begin = end;
        }
    }
    return (0);
}

