SUMP Pump Binary Distribution
$LastChangedDate$
$Revision$

Copyright (C) 2010 - 2011, Ordinal Technology Corp, http://www.ordinal.com

This program and library is free software; you can redistribute it 
and/ormodify it under the terms of Version 2 of the GNU General Public
License as published by the Free Software Foundation.

This program and library are distributed in the hope that it will be 
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Linking SUMP Pump statically or dynamically with other modules is
making a combined work based on SUMP Pump.  Thus, the terms and
conditions of the GNU General Public License v.2 cover the whole
combination.

In addition, as a special exception, the copyright holders of SUMP Pump
give you permission to combine SUMP Pump program with free software
programs or libraries that are released under the GNU LGPL and with
independent modules that communicate with SUMP Pump solely through
Ordinal Technology Corp's Nsort Subroutine Library interface as defined
in the Nsort User Guide, http://www.ordinal.com/NsortUserGuide.pdf.
You may copy and distribute such a system following the terms of the
GNU GPL for SUMP Pump and the licenses of the other code concerned,
provided that you include the source code of that other code when and
as the GNU GPL requires distribution of source code.

Note that people who make modified versions of SUMP Pump are not
obligated to grant this special exception for their modified
versions; it is their choice whether to do so.  The GNU General
Public License gives permission to release a modified version without
this exception; this exception also makes it possible to release a
modified version which carries forward this exception.

For more information on SUMP Pump, see:
    http://www.ordinal.com/sump.html
    http://code.google.com/p/sump-pump/


Contents
--------
   The include files
      sump.h 
      sump_win.h            (Windows only)

   32,64 directories for executables and libraries
      {32,64}/sump          (Linux only)
      {32,64}/libsump.so.1  (Linux only)
      {32,64}/sump.exe      (Windows only)
      {32,64}/libsump.lib   (Windows only)
      {32,64}/libsump.dll   (Windows only)

   GNU GPL 2.0 License
      gpl-2.0.txt

Directions
----------
For sump executable, invoke it with no arguments to view1 self-documentation.

For building programs that utilize the sump pump library, see comments in
the sump.h file for documentation.  The compiler command line for building
programs that use the sump pump library are as follows:
   On Linux
      gcc $(CFLAGS) -o my_sump_prog my_sump_prog.c libsump.so.1

   On Windows      
      cl $(CFLAGS) my_sump_prog.c libsump.lib
