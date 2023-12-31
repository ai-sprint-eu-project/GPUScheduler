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

#ifndef ANALYSIS_HH
#define ANALYSIS_HH

#include "fileIO.hpp"

#include <unordered_set>

class Analysis {

private:
  // type definitions
  typedef std::vector<std::unordered_set<std::string>> string_matrix_t;

  // full path of the results folder
  std::string directory = "";

  unsigned slot_number = 0;
  std::vector<double> sim_times;
  std::vector<double> costs;
  std::vector<double> tardiness;
  string_matrix_t VM_types;
  string_matrix_t nodes;

  /* print_costs
  *   prints aggregated costs on a file whose name is cost-method-ARGS.csv for
  *   non-random methods and cost-method-ARGS_seed.csv for random methods (so 
  *   that the file name is unique)
  *
  *   Input:    const std::string&    name of the method used to produce the
  *                                   analyzed result
  *             unsigned              seed used during the algorithm (needed to
  *                                   produce an unique file name) - default
  *                                   value: 0 (itentifies non-random methods)
  */
  void print_costs (const std::string&, unsigned seed = 0) const;

  /* print_tardiness
  *   prints aggregated information about tardiness on a file whose name is 
  *   tardi-method-ARGS.csv for non-random methods and 
  *   tardi-method-ARGS_seed.csv for random methods (so that the file name is 
  *   unique)
  *
  *   Input:    const std::string&    name of the method used to produce the
  *                                   analyzed result
  *             unsigned              seed used during the algorithm (needed to
  *                                   produce an unique file name) - default
  *                                   value: 0 (itentifies non-random methods)
  */
  void print_tardiness (const std::string&, unsigned seed = 0) const;

public:
  /*  constructors
  *
  *   Input(1):  const std::string&   full path of the results folder
  */
  Analysis (const std::string&);

  /* perform_analysis
  *   analyses the file whose name is passed as parameter, storing a schedule
  *   built by an heuristic algorithm, to produce aggregated information 
  *   about costs and tardiness in the different iterations
  *
  *   Input:    const std::string&    name of file storing the schedule to be
  *                                   analyzed
  */
  void perform_analysis (const std::string&);

  /* print
  *   prints the information produced by the analysis, using the methods
  *   print_costs and print_tardiness (receives the same parameters)
  */
  void print (const std::string&, unsigned seed = 0) const;

};

#endif /* ANALYSIS_HH */