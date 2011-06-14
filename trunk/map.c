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
 * Usage: map < input_file > output_file
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
    int         c;

    for (;;)
    {
        /* skip over any non-alphabetic characters */
        while ((c = getchar()) != EOF && !isalpha(c))
            continue;
        if (c == EOF)
            break;
        /* while alphabetic characters, output them */
        do
        {
            putchar(tolower(c));
        } while ((c = getchar()) != EOF && isalpha(c));
        /* output tab character, digit '1', and newline */
        fwrite("\t1\n", 1, 3, stdout);
    }
    return (0);
}

