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

#ifndef GREEDY_HH
#define GREEDY_HH

#include "heuristic.hpp"
#include "dstar.hpp"

/*
*
    rules for Greedy Algorithm:

    1) jobs are ordered with respect to pressure (current time + minimum 
       execution time - deadline)

    2) each job in the ordered queue is assigned to:
       2.1) the optimal configuration on an already opened node, if possible
       2.2) a new node, equipped with the optimal configuration in terms of
            GPUType and with the VMType that allows to select the highest 
            number of available GPUs, if possible
       2.3) the best among the suboptimal configurations available on an 
            already opened node with residual space, if possible
       
       NOTE: assignment to already opened nodes is performed with a 
       best-fit approach

    3) postprocessing phase:
       3.1) for all opened nodes with idle GPUs, a new configuration is
            selected, with the same VMtype and GPUtype and maxnGPUs equal
            to the number of used GPUs (or usedGPUs+1, since there are no
            available configurations with, for instance, 3 GPUs), if possible
       3.2) the GPUs remaining idle are assigned to the job with best 
            improvement in performance (: with the highest speed-up)
*
*/

class Greedy: public Heuristic {

protected:
  // type definitions
  typedef std::pair<setup_time_t::const_iterator, 
                    setup_time_t::const_iterator> range_t;

  /* compare_pressure (static)
  *   return true if the 1st job's pressure is higher than the 2nd's
  */
  static bool compare_pressure (const Job&, const Job&);

  /* compare_configuration
  *   return true if the given Setup and Node have the same configuration 
  *   (same GPUtype)
  */
  virtual bool compare_configuration (const Setup&, const Node&) const;

  /* sort_jobs_list
  *   sort the submitted_jobs list according to compare_pressure
  */
  virtual void sort_jobs_list (void) override;

  /* objective_function
  *   evaluate objective function of the current schedule
  *
  *   Input:  job_schedule_t&             new proposed schedule
  *           double                      elapsed time between previous and 
  *                                       current iteration
  *           const std::vector<Node>&    vector of open nodes
  *
  *   Output: double              total cost of the proposed schedule
  */
  virtual double objective_function (job_schedule_t&, double,
                                     const std::vector<Node>&) override;

  /* select_best_node
  *   return the best node the given setup fits in; a node is the best-fit 
  *   if the new setup (almost) saturates all its idle GPUs
  *
  *   Input:  const Setup&    setup to be allocated in an opened node
  *
  *   Outpu:  unsigned        index of the best opened node
  */
  unsigned select_best_node (const Setup&);

  /* assign_to_existing_node:
  *   assign the given job to an already opened node
  *
  *   Input:  const Job&                      job to be assigned
  *           setup_time_t::const_iterator    iterator to the best setup
  *           job_schedule_t&                 new proposed schedule
  *
  *   Output: bool                            true if job has been assigned
  */
  virtual bool assign_to_existing_node (const Job&, 
                                        setup_time_t::const_iterator,
                                        job_schedule_t&);
  
  /* assign_to_new_node:
  *   assign the given job to a new node
  *
  *   Input:  const Job&                      job to be assigned
  *           setup_time_t::const_iterator    iterator to the best setup
  *           job_schedule_t&                 new proposed schedule
  */
  virtual void assign_to_new_node (const Job&, 
                                   setup_time_t::const_iterator,
                                   job_schedule_t&);

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
                                     Dstar&, job_schedule_t&);

  /* select_largest_setup
  *   select a setup with the given GPUtype and nGPUs, but the maximum 
  *   possible maxnGPUs
  *
  *   Input:  const Job&                      job to be assigned
  *           unsigned                        number of required GPUs
  *           const std::string&              required GPU type
  *
  *   Output: setup_time_t::const_iterator    iterator to the largest setup
  */
  setup_time_t::const_iterator select_largest_setup (const Job&, unsigned,
                                                     const std::string&) const;

  /* select_setup
  *   select a setup with the given VMtype, GPUtype and nGPUs
  *
  *   Input:  const Job&                      job to be assigned
  *           const std::string&              required VM type
  *           const std::string&              required GPU type
  *           unsigned                        number of required GPUs
  *
  *   Output: setup_time_t::const_iterator    iterator to the new setup
  */
  setup_time_t::const_iterator select_setup (const Job&, const std::string&,
                                             const std::string&, 
                                             unsigned) const;

  // selects from a range of setups with VMtype and GPUtype equal to the 
  // configuration in the given Node the one with required maxnGPUs and
  // nGPUs
  setup_time_t::const_iterator select_setup_in_range (range_t, const Node&, 
                                                      unsigned) const;

  /* change_nodes_configurations 
  *   loop over the opened nodes and change configuration of those with
  *   idle GPUs
  *
  *   Input:  const job_schedule_t&           proposed schedule
  */
  void change_nodes_configurations (const job_schedule_t&);

  /* perform_assignment 
  *   assign the selected job to the new proposed schedule
  *
  *   Input:  const Job&                      job to be assigned
  *           job_schedule_t&                 new proposed schedule
  */
  virtual void perform_assignment (const Job&, job_schedule_t&) override;

  /* scheduling_step
  *   sort the list of submitted jobs and perform scheduling of all
  *   submitted jobs
  *
  *   Input:    job_schedule_t&     empty schedule to be built
  */
  virtual void scheduling_step (job_schedule_t&) override;

  /* postprocessing
  *   aim to reduce the number of idle GPUs in selected configurations
  *
  *   Input:    job_schedule_t&         proposed schedule to be updated
  */
  void postprocessing (job_schedule_t&);

public:
  /*  constructor
  *
  *   Input:  const std::string&    full path of data directory
  *           const std::string&    name of file with the list of jobs
  *           const std::string&    name of file with execution times of jobs
  *           const std::string&    name of file with the list of nodes
  *
  */
  Greedy (const std::string&, const std::string&, const std::string&, 
          const std::string&);
  
  /* destructor
  */
  virtual ~Greedy (void) = default;

};

#endif /* GREEDY_HH */
