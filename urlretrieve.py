#!/usr/bin/env python
#
# Simple python program to retrieve a file from the web.  Used as part of
# the process of getting the word_100MB.txt file from the Stanford Phoenix
# web site.
#
# $Revision$
#
from urllib import urlretrieve
import sys

if len(sys.argv) != 3:
    print 'usage: urlretrieve.py your_URL your_local_file'
    sys.exit(1)
else:
    urlretrieve(sys.argv[1], sys.argv[2])
