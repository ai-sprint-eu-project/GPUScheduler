HPATH = ../include
SPATH = ../src
SSYS ?= false
ROOT_RESTC ?= /home/lorenzo/restc-cpp
BOOST_INC_PATH ?= ${mkBoostInc}
BOOST_LIB_PATH ?= ${mkBoostLib}
CXXFLAGS += -O3 -std=c++14
CPPFLAGS += -I$(HPATH) -I$(SPATH) -I$(ROOT_RESTC)/include \
	-I$(ROOT_RESTC)/dbuild/generated-include \
	-I$(BOOST_INC_PATH) \
	-I$(ROOT_RESTC)/dbuild/external-projects/src/externalRapidJson/include
ifeq ($(SSYS),true)
	CPPFLAGS += -DSMALL_SYSTEM
endif
VPATH = $(HPATH) $(SPATH)
LDFLAGS = -L$(ROOT_RESTC)/lib -DBOOST_COROUTINE_NO_DEPRECATION_WARNING -L$(BOOST_LIB_PATH)
LIBS = -lrestc-cpp -lz -lssl -lcrypto -lpthread \
        -lboost_system -lboost_program_options \
        -lboost_filesystem -lboost_date_time \
        -lboost_context -lboost_coroutine \
        -lboost_chrono -lboost_log -lboost_thread \
        -lboost_log_setup -lboost_regex \
        -lboost_atomic -lpthread

SRCS=$(patsubst $(SPATH)/%.cpp, %.cpp, $(wildcard $(SPATH)/*.cpp))
OBJS = $(SRCS:.cpp=.o)
exe_sources=$(filter main%.cpp,$(SRCS))
EXE=$(exe_sources:.cpp=)

.PHONY: all dep build run clean distclean

.DEFAULT_GOAL = all

-include make.dep

all: dep build

build: $(EXE)

dep: $(SPATH)
	$(RM) make.dep
	for f in $$(ls $(SPATH)); do \
	       $(CXX) $(CXXFLAGS) $(CPPFLAGS) -MM $(SPATH)/$$f >> make.dep; \
	done

$(EXE): main.o $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

run:
	@./$(EXE) $(ARGS)

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) make.dep
	$(RM) $(EXE)
	$(RM) *~
