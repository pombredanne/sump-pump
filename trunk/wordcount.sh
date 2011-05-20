#!/usr/bin/env sh
#
# wordcount.sh - shell script to run performance tests for "word count"
#                MapReduce example using mapper.py, nsort and reducer.py.
#                The input file, word_100MB.txt, is part of the word count
#                input dataset for Stanford's Phoenix system for MapReduce,
#                available at http://mapreduce.stanford.edu
#
# $Revision$
#
sump $1 mapper.py < word_100MB.txt | nsort $1 -match -format:sep=tab -field:word,char,count,decimal,max=4 -key:word -sum:count -nowarn -mem:300m | sump $1 -group reducer.py > out.txt

