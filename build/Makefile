HPATH = ../include
SPATH = ../src
SSYS ?= false
CXXFLAGS += -O3 -Wall -std=c++11
CPPFLAGS += -I$(HPATH) -I$(SPATH)
ifeq ($(SSYS),true)
	CPPFLAGS += -DSMALL_SYSTEM
endif
VPATH = $(HPATH) $(SPATH)

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
	$(CXX) $(CXXFLAGS) $^ -o $@

run:
	@./$(EXE) $(ARGS)

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) make.dep
	$(RM) $(EXE)
	$(RM) *~
