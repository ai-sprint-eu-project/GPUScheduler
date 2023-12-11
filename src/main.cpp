// Copyright 2020-2021 Federica Filippini
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <cstring>

#include "greedy.hpp"
#include "random_greedy.hpp"
#include "local_search.hpp"
#include "path_relinking.hpp"
#include "FIFO.hpp"
#include "EDF.hpp"
#include "Priority.hpp"
#include "analysis.hpp"
#include "builders.hpp"
#include "GetPot.hpp"


void print_help (void);


int main (int argc, char *argv[])
{
  int exit_code = 0;

  // initialze Getpot parser
  GetPot GP(argc,argv);
    
  if (GP.search(1,"-h"))
  	print_help();
  else
  {
  	std::string method = GP("method", "");
  	std::string directory = GP("folder", "");
  	unsigned cpp_seed = GP("seed", 0);
  	unsigned n_random_iter = GP("iter", 0);
    unsigned verbose = GP("verbose", 0);

  	if (method == "" || directory == "" || 
  		(method == "RandomGreedy" && n_random_iter == 0) ||
  		(method == "LocalSearch" && n_random_iter == 0))
  	{
  	  std::cerr << "\nERROR: missing or corrupted parameters" << std::endl;
  	  print_help();
  	  exit_code = 1;
  	}
  	else
  	{
	  	// create factory (TODO: move this where it should be)
      typedef std::unique_ptr<BaseBuilder<Heuristic>> GreedyBuilder;
      typedef std::map<std::string, GreedyBuilder> factory_t;
      factory_t factory;
      factory["Greedy"]       = GreedyBuilder(new Builder<Greedy, Heuristic>);
      factory["RandomGreedy"] = GreedyBuilder(new Builder<Random_greedy, Heuristic>);
      factory["LocalSearch"]  = GreedyBuilder(new Builder<Local_search, Heuristic>);
      factory["PathRelinking"]= GreedyBuilder(new Builder<Path_relinking, Heuristic>);
      factory["FIFO"]         = GreedyBuilder(new Builder<FIFO, Heuristic>);
      factory["EDF"]          = GreedyBuilder(new Builder<EDF, Heuristic>);
      factory["Priority"]     = GreedyBuilder(new Builder<Priority, Heuristic>);

      // name of files containing data
	    std::string jobs_list_filename = "Lof_Selectjobs.csv";
	    std::string times_filename = "SelectJobs_times.csv";
	    std::string nodes_filename = "tNodes.csv";

	    // initialization of method
	    factory_t::const_iterator where = factory.find(method);
	    if (where != factory.end())
	    {
	      std::unique_ptr<Heuristic> G = where->second->create(directory,
	                                                        	jobs_list_filename, 
	                                                        	times_filename, 
	                                                        	nodes_filename);

	      std::string result_filename = method + "_schedule";

	      // for random methods
	      if (method == "RandomGreedy" || method == "LocalSearch" ||
            method == "PathRelinking")
	      {
	      	result_filename += ("_" + std::to_string(cpp_seed) + ".csv");
          G->algorithm(verbose, cpp_seed, n_random_iter);
	      }
	      else
	      {
	      	result_filename += ".csv";
	      	G->algorithm(verbose);
	      }

	      // print resulting schedule
        G->print_schedule(result_filename);

        #ifdef SMALL_SYSTEM
          // analysis
          Analysis A(directory);
          A.perform_analysis(result_filename);
          A.print(method, cpp_seed);
        #endif
	    }
	    else
	    {
	      std::cerr << "ERROR: method " << method << " is not registered"
	                << std::endl;
	      exit_code = 2;
	    }
  	}
  }

  return exit_code;

}


void print_help (void)
{
  std::cout << "usage: ./main [arguments]\n";
  std::cout << "\nrequired arguments (for all methods):\n";
  std::cout << "\tmethod=:     name of the method you want to use\n";
  std::cout << "\tfolder=:     complete path of data folder\n";
  std::cout << "\nadditional arguments (for random methods):\n";
  std::cout << "\tseed=:       seed for randomization\n";
  std::cout << "\titer=:       number of random iterations\n";
  std::cout << "\noptional arguments:\n";
  std::cout << "\tverbose=:    verbosity level (0, 1, 2)\n" << std::endl;
}
