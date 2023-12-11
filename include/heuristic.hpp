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

#ifndef HEURISTIC_HH
#define HEURISTIC_HH

#include "fileIO.hpp"
#include "schedule.hpp"
#include "timing.hpp"

#include <random>

class Heuristic {

protected:
  // full path of data (and results) directory
  std::string directory = "";

  const double scheduling_interval = INF;

  double current_time = 0.;

  // list of all jobs
  std::list<Job> jobs;
  // list of all nodes
  std::vector<Node> nodes;
  // table tjvg for all j (see utilities.hpp for type definition)
  time_table_t ttime;

  // list of submitted jobs
  std::list<Job> submitted_jobs;
  // list of scheduled jobs
  std::vector<job_schedule_t> scheduled_jobs;

  // index of last used node
  unsigned last_node_idx = 0;

  // total costs and total tardiness
  double total_vm_cost = 0.0;
  double total_tardi_cost = 0.0;
  double total_tardi = 0.0;

  // verbosity level - 0 for minimum, 2 for maximum
  unsigned verbose = 0;
  unsigned level = 0;

  // ONLY FOR RANDOMIZED HEURISTC METHODS
  unsigned max_random_iter = 0;           // maximum # of random iterations
  std::default_random_engine generator;   // generator

  /* prepare_logging
  */
  std::string prepare_logging (void) const;

  /* submit_job
  *   add a new job to the queue according to its submission time
  *
  *   Input:  double    elapsed time between previous and current iteration
  *
  *   Output: double    submission time of new job
  */
  double submit_job (double);

  /* remove_ended_jobs
  *   remove from the queue all jobs that have already been executed
  */
  virtual void remove_ended_jobs (void);

  /* sort_jobs_list
  */
  virtual void sort_jobs_list (void) = 0;

  /* close_nodes
  *   close all the currently opened nodes
  */
  virtual void close_nodes (void);

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
                                     const std::vector<Node>&) = 0;

  /* update_execution_times
  *   update execution time of jobs that have been partially executed
  *
  *   Input:  unsigned    iteration number
  *           double      elapsed time between previous and current iteration
  */
  void update_execution_times (unsigned, double);

  /* update_minMax_exec_time
  *   compute and update minimum and maximum execution time of the given job
  */
  void update_minMax_exec_time (Job&) const;

  /* update_scheduled_jobs
  *   update list of scheduled jobs with information about execution time,
  *   costs, etc
  *
  *   Input:  unsigned    iteration number
  *           double      elapsed time between previous and current iteration
  *
  *   Output: bool        true if completion percent of all jobs is 100
  */
  bool update_scheduled_jobs (unsigned, double);

  /* find_first_finish_time
  *   return the execution time of the first ending job in the given schedule
  */
  double find_first_finish_time (const job_schedule_t&) const;

  /* perform_assigment
  *   assign the selected job to the new proposed schedule
  *
  *   Input:  const Job&                      job to be assigned
  *           job_schedule_t&                 new proposed schedule
  */
  virtual void perform_assignment (const Job&, job_schedule_t&) = 0;

  /* perform_scheduling
  *   perform the scheduling process that assigns the best configuration to
  *   each submitted jobs (as long as resources are available)
  *
  *   Output: job_schedule_t        proposed schedule
  */
  virtual job_schedule_t perform_scheduling (void);

  /* available_resources
  *   return true if resources are still available in the system
  */
  bool available_resources (void) const;

  /* preprocessing
  *   update execution time and pressure of all submitted jobs, then sort the 
  *   queue as prescribed by the employed method
  */
  virtual void preprocessing (void);

  /* scheduling_step
  *   sort the list of submitted jobs and perform scheduling of all
  *   submitted jobs
  *
  *   Input:    job_schedule_t&     empty schedule to be built
  */
  virtual void scheduling_step (job_schedule_t&) = 0;


public:
  /*  constructor
  *
  *   Input:  const std::string&    full path of data directory
  *           const std::string&    name of file with the list of jobs
  *           const std::string&    name of file with execution times of jobs
  *           const std::string&    name of file with the list of nodes
  *
  */
  Heuristic (const std::string&, const std::string&, const std::string&, 
             const std::string&);
  
  /* destructor
  */
  virtual ~Heuristic (void) = default;

  /* algorithm
  *
  *   Input:  unsigned      verbosity level
  */
  void algorithm (unsigned);

  /* algorithm (overload - for eventual randomized methods)
  *
  *   Input:  unsigned      verbosity level
  *           unsigned      seed for random numbers generator
  *           unsigned      maximum number of random iterations
  */
  void algorithm (unsigned, unsigned, unsigned);
  
  /* print_data
  *   print data about jobs, nodes and execution times used by the algorithm
  *   using functions from fileIO.hpp (see for further information)
  */
  void print_data (void) const;

  /* print_schedule
  *   print the schedule produced by the algorithm using the function
  *   print_result from fileIO.hpp (see for further information)
  *
  *   Input:    const std::string&    filename where the schedule should be
  *                                   printed
  */
  void print_schedule (const std::string&) const;

};

#endif /* HEURISTIC_HH */
