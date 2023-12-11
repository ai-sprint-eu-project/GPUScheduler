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

#ifndef LOCAL_SEARCH_HH
#define LOCAL_SEARCH_HH

#include "random_greedy.hpp"

class Local_search: public Random_greedy {

protected:
  // type definitions
  typedef std::list<job_schedule_t::const_iterator> sorted_jobs_t;
  typedef std::vector<std::list<job_schedule_t::const_iterator>> nodes_map_t;
  typedef std::pair<double,job_schedule_t> best_change_t;

  // maximum number of best solutions to be saved
  unsigned K = 10;

  // maximum size of neighborhoods
  unsigned k1 = 10;

  // number of iterations of local search (1 corresponds to a first-improving
  // strategy)
  unsigned max_ls_iter = 10;

  /* compare_costs
  *  compare the costs passed as parameters
  *
  *  Input:   double                total cost
  *           double                new proposed cost
  *           double                current optimal cost
  *
  *  Output:  bool                  true if the new proposed cost is the best
  *                                 among the three costs
  *
  */
  bool compare_costs (double, double, double) const;

  /* perform_scheduling
  *   performs the scheduling process that assigns the best configuration to
  *   each submitted jobs (as long as resources are available)
  *
  *   Output: job_schedule_t        proposed schedule
  */
  virtual job_schedule_t perform_scheduling (void) override;

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

  /* restore_map_size
  *  erase the element in the map with the worst cost (the first element), so 
  *  that the size of the map returns less than or equal to the maximum size K
  *
  *  Input:     K_best_t&           map to be modified
  */
  virtual void restore_map_size (K_best_t&) override;

  /* to_be_inserted
  *  return true if the element characterized by the given upper bound should
  *  be inserted in the map
  */
  virtual bool to_be_inserted (const K_best_t&, K_best_t::iterator) const override;

  /* get_sorted_jobs
  *   analyses the solution passed as parameter to generate the list of 
  *   jobs sorted by decreasing tardiness, the list of jobs sorted by 
  *   decreasing VM cost and a table storing the list of jobs running on 
  *   each node
  *
  *   Input:  const Best_solution&            solution to be examined
  *           sorted_jobs_t&                  list of jobs sorted by tardiness
  *           sorted_jobs_t&                  list of jobs sorted by cost
  *           std::vector<sorted_jobs_t>&     map of jobs per node
  */
  void get_sorted_jobs (const Best_solution&, sorted_jobs_t&,
                        sorted_jobs_t&, std::vector<sorted_jobs_t>&) const;

  /* local_search
  *   perform the local search, updating the map of best solutions with the
  *   new candidates
  */
  void local_search (void);

  /* first_neighborhood
  *   jobs executed on different nodes, high tardiness VS high vm cost
  *
  *   Input:  const sorted_jobs_t&            list of jobs sorted by tardi
  *           const sorted_jobs_t&            list of jobs sorted by cost
  *           const Best_solution&            original best solution
  *           double                          original total cost
  *
  *   Output: best_change_t                   proposed modification with new 
  *                                           total cost
  */
  best_change_t first_neighborhood (const sorted_jobs_t&, const sorted_jobs_t&, 
                                    const Best_solution&, double);

  /* second_neighborhood
  *   jobs executed on different nodes, high tardiness VS low pressure
  *
  *   Input:  const sorted_jobs_t&            list of jobs sorted by tardi
  *           const Best_solution&            original best solution
  *           double                          original total cost
  *
  *   Output: best_change_t                   proposed modification with new 
  *                                           total cost
  */
  best_change_t second_neighborhood (const sorted_jobs_t&,const Best_solution&,
                                     double);

  /* third_neighborhood
  *   executed job (high tardiness) VS postponed job (high pressure)
  *
  *   Input:  const Best_solution&            original best solution
  *           double                          original total cost
  *
  *   Output: best_change_t                   proposed modification with new 
  *                                           total cost
  */
  best_change_t third_neighborhood (const Best_solution&, double);

