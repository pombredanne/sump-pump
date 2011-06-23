#!/usr/bin/env sh
#
# wordcount.sh - shell script to run performance tests for "word count"
#                MapReduce example using mapper.py, nsort and reducer.py.
#                The input file, word_100MB.txt, is part of the word count
#                input dataset for Stanford's Phoenix system for MapReduce,
#                available at http://mapreduce.stanford.edu
#
# Argument 1 ($1), if specified, is used to specify the number of threads
# to use.  For example: -threads=4
# For performance testing, this shell scipt may be invoked with "taskset"
# to limit system processors this script and it children can run on.
# For instance, to limit the script to run on 4 processors, this script
# would be invoked with:
#    taskset -c 0-3 wordcount.sh -threads=4
#
# $Revision$
#

# Use sump program to process word_100MB.txt file using mapper.py in parallel 
sump $1 mapper.py < word_100MB.txt | \
# 
# Run nsort to sort mapper.py output.  The nsort arguments are:
#  -format:sep=tab   Input records are lines of text with tab-separated fields
#  -field:word,char,count,decimal,max=4  Fields are "word" and "count".
#                                        "count" is numeric and max of 4 digits
#  -key:word      Key is "word" field.
#  -sum:count     Summarize count field when deleting recs with duplicate keys.
#  -nowarn        Suppress warnings, especially the one that warns that.
#                 duplicate records were not deleted because maximum size
#                 of the "count" field would be exceeded.
#  -match         Output extra information that allows downstream sump pump
#                 to avoid splitting records with equal keys to different
#                 reducer.py instances.
#  -mem:300m      Use 300 megabytes of process memory.
#
nsort $1 -format:sep=tab -field:word,char,count,decimal,max=4 -key:word \
  -sum:count -nowarn -match -mem:300m | \
#
# Use sump program to process sorted output in parallel with reducer.py.
# The "-group" argument directs the sump pump to use the extra information
# produced with the nsort "-match" directive to insure records with the
# same key values are processed by the same reducer.py instance.
#
sump $1 -group reducer.py > out.txt
