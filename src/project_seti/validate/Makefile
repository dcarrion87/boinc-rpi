# under construction - jeffc


EXEEXT = 
OBJEXT = o
LIBEXT = a
DLLEXT = so
DOTEXEEXT = 

SUFFIXES = .cpp .c .o .so .a 

BOINCDIR = /mnt/usb/boinc-rpi/boinc/boinc_7.1.0

CC = gcc 
CFLAGS = -D_POSIX_C_SOURCE=1  -I/usr/local/include -I/usr/include -I/usr/include/openssl -DHAVE_CONFIG_H -DTEXT_UI -DNDEBUG  -pthread

CXX = g++ 
CXXFLAGS = $(CFLAGS) 

LDFLAGS =  -L/usr/local/lib -L/usr/lib 

CLIBS =  -lssl -lcrypto 

#SUFFIXES = .cpp .o

#BOINC_INC	= -I$(BOINCDIR)/api -I$(BOINCDIR)/lib -I$(BOINCDIR)/sched -I$(BOINCDIR)/db
BOINCDIR        = /mnt/usb/boinc-rpi/boinc/boinc_7.1.0
BOINC_CFLAGS    = -I/mnt/usb/boinc-rpi/boinc/boinc_7.1.0 -I/mnt/usb/boinc-rpi/boinc/boinc_7.1.0/api -I/mnt/usb/boinc-rpi/boinc/boinc_7.1.0/lib -I/mnt/usb/boinc-rpi/boinc/boinc_7.1.0/sched -I/mnt/usb/boinc-rpi/boinc/boinc_7.1.0/db
BOINC_LIBS 	    = -L$(BOINCDIR)/sched -lsched  -L$(BOINCDIR)/lib -lboinc
MYSQL_CFLAGS    = 
MYSQL_LIBS      = 
INFORMIXDIR     = 
INFORMIX_CFLAGS = 
INFORMIX_LIBS   = 

OBJS = \
    sah_validate.$(OBJEXT) \
    sah_result.$(OBJEXT) \
    sah_boinc_db.$(OBJEXT)

BOINC_OBJS = $(BOINCDIR)/sched/validator.o \
        $(BOINCDIR)/sched/validate_util.o 


PROG = sah_validate

.cpp.o:
	$(CXX) $(CXXFLAGS) $(BOINC_CFLAGS) $(MYSQL_CFLAGS) -I.. -I../db -c -o $*.o $<

.c.o:
	$(CC) $(CFLAGS) $(BOINC_CFLAGS) -I.. -I../db -c -o $*.o $<

#all: Makefile dependencies $(PROGS)
all: Makefile $(PROG)

../aclocal.m4: ../m4/*.m4
	/bin/cat ../m4/*.m4 >../aclocal.m4

../configure: ../configure.ac ../aclocal.m4 ../config.h.in
	(cd ..; make config.h)

$(PROG): $(OBJS) $(BOINC_OBJS)
	$(CXX) $(OBJS) $(BOINC_OBJS) -I../db $(BOINC_CFLAGS) $(CLIBS) $(BOINC_LIBS) $(MYSQL_LIBS) $(INFORMIX_LIBS) -o $(PROG)

clean:
	rm -f *.$(OBJEXT) $(PROG) dependencies config.log config.cache

#include dependencies
