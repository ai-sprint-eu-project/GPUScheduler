FOLDER ?= $(PWD)/build
LISTDIR = $(FOLDER)/data/lists
LIST ?= list_of_inputs.txt
LISTD ?= list_of_data.txt
SSYS ?= true

.PHONY: help all run list data tests clean testclean distclean

.DEFAULT_GOAL = all

help:
	@echo
	@echo "######################## Makefile for GPUspb program \
	#########################"
	@echo
	@echo "Run make without specifying any target to compile and \
	build the program"
	@echo "Run make <target> to execute a specific task"
	@echo "If a target requires or may receive additional arguments, \
	those must "
	@echo "be provided as"
	@echo "make <target> ARGS=\"additional arguments\""
	@echo "as specified below"
	@echo
	@echo
	@echo "######################### List of possible targets \
	###########################"
	@echo
	@echo "------ help:	print this message and exit"
	@echo
	@echo "------ list:	create a list of arguments for data \
	generation"
	@echo "		and testing, according to the information specified"
	@echo "		in the configuration file (by default, config.ini)"
	@echo
	@echo "		Possible additional argument: name of the \
	configuration file"
	@echo
	@echo "		Example:"
	@echo "		make list"
	@echo "		if the default config.ini file is used;"
	@echo "		make list ARGS=\"-c new_config.ini\""
	@echo "		to specify a different configuration file"
	@echo
	@echo "------ data:	generate data required for testing"
	@echo "		Intended usage:"
	@echo "		1) edit the configuration file"
	@echo "		2) make list"
	@echo "		3) make data [additional arguments]"
	@echo
	@echo "		Possible additional arguments: name of a list of \
	data to be"
	@echo "		generated (default: list_of_data.txt generated by \
	make list);"
	@echo "		additional arguments for script/py/\
	run_experiments.py (run"
	@echo "		python3 script/py/run_experiments.py -h"
	@echo "		for a complete list)"
	@echo
	@echo "		Example:"
	@echo "		make data"
	@echo "		to use the default list_of_data.py and if no \
	additional"
	@echo "		arguments for script/py/run_experiments.py are \
	needed"
	@echo "		make data LISTD=\"other_list_of_data.txt\""
	@echo "		to specify a different list of data to be \
	generated"
	@echo "		make data ARGS=\"additional arguments\""
	@echo "		to pass additional arguments to script/py/\
	run_experiments.py"
	@echo "		e.g. make data ARGS=\"-j 4\""
	@echo "		to use 4 processors for data generation"
	@echo
	@echo "------ run:	run the optimization tool; requires as \
	arguments:"
	@echo "		ARGS=\"method=<method name> folder=<path to result folder>\""
	@echo "		if the method does not uses randomization; the \
	same plus"
	@echo "		seed=<seed> iter=<number of random iterations>"
	@echo "		otherwise;"
	@echo "		make run ARGS=\"-h\""
	@echo "		provides further information about these and other non-mandatory"
	@echo "		parameters"
	@echo
	@echo "		Example:"
	@echo "		make run ARGS=\"method=Greedy folder=~/GPUspb/build/data/\\"
	@echo "		tests_new/1-3-30-1000-high-000/results_0.5\""
	@echo "		or"
	@echo "		make run ARGS=\"method=Greedy folder=~/GPUspb/build/data/\\"
	@echo "		tests_new/1-3-30-1000-high-000/results_0.5 seed=1020 iter=100\""
	@echo
	@echo "------ tests:	automatically performs a series of tests"
	@echo "		Intended usage:"
	@echo "		1) edit the configuration file"
	@echo "		2) make list"
	@echo "		3) make data (IF NEEDED)"
	@echo "		4) make tests [additional arguments]"
	@echo
	@echo "		Possible additional arguments: name of a list of \
	tests to be"
	@echo "		performed (default: list_of_inputs.txt generated \
	by make list);"
	@echo "		additional arguments for script/py/\
	run_experiments.py (run "
	@echo "		python3 script/py/run_experiments.py -h"
	@echo "		for a complete list)"
	@echo
	@echo "		Example:"
	@echo "		make tests"
	@echo "		to use the default list_of_inputs.py and if no \
	additional"
	@echo "		arguments for script/py/run_experiments.py are \
	needed"
	@echo "		make tests LIST=\"other_list_of_inputs.txt\""
	@echo "		to specify a different list of tests to be \
	performed"
	@echo "		make tests ARGS=\"additional arguments\""
	@echo "		to pass additional arguments to script/py/\
	run_experiments.py"
	@echo "		e.g., to use 4 processors to perform the tests, \
	type"
	@echo "		make tests ARGS=\"-j 4\""
	@echo "		to specify a different folder for outputs (written\
	 by default"
	@echo "		in build/output, type"
	@echo "		make tests ARGS=\"-o build/output_new\""
	@echo "		NOTE: the name of the new output folder must start by output"
	@echo
	@echo "------ clean:	remove object files from build/ folder"
	@echo
	@echo "------ testclean:	remove output/ folder generated \
	by make tests"
	@echo
	@echo "------ distclean:	remove object files and \
	executable from build/ folder"
	@echo

all:
	@($(MAKE) SSYS=$(SSYS) -C $(FOLDER))

run:
	@export ARGS="$(ARGS)"
	@(make run -C $(FOLDER))

list:
	python3 script/py/generate_list.py $(ARGS)

data:
	python3 script/py/run_experiments.py -l $(LISTDIR)/$(LISTD) -o \
		outData --exec script/py/generate_data_new.py $(ARGS)
	$(RM) -r outData

data_old:
	python3 script/py/run_experiments.py -l $(LISTDIR)/$(LISTD) -o \
		outData --exec script/py/convert_data.py $(ARGS)
	$(RM) -r outData

tests:
	python3 script/py/run_experiments.py -l $(LISTDIR)/$(LIST) $(ARGS)

clean:
	@(make clean -C $(FOLDER))

testclean:
	$(RM) -r build/output

distclean:
	@(make distclean -C $(FOLDER))