SUMP Pump Source Code Distribution
$LastChangedDate$
$Revision$

Contents
--------
  The SUMP Pump source code and associated include files:
      sump.c
      sump.h 
      sumpversion.c
      nsort.h
      nsorterrno.h

   Makefile and supporting files:
      Makefile
      Make.version
      exports.linux

   Regression tests, regression input file generators and test script:

      oneshot.c
      reduce.c
      reducefixed.c
      upper.c
      upperfixed.c
      upperwhole.c
      
      genreduce.py

      runregtests.py

   Performance tests, performance input file generators, and test script:

      billing.c
      gensort.c
      lookup.c
      rand16.c
      rand16.h
      spgzip.c
      valsort.c

      genbilling.py
      gencsv.py

      runperftests.py

   GNU GPL 2.0 License

      gpl-2.0.txt

Directions
----------
   To build the SUMP Pump Library, libsump.so.1

      make

   To build the regression tests, including input files

      make reg

   To run the regression tests

      runregtests.py

   To build the performance tests, including input files

      make perf

   To run the performance tests

      runperftests.py

   To make everything

      make all
