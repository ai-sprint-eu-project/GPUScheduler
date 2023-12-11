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

#ifndef SCHEDULE_HH
#define SCHEDULE_HH

#include "job.hpp"
#include "setup.hpp"
#include "node.hpp"

#include <cassert>

class Schedule {

public:
  // friend operator==, operator!=
  friend bool operator== (const Schedule&, const Schedule&);
  friend bool operator!= (const Schedule&, const Schedule&);

private:
  setup_time_t::const_iterator csetup;
  unsigned node_idx;

  bool empty_schedule = true;

  unsigned iter = 0;
  double sim_time = 0;
  double execution_time = 0.;
  double completion_percent = 0.;
  double completion_percent_step = 0.;
  double start_time = 0.;
  double finish_time = 0.;
  double tardiness = 0.;
  double vmCost = 0.;
  double tardinessCost = 0.;

public:
  /*  constructors
  *
  *   input(1):  void              default
  *
  *   input(2):  setup_time_t::const_iterator   iterator to a setup
  *              unsigned                       node index
  *
  */
  Schedule (void) = default;
  Schedule (setup_time_t::const_iterator, unsigned);

  // getters
  bool isEmpty (void) const {return empty_schedule;}
  //
  double get_startTime (void) const {return start_time;}
  double get_completionPercent (void) const {return completion_percent;}
  double get_cP_step (void) const {return completion_percent_step;}
  double get_simTime (void) const {return sim_time;}
  double get_tardiness (void) const {return tardiness;}
  double get_vmCost (void) const {return vmCost;}
  double get_tardinessCost (void) const {return tardinessCost;}
  //
  const Setup& get_setup (void) const;
  double get_selectedTime (void) const;
  unsigned get_node_idx (void) const;

  // setters
  void set_iter (unsigned it) {iter = it;}
  void set_simTime (double st) {sim_time = st;}
  void set_executionTime (double et) {execution_time = et;}
  void set_completionPercent (double cp) {completion_percent = cp;}
  void set_cP_step (double cp) {completion_percent_step = cp;}
  void set_finishTime (double ft) {finish_time = ft;}
  void set_tardiness (double t);
  void set_startTime (double st) {start_time = st;}
  void compute_vmCost (unsigned);
  void compute_tardinessCost (double);
  void change_setup (setup_time_t::const_iterator stp) {csetup = stp;}
  void change_node (unsigned idx) {node_idx = idx;}

  /*  print_names (static)
  *
  *   Input:  std::ostream&       where to print names of fields stored in the 
  *                               class
  *           char endline='\n'   last character to be printed (default \n)
  */
  static void print_names (std::ostream& ofs, char endline = '\n');

  /*  print
  *
  *   Input:  const Job&                job associated to this schedule
  *           const std::vector<Node>&  vector of open nodes
  *           std::ostream&             where to print
  *           char endline='\n'         last character to be printed
  */
  void print (const Job&, const std::vector<Node>&, std::ostream&, 
              char endline = '\n') const;
};

// operator== and operator!=  two schedules are equal if the corresponding 
//                            setups are equal and they have the same number 
//                            of GPUs
bool operator== (const Schedule&, const Schedule&);
bool operator!= (const Schedule&, const Schedule&);

#endif /* SCHEDULE_HH */
