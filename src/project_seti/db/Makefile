# $Id: Makefile.in,v 1.14.2.1 2007/03/13 17:41:27 vonkorff Exp $
#
#



EXEEXT = 
OBJEXT = o
LIBEXT = a
DLLEXT = so
DOTEXEEXT = 


SUFFIXES = .ec .cpp .c .o .so .a 

BOINCDIR = /mnt/usb/boinc-rpi/boinc/boinc_7.1.0
INFORMIXDIR = 
MYSQL_LIBS = 
MYSQL_CFLAGS = 
INFORMIX_CFLAGS = 

DBLIBS=  -lsocket  -lm -lstdc++ 

CC = gcc
CXX = g++
CFLAGS = -D_POSIX_C_SOURCE=1  -I/usr/local/include -I/usr/include -I/usr/include/openssl -I.. -I$(BOINCDIR) -I$(BOINCDIR)/lib -I$(BOINCDIR)/api $(INFORMIX_CFLAGS) $(MYSQL_CFLAGS) -pthread
CXXFLAGS = $(CFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $*.o -c $<


all: dependencies app_config.o schema_master_client.o sqlrow_client.o schema_master.o sqlrow.o sqlifx.o sqlblob.o sqlint8.o xml_util.o

schema_master.cpp: schema_master.sql find_references.awk schema_to_class.awk
	chmod +x schema_to_class
	./schema_to_class schema_master.sql

schema_master.h: schema_master.sql find_references.awk schema_to_class.awk
	chmod +x schema_to_class
	./schema_to_class schema_master.sql


schema_master_client.o: schema_master.cpp schema_master.h db_table.h Makefile
	$(CXX) $(CXXFLAGS) -DCLIENT -c -o schema_master_client.o schema_master.cpp

sqlrow_client.o: sqlrow.cpp sqlrow.h Makefile
	$(CXX) $(CXXFLAGS) -DCLIENT -c -o sqlrow_client.o sqlrow.cpp

schema_master.o: schema_master.cpp schema_master.h db_table.h 

sqlifx.cpp: sqlifx.ec
	$(INFORMIXDIR)/bin/esql -e $<
	mv $*.c $*.cpp

sqlifx.o: sqlifx.cpp

xml_util.o: xml_util.cpp

app_config.o: app_config.cpp

.ec.cpp:
	$(INFORMIXDIR)/bin/esql -e $<
	mv $*.c $*.cpp

Makefile: Makefile.in ../config.status ../configure ../configure.ac
	(cd ..; $(MAKE) config.status)

dependencies: schema_master.cpp *.cpp *.h 
	$(CXX) $(CXXFLAGS) -M *.cpp > dependencies
        


include dependencies
