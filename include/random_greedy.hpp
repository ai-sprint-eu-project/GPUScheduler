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

#ifndef RANDOM_GREEDY_HH
#define RANDOM_GREEDY_HH

#include "greedy.hpp"
#include "best_solution.hpp"

class Random_greedy: public Greedy {

protected:
  // type definitions
  typedef std::map<double, Best_solution> K_best_t;

  // maximum number of best solutions to be saved
  unsigned K = 1;
  // map of best solutions (ordered by minimum cost)
  K_best_t K_best;

  // parameters for randomization
  double alpha = 0.05;                    // parameter for setup selection
  double pi = 0.05;                       // parameter for random swaps in 
                                          //  submitted_jobs
  double beta = 0.2;                      // parameter for node selection
  bool r_swap = true;                     // true to swap jobs in sorted list

  /* random_swap
  *   randomly swap jobs in the sorted list
  */
  void random_swap (void);

  /* select_best_node
  *   return the best node the given setup fits in; a node is the best-fit 
  *   if the new setup (almost) saturates all its idle GPUs
  *
  *   Input:  const Setup&    setup to be allocated in an opened node
  *
  *   Outpu:  unsigned        index of the best opened node
  */
  unsigned select_best_node (const Setup&);

  /* assign_to_suboptimal:
  *   assign the given job to a suboptimal configuration, available in 
  *   an already opened node
  *
  *   Input:  const Job&                      job to be assigned
  *           const setup_time_t&             execution times of the given job
  *           Dstar&                          see dstar.hpp
  *           job_schedule_t&                 new proposed schedule
  *
  *   Output: bool                            true if job has been assigned
  */
  virtual bool assign_to_suboptimal (const Job&, const setup_time_t&,
                                     Dstar&, job_schedule_t&) override;

  /* perform_assignment 
  *   assign the selected job to the new proposed schedule
  *
  *   Input:  const Job&                      job to be assigned
  *           job_schedule_t&                 new proposed schedule
  */
  virtual void perform_assignment (const Job&, job_schedule_t&) override;

  /* perform_scheduling
  *   perform the scheduling process that assigns the best configuration to
  *   each submitted jobs (as long as resources are available)
  *
  *   Output: job_schedule_t        proposed schedule
  */
  virtual job_schedule_t perform_scheduling (void) override;

  /* scheduling_step
  *   sort the list of submitted jobs and perform scheduling of all
  *   submitted jobs
  *
  *   Input:    job_schedule_t&     empty schedule to be built
  */
  virtual void scheduling_step (job_schedule_t&) override;

  /* restore_map_size
  *  erase the element in the map with the worst cost (the last element), so 
  *  that the size of the map returns less than or equal to the maximum size K
  *
  *  Input:     K_best_t&           map to be modified
  */
  virtual void restore_map_size (K_best_t&);

  /* to_be_inserted
  *  return true if the element characterized by the given upper bound should
  *  be inserted in the map
  */
  virtual bool to_be_inserted (const K_best_t&, K_best_t::iterator) const;

  /* update_best_schedule
  *   compute the cost of the given schedule and inserts it in the map of
  *   best solutions if the cost is better than other costs already in the map
  *
  *   Input:    job_schedule_t&         new proposed schedule
  */
  virtual void update_best_schedule (job_schedule_t&);

  public:
  /*  constructor
  *
  *   Input:  const std::string&    full path of data directory
  *           const std::string&    name of file with the list of jobs
  *           const std::string&    name of file with execution times of jobs
  *           const std::string&    name of file with the list of nodes
  *
  */
  Random_greedy (const std::string&, const std::string&, const std::string&, 
                 const std::string&);
  
  /* destructor
  */
  virtual ~Random_greedy (void) = default;

};

#endif /* RANDOM_GREEDY_HH */