BOINCDIR = /mnt/usb/boinc-rpi/boinc/boinc_7.1.0
INFORMIXDIR= 
BOINCLIBS=-L$(BOINCDIR)/lib -lboinc 
SETIHOME=../..
DBLIBS=../xml_util.o ../sqlblob.o ../sqlrow.o ../sqlifx.o  -ldl -lm -lstdc++ 

INCLUDE_DIRS= -I$(SETIHOME) -I$(SETIHOME)/client -I$(SETIHOME)/db \
               \
               \
	      -pthread \
              -I$(BOINCDIR) -I$(BOINCDIR)/lib -I$(BOINCDIR)/sched -I$(BOINCDIR)/db -I$(BOINCDIR)/tools

LINKOPTIONS=-Xlinker -R -Xlinker $(LD_LIBRARY_PATH):$(INFORMIXDIR)/lib:$(INFORMIXDIR)/lib/esql

SUFFIXES = .cpp .o

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

CC = gcc

CFLAGS = -D_POSIX_C_SOURCE=1  -I/usr/local/include -I/usr/include -I/usr/include/openssl -DHAVE_CONFIG_H $(INCLUDE_DIRS) -DUSE_INFORMIX

CXX = g++

CXXFLAGS = $(CFLAGS)

dependencies: *.cpp 
	$(CC) $(CFLAGS) -M *.cpp > dependencies

all: dependencies print_s4_receivers insert_analysis_config insert_splitter_config insert_receiver_config insert_rfi_zones

insert_s4_receivers: insert_s4_receivers.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_s4_receivers insert_s4_receivers.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_receiver_config: insert_receiver_config.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_receiver_config insert_receiver_config.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_analysis_config: insert_analysis_config.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_analysis_config insert_analysis_config.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_rfi_zones: insert_rfi_zones.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_rfi_zones insert_rfi_zones.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_science_config: insert_science_config.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_science_config insert_science_config.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_splitter_config: insert_splitter_config.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_splitter_config insert_splitter_config.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

print_s4_receivers: print_s4_receivers.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o print_s4_receivers print_s4_receivers.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

include dependencies
