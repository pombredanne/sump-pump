SUMP Pump Source Code Distribution
$LastChangedDate$
$Revision$

What's New in Version 1.2.3?
----------------------------
- "sump" program
- As an alternative to defining a pump function, the ability to invoke an
  external program reading its standard input and writing to standard output.
- A '-' character is now required before all sump pump directives in the
  string passed to sp_start().  THIS IS INCOMPATIBLE WITH PROGRAMS WRITTEN
  FOR PREVIOUS VERSIONS OF THE SUMP PUMP LIBRARY.
- Windows makefile (Make.win)

Contents
--------
  The SUMP Pump source code and associated include files:
      sump.c
      sump.h 
      main.c
      sump_win.c    (Windows only)
      sump_win.h    (Windows only)
      sumpversion.c
      sumpversion.h
      nsort.h
      nsorterrno.h

   Makefile and supporting files:
      Makefile
      Make.version
      exports.linux
      Make.win

   Regression tests, regression input file generators and test script:
      oneshot.c
      reduce.c
      reducefixed.c
      upper.c
      upperfixed.c
      upperwhole.c
      genreduce.py
      runregtests.py
      hounds.txt
      correct_hounds_wc.txt
      map.c
      red.c

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
      urlretrieve.py
      mapper.py
      reducer.py
      wordcount.sh

   GNU GPL 2.0 License
      gpl-2.0.txt

Directions
----------
   On Linux:
      To build the SUMP Pump Library, libsump.so.1, and the "sump" executable
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

   On Windows 
      (For nmake commands, must be inside Visual Studio Command Prompt window)
      To build libsump.lib and libsump.dll
         nmake -f Make.win libsump.dll

      To build sump.exe
         nmake -f Make.win sump.exe

      To build regression test programs
         nmake -f Make.win regtestprogs

      Performance test programs currently do not work on Windows

      To build all executables (including regression tests)
         nmake -f Make.win

      To build the regression test files
         make regtestfiles     (must be in cygwin environment)

      To run the regression tests
         runregtests.py        (must be in cygwin environment)
