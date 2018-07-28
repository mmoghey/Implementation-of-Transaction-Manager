#
# Makefile for TX Manager project.  Needs GNU make.
#
# Define DEBUGFLAGS for debugging output
#
# Warning: make depend overwrites this file.

.PHONY: depend clean backup setup

MAIN=test

# Change the following line depending on where you have copied and unzipped the files
#solutions dir should have src, includes, and test-files directories
#change with your path if you are not using omega.uta.edu

TXMGR=..

#set DIRPATH to the dir from where you use the g++ compiler, change with your path if you are not using omega.uta.edu omega.uta.edu 
DIRPATH=/usr

CC=$(DIRPATH)/bin/g++ 

# EXAMPLE: In the next line only TX_DEBUG is enabled
#DEBUGFLAGS =  -DTX_DEBUG # -DTM_DEBUG -DHT_DEBUG

#Below, all are enabled; you can disable it as you wish
DEBUGFLAGS = -DTX_DEBUG -DTM_DEBUG -DHT_DEBUG

INCLUDES = -I${TXMGR}/include -I.

LINCLUDES = -L$(DIRPATH)/lib

SRCS = transx_test.cpp transx_manager.cpp transx.cpp transx_hash.cpp transx_sem.cpp transx_deadlock.cpp
OBJS = $(SRCS:.cpp=.o)

$(MAIN):  $(OBJS) Makefile
	 $(CC) -g -pthread $(CFLAGS) $(DEBUGFLAGS) $(INCLUDES) $(OBJS) -o $(MAIN) $(LFLAGS)

.cpp.o:
	$(CC) -g $(CFLAGS) $(INCLUDES) $(LINCLUDES) $(DEBUGFLAGS) -c $<

depend: $(SRCS) Makefile
	makedepend $(INCLUDES)  $^

clean:
	rm -f *.o *~ $(MAIN)

# Grab the sources for a user who has only the makefile
setup:
	/bin/cp -f $(TXMGR)/src/*.[cpp] .
	/bin/cp -f $(TXMGR)/test_cases/*.txt .
	/bin/cp -f $(TXMGR)/includes/*.[hpp]

# DO NOT DELETE THIS LINE -- make depend needs it
