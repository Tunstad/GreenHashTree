CMN_INC	:= ../common

ARCH:=$(shell uname -p)

#Location of profiler library and include (e.g., PAPI)
PROF_LIB:= /home/ibrahim/local/lib
PROF_INC:= /home/ibrahim/local/include

#Location of Intel PCM library
PCM_DIR:= /opt/intel-pcm

#Addon (default) files
ADDONS	:= $(CMN_INC)/barrier.c $(CMN_INC)/locks.c $(CMN_INC)/bench.c

#Profiling FLAGS, LIBS, ADDONS
SRCPROF = $(CMN_INC)/papicounters.c
ADDFLAG = -I$(PROF_INC) -D__USEPROF -D__MULTIPLEX
ADDLD   = -L$(PROF_LIB) -Wl,-rpath=$(PROF_LIB)
PROFLIB = -lpapi

#Default build objects
OBJS    := $(SRCS:.c=.o) $(ADDONS:.c=.o) 

#Default CFLAGS and LDFLAGS
CCFLAGS = -g -O3 -fPIC -DNDEBUG -D_REENTRANT -Wall -funroll-loops -fno-strict-aliasing -I$(CMN_INC) $(EXTRAC)
LIBS = -lm -lpthread $(EXTRAL)

ifeq ($(TEST), Y)
CCFLAGS += -D__TEST
endif

#Energy FLAGS, LIBS, ADDONS
ifeq ($(ENERGY), Y)

CCFLAGS += -std=c++11 -D__ENERGY
CXXFLAGS += -std=c++11

ifeq ($(ARCH), x86_64)
CC = $(CXX)
SRCENE   = $(CMN_INC)/pcmpower.c 
SRCPCM	  = $(PCM_DIR)/msr.cpp
CCFLAGS += -I$(PCM_DIR) -fpermissive
LDFLAGS += -fPIC -Wl,-rpath=$(PCM_DIR)/intelpcm.so -L$(PCM_DIR)/intelpcm.so -lintelpcm 
endif

ifeq ($(ARCH), armv7l)
SRCENE = $(CMN_INC)/armpower.c
endif

ifeq ($(ARCH), unknown)
SRCENE = $(CMN_INC)/micpower.c
endif

OBJS += $(SRCENE:.c=.o)

endif



ifeq ($(SIM),Y)
GEM5_DIR := ../m5
GEM5_DIR_THREAD := $(GEM5_DIR)/m5thread
GEM5_DIR_OPS	:= $(GEM5_DIR)/m5ops
CCFLAGS += -D__SIM -I$(GEM5_DIR_THREAD) -I$(GEM5_DIR_OPS) #-DM5OP_ADDR=0xFFFF0000
LDFLAGS := -static -fPIC
LIBS	:= -lm -lpthread
SIM_OBJS := $(GEM5_DIR_THREAD)/pthread.o $(GEM5_DIR_OPS)/m5op_x86.o
endif

ifeq ($(PROF),Y)
OBJS += $(SRCPROF:.c=.o)
CCFLAGS += $(ADDFLAG)
LDFLAGS += $(ADDLD) $(PROFLIB)
endif

LDFLAGS = -shared -fPIC

.PHONY: all clean
all:: $(TARGET) 

ifeq ($(SIM),Y)
$(TARGET): $(OBJS) $(SIM_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) 
else
$(TARGET): $(OBJS) $(SIM_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) -lc
endif

$(OBJS): %.o: %.c
	$(CC) $(CCFLAGS) $(TREE) -o $@ -c $< 

clean:: 
	-rm -f *~ $(OBJS) $(SIM_OBJS) $(TARGET) 

ifeq ($(SIM),Y)
$(GEM5_DIR_THREAD)/pthread.o: $(GEM5_DIR_THREAD)/pthread.c $(GEM5_DIR_THREAD)/pthread_defs.h $(GEM5_DIR_THREAD)/tls_defs.h
	$(CC) $(CCFLAGS) -c $(GEM5_DIR_THREAD)/pthread.c -o $(GEM5_DIR_THREAD)/pthread.o
$(GEM5_DIR_OPS)/m5op_x86.o: $(GEM5_DIR_OPS)/m5op_x86.S
	$(CC) -O2 -c $(GEM5_DIR_OPS)/m5op_x86.S -o $(GEM5_DIR_OPS)/m5op_x86.o	
endif
