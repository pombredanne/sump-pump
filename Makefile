# SUMP Pump Makefile
#
# $Revision$
# 
# This Makefile currently only works for x86 and x64 Linux
#

include Make.version

#regression tests and files
REG_TESTS=reduce reducefixed upper upperfixed upperwhole oneshot sumpversion
REG_FILES=rin1.txt rin2.txt rin3.txt upper_correct.txt rout1_correct.txt \
          rout2_correct.txt rout3_correct.txt

# performance tests and files
PERF_TESTS=spgzip lookup billing gensort valsort
PERF_FILES=lookupref.txt lookupin.txt spgzipinput billing_input.txt \
           billing_correct_output.txt sortoutput.txt word_100MB.txt

CFLAGS=-Wall
LIB=libsump.so.1

default: $(LIB) sump

all: default reg perf

$(LIB): sump.o
	gcc -o $(LIB) -shared \
	-Xlinker --version-script=exports.linux sump.o -lpthread -ldl -lrt

# SUMP Pump object file. Optimization is not used in order to aid
# debugging.  Optimization doesn't seem to speed things up that much as
# most CPU use tends to be done by pump functions.
#
sump.o: sump.c sump.h sumpversion.h
	gcc -c -fPIC $(CFLAGS) -g sump.c

sump: sump.o main.c sump.h sumpversion.h
	gcc -g -o sump main.c sump.o -lpthread -ldl -lrt

sumpversion.h: sump.c sump.h
	(echo -n 'static char *sp_version = "'; echo -n $(RELEASE); echo -n ', svn: '; svnversion -n; echo '";') > sumpversion.h

reg: $(REG_TESTS) $(REG_FILES) 

perf: $(PERF_TESTS) $(PERF_FILES)

# Install rule to place the sump pump library in the library directory, 
# and the sump.h file in the include directory.
# 
# Note that it is not necessary to "install" sump pump in order to use it.
# Without an "install", you should have the library in a directory that 
# is in your LD_LIBRARY_PATH environment variable.
#
install: $(LIB)
	sudo cp $(LIB) /usr/lib64
	sudo cp sump.h /usr/include

# regression tests
sumpversion: sumpversion.c $(LIB)
	gcc -g $(CFLAGS) -o sumpversion sumpversion.c $(LIB)

oneshot: oneshot.c $(LIB)
	gcc -g $(CFLAGS) -o oneshot oneshot.c $(LIB)

reduce: reduce.c $(LIB)
	gcc -g $(CFLAGS) -o reduce reduce.c $(LIB)

reducefixed: reducefixed.c $(LIB)
	gcc -g $(CFLAGS) -o reducefixed reducefixed.c $(LIB)

upper: upper.c $(LIB)
	gcc -g $(CFLAGS) -o upper upper.c $(LIB)

upperfixed: upperfixed.c $(LIB)
	gcc -g $(CFLAGS) -o upperfixed upperfixed.c $(LIB)

upperwhole: upperwhole.c $(LIB)
	gcc -g $(CFLAGS) -o upperwhole upperwhole.c $(LIB)

# regression files
$(REG_FILES):
	genreduce.py

# performance tests
billing: billing.c $(LIB)
	gcc -g $(CFLAGS) -o billing billing.c $(LIB) 

spgzip: spgzip.c $(LIB)
	gcc -g $(CFLAGS) -o spgzip spgzip.c $(LIB) -lz 

lookup: lookup.c $(LIB)
	gcc -g $(CFLAGS) -o lookup lookup.c $(LIB) -lz 

gensort: gensort.c rand16.o $(LIB)
	gcc -g $(CFLAGS) -o gensort gensort.c rand16.o $(LIB) -lz 

valsort: valsort.c rand16.o $(LIB)
	gcc -g $(CFLAGS) -o valsort valsort.c rand16.o $(LIB) -lz 

rand16.o: rand16.c rand16.h
	gcc -g $(CFLAGS) -c rand16.c


# performance input files
lookupref.txt:
	@echo warning: generating lookupref.txt takes a long time
	gencsv.py 0 2000000 str str | sort -u -k 1,1 -t , > lookupref.txt

lookupin.txt:
	@echo warning: generating lookupin.txt takes a long time
	gencsv.py 1 20000000 str seq u4 u4 u4 > lookupin.txt

spgzipinput:
	@echo warning: generating spgzipinput takes a long time
	gencsv.py 1 8000000 str seq u4 u4 > spgzipinput

billing_input.txt billing_correct_output.txt:
	@echo warning: generating billing_input.txt takes a long time
	genbilling.py 1000000 | sort -k 5,5 -t , > billing_input.txt

sortoutput.txt: gensort
	gensort 10000000 sortinput.txt
	@echo warning: this sort takes a long time
	(LC_ALL=C; export LC_ALL; sort sortinput.txt -o sortoutput.txt)

word_100MB.txt:
	@echo downloading data file from the web...
	urlretrieve.py http://mapreduce.stanford.edu/datafiles/word_count.tar.gz word_count.tar.gz
	gzip -dc < word_count.tar.gz | tar xvf - word_count_datafiles/word_100MB.txt
	mv word_count_datafiles/word_100MB.txt .
