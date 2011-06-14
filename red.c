/* red.c - SUMP Pump(TM) C programming language version of reducer.py.
 *         Can be used in place of reducer.py to perform a word count
 *         operation.
 *
 *         SUMP Pump is a trademark of Ordinal Technology Corp
 *
 * $Revision: 42 $
 *
 * Copyright (C) 2011, Ordinal Technology Corp, http://www.ordinal.com
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
 * Usage: red < input_file > output_file
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#if !defined(win_nt)
# include <ctype.h>
#endif

char    Last_word[100];
int     Total;


void output()
{
    if (Last_word[0] != '\0')
        printf("%s was found %d times\n", Last_word, Total);
}


int main(int argc, char *argv[])
{
    char        buf[200];
    char        *tab;
    size_t      word_len;
    size_t      line_len;

    for (;;)
    {
        /* if no more input lines, break out of loop */
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;
        if ((line_len = strlen(buf)) == 0 || buf[line_len - 1] != '\n')
        {
            fprintf(stderr, "input line read failure occured\n");
            exit(1);
        }
        if ((tab = strchr(buf, '\t')) == NULL)
        {
            fprintf(stderr, "red: can't find tab in: %s", buf);
            return (1);
        }
        word_len = tab - buf;
        /* if not the same word as Last_word */
        if (word_len != strlen(Last_word) ||
            strncmp(buf, Last_word, word_len) != 0)
        {
            output();
            memcpy(Last_word, buf, word_len);
            Last_word[word_len] = '\0';
            Total = 0;
        }
        Total += atoi(tab + 1);
    }
    output();
    return (0);
}