  /* fourth_neighborhood
  *   nodes with jobs with highest tardiness --> more powerful setup
  *
  *   Input:  const sorted_jobs_t&            list of jobs sorted by tardiness
  *           const job_schedule_t&           original schedule
  *           const std::vector<Node>&        vector of open nodes (to be 
  *                                           modified)
  *           double                          original total cost
  *           std::vector<sorted_jobs_t>&     map of jobs per node
  *
  *   Output: best_change_t                   proposed modification with new 
  *                                           total cost
  */
  best_change_t fourth_neighborhood (const sorted_jobs_t&,const job_schedule_t&,
                                     std::vector<Node>&, double,
                                     const std::vector<sorted_jobs_t>&);

  /* fifth_neighborhood
  *   nodes with jobs with highest tardiness --> double number of GPUs
  *
  *   Input:  const sorted_jobs_t&            list of jobs sorted by tardiness
  *           const job_schedule_t&           original schedule
  *           const std::vector<Node>&        vector of open nodes (to be 
  *                                           modified)
  *           double                          original total cost
  *           std::vector<sorted_jobs_t>&     map of jobs per node
  *
  *   Output: best_change_t                   proposed modification with new 
  *                                           total cost
  */
  best_change_t fifth_neighborhood (const sorted_jobs_t&,const job_schedule_t&, 
                                    std::vector<Node>&, double,
                                    const std::vector<sorted_jobs_t>&);

  /* sixth_neighborhood
  *   nodes with jobs with highest tardiness --> half number of GPUs
  *
  *   Input:  const sorted_jobs_t&            list of jobs sorted by tardiness
  *           const job_schedule_t&           original schedule
  *           const std::vector<Node>&        vector of open nodes (to be 
  *                                           modified)
  *           double                          original total cost
  *           std::vector<sorted_jobs_t>&     map of jobs per node
  *
  *   Output: best_change_t                   proposed modification with new 
  *                                           total cost
  */
  best_change_t sixth_neighborhood (const sorted_jobs_t&,const job_schedule_t&, 
                                    std::vector<Node>&, double,
                                    const std::vector<sorted_jobs_t>&);

  /* seventh_neighborhood
  *
  *
  *   Input:  const sorted_jobs_t&            list of jobs sorted by tardiness
  *           const job_schedule_t&           original schedule
  *           const std::vector<Node>&        vector of open nodes (to be 
  *                                           modified)
  *           double                          original total cost
  *           std::vector<sorted_jobs_t>&     map of jobs per node
  *
  *   Output: best_change_t                   proposed modification with new 
  *                                           total cost
  */
  best_change_t seventh_neighborhood (const sorted_jobs_t&,
                                      const job_schedule_t&, 
                                      std::vector<Node>&, 
                                      double,
                                      const std::vector<sorted_jobs_t>&);

  /* swap_running_jobs
  *   swap schedules of jobs passed as parameter and evaluate the modified 
  *   cost, updating the best change in case of better cost
  *
  *   Input:  const Job&                        first job to be swapped
  *           const Job&                        second job to be swapped
  *           const Schedule&                   schedule of first job
  *           const Schedule&                   schedule of second job
  *           const job_schedule_t&             original schedule
  *           const std::vector<Node>&          original vector of open nodes
  *           double                            original total cost
  *           best_change_t&                    best change (to be updated)
  *
  *   Output: bool                              true if best change has been
  *                                             updated
  */
  bool swap_running_jobs (const Job&, const Job&, const Schedule&, 
                          const Schedule&, const job_schedule_t&, 
                          const std::vector<Node>&, double, best_change_t&);

  /* cfr_costs
  *   compute the cost of the original and modified schedule, updating the best 
  *   change in case of better cost
  *
  *   Input:  double                            original total cost
  *           job_schedule_t&                   original schedule
  *           job_schedule_t&                   modified schedule
  *           const std::vector<Node>&          modified vector of open nodes
  *           best_change_t&                    best change (to be updated)
  *
  *   Output: bool                              true if best change has been
  *                                             updated
  */
  bool cfr_costs (double, const job_schedule_t&, job_schedule_t&,
                  const std::vector<Node>&, best_change_t&);

public:
  /*  constructor
  *
  *   Input:  const std::string&    full path of data directory
  *           const std::string&    name of file with the list of jobs
  *           const std::string&    name of file with execution times of jobs
  *           const std::string&    name of file with the list of nodes
  *
  */
  Local_search (const std::string&, const std::string&, const std::string&, 
                const std::string&);

  /* destructor
  */
  virtual ~Local_search (void) = default;

};

#endif /* LOCAL_SEARCH_HH */