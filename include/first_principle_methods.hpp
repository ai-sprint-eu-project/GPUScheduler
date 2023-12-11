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

#ifndef FIRST_PRINCIPLE_METHODS_HH
#define FIRST_PRINCIPLE_METHODS_HH

#include "heuristic.hpp"
#include "dstar.hpp"
#include <unordered_map>
#include <unordered_set>
#include <map>

class First_principle_methods: public Heuristic {

protected:
  // type definitions
  typedef std::pair<setup_time_t::const_iterator,
                    setup_time_t::const_iterator> range_t;
  typedef std::pair<setup_time_t::const_iterator,bool> node_info;

  std::vector<node_info> nodes_info;
  unsigned nodes_in_use = 0;

  double TotalCost = 0;

  bool is_active(unsigned) const;

  void set_active(unsigned, bool);

  /* Custom comparator between jobs to order the jobs' queue.
  *   Pure virtual because it is intended to be defined in the specific heuristic versions
  */
  unsigned get_job_index(const Job&) const;

  /* Returns the best setup, no matter the cost */

  //setup_time_t::const_iterator get_best_time_setup(const setup_time_t& tjvg, setup_time_t::const_iterator & best_stp) const;

  /* compare_configuration
  *   returns true if the given Setup and Node have the same configuration
  *   (same VMtype and GPUtype)
  */
  virtual bool compare_configuration (const Setup&, const Node&) const;

  /* objective_function
  *   evaluate objective function of the current schedule
  *
  *   Input:  job_schedule_t&     new proposed schedule
  *           double              elapsed time between previous and current
  *                               iteration
  *
  *   Output: double              total cost of the proposed schedule
  */
  virtual double objective_function (job_schedule_t&, double,
                                     const std::vector<Node>&) override;

  /* assign_to_existing_node:
  *   assigns the given job to an already opened node, only if it has been closed before, in order to avoid
  *   having multiple jobs on a single node.
  *
  *   Input:  const Job&                      job to be assigned
  *           setup_time_t::const_iterator    iterator to the best setup
  *           job_schedule_t&                 new proposed schedule
  *
  *   Output: bool                            true if job has been assigned
  */

  bool is_in_execution(const Job&) const;

  /* renew_or_assign:
  *   if the job was already in execution, the function renews its assignment to the same node,
  *   otherwise, if it finds room, it opens a new node for the new job.
  *
  *   Input:  const Job&                      job to be renewed or assigned
  *           setup_time_t::const_iterator    iterator to the best setup
  *           job_schedule_t&                 new proposed schedule
  */
  virtual void renew_or_assign (const Job&,
                                   setup_time_t::const_iterator,
                                   job_schedule_t&);


  // selects from a range of setups with VMtype and GPUtype equal to the
  // configuration in the given Node the one with required maxnGPUs and
  // nGPUs
  setup_time_t::const_iterator select_setup_in_range (range_t, const Node&,
                                                      unsigned) const;

  /* change_nodes_configurations
  *   loops over the opened nodes and changes configuration of those with
  *   idle GPUs
  *
  *   Input:  const job_schedule_t&           proposed schedule
  */
  void change_nodes_configurations (const job_schedule_t&);

  /* perform_assignment
  *   assigns the selected job to the new proposed schedule
  *
  *   Input:  const Job&                      job to be assigned
  *           job_schedule_t&                 new proposed schedule
  */
  virtual void perform_assignment (const Job&, job_schedule_t&) override;

  /* scheduling_step
  *   sorts the list of submitted jobs and performs scheduling of all
  *   submitted jobs
  *
  *   Input:    job_schedule_t&     empty schedule to be built
  */
  virtual void scheduling_step (job_schedule_t&) override;

  /* close_nodes
  *   close all the currently opened nodes which are not active
  */
  virtual void close_nodes (void) override;

  /* remove_ended_jobs
  *   remove from the queue all jobs that have already been executed AND
  *   update at every time step the active nodes.
  *
  */
  virtual void remove_ended_jobs (void) override;

  /* sort_jobs_list
  *   sort the submitted jobs according to compare function
  */
  virtual void sort_jobs_list (void) override;

  /* compare
  * true if j1 before j2 with the considered heuristic
  */
  virtual bool compare (const Job& j1, const Job& j2) = 0;

public:
  /*  constructor
  *
  *   Input:  const std::string&    full path of data directory
  *           const std::string&    name of file with the list of jobs
  *           const std::string&    name of file with execution times of jobs
  *           const std::string&    name of file with the list of nodes
  *
  */
  First_principle_methods (const std::string&, const std::string&, 
                           const std::string&, const std::string&);

  virtual ~First_principle_methods (void) = default;
};

#endif /* FIRST_PRINCIPLE_METHODS_HH */