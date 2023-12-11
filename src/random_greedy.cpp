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

#include "random_greedy.hpp"

Random_greedy::Random_greedy (const std::string& directory, 
                              const std::string& file_jobs, 
                              const std::string& file_times, 
                              const std::string& file_nodes):
  Greedy(directory, file_jobs, file_times, file_nodes)
{}

void
Random_greedy::random_swap (void)
{
  // random swap
  std::bernoulli_distribution distribution; // TODO: set p
  std::list<Job>::iterator it = submitted_jobs.begin();
  for (unsigned idx = 0; idx < submitted_jobs.size(); ++idx)
  {
    std::list<Job>::iterator current_it = it;
    it++;
    double w1 = current_it->get_tardinessWeight();
    double w2 = it->get_tardinessWeight();
    double p = (w1 > w2) ? (0.5 + pi) : (0.5 - pi);
    distribution.param(std::bernoulli_distribution::param_type(p));
    bool swap = distribution(generator);
    if (it != submitted_jobs.end() && swap)
      std::swap(*current_it, *it);
  }

  if (verbose > 1)
  {
    std::string base = prepare_logging();
    std::cout << base << "\tlist of jobs after swap: ";
    for (const Job& j : submitted_jobs)
      std::cout << j.get_ID() << "; ";
    std::cout << std::endl;
  }
}

unsigned
Random_greedy::select_best_node (const Setup& best_stp)
{
  // map of opened nodes, sorted by the level of saturation
  std::multimap<unsigned, unsigned> sorted_nodes;
  unsigned best_idx = last_node_idx;

  for (unsigned idx = 0; idx < last_node_idx; ++idx)
  {
    const Node& node = nodes[idx];

    // compute the difference between the number of idle GPUs on the 
    // current node and the number of required GPUs
    int current_diff = node.get_remainingGPUs() - best_stp.get_nGPUs();

    // if the best setup fits the current node, insert the pair in the map
    if (compare_configuration(best_stp, node) && current_diff >= 0)
      sorted_nodes.insert({current_diff, idx});
  }

  // select the best index
  if (sorted_nodes.size() > 0)
    best_idx = random_select(sorted_nodes, generator, beta);

  return best_idx;
}

bool
Random_greedy::assign_to_suboptimal (const Job& j, const setup_time_t& tjvg,
                                     Dstar& dstar, 
                                     job_schedule_t& new_schedule)
{
  std::string base = "";
  bool assigned = false;
  while (!dstar.is_end() && !assigned)
  {
    dstar.set_generator(generator);
    setup_time_t::const_iterator best_stp = dstar.get_best_setup();
    generator = dstar.get_generator();

    if (verbose > 1)
    {
      base = prepare_logging();
      std::cout << base << "\tnew best setup: (" 
                << (best_stp->first).get_VMtype() << ", " 
                << (best_stp->first).get_maxnGPUs() << ")" << std::endl;
    }

    assigned = assign_to_existing_node(j, best_stp, new_schedule);

    if (verbose > 1 && assigned)
      std::cout << base << "\t\t---> ASSIGNED" << std::endl;
  }
  return assigned;
}

void
Random_greedy::perform_assignment (const Job& j, job_schedule_t& new_schedule)
{
  std::string base = "";

  // determine the setups s.t. deadline is matched
  const setup_time_t& tjvg = ttime.at(j.get_ID());
  Dstar dstar(j, tjvg, current_time);
  dstar.set_random_parameter(alpha);

  // determine the best setup:
  //   if it is possible to match deadline, the best setup is the 
  //   cheapest; otherwise, the best setup is the fastest
  dstar.set_generator(generator);
  setup_time_t::const_iterator best_stp = dstar.get_best_setup();
  generator = dstar.get_generator();

  if (verbose > 1)
  {
    base = prepare_logging();
    std::cout << base << "\tfirst selected setup: (" 
              << (best_stp->first).get_VMtype() << ", " 
              << (best_stp->first).get_maxnGPUs() << ")" << std::endl;
  }

  // check the already opened nodes...
  bool assigned = assign_to_existing_node(j, best_stp, new_schedule);

  // if the already opened nodes are not suitable...
  if (! assigned)
  {
    // if it is possible, open a new node with the optimal configuration
    // else, assign to an available suboptimal configuration
    if (last_node_idx < nodes.size())
    { 
      assigned = true;
      assign_to_new_node(j, best_stp, new_schedule);
    }
    else
      assigned = assign_to_suboptimal(j, tjvg, dstar, new_schedule);
  }
  else
    if (verbose > 1)
      std::cout << base << "\t\t---> ASSIGNED" << std::endl;

  // if job j cannot be assigned to any configuration, add an empty 
  // schedule
  if (!assigned)
    new_schedule[j] = Schedule(); 
}

job_schedule_t 
Random_greedy::perform_scheduling (void)
{
  K_best.clear();

  // (STEP #0)
  preprocessing();

  // initialization of minimum total cost, best schedule and corresponding
  // index by a step of pure greedy
  job_schedule_t full_greedy_schedule;
  Greedy::scheduling_step(full_greedy_schedule);
  update_best_schedule(full_greedy_schedule);

  #ifdef SMALL_SYSTEM
  // print full greedy cost
    std::cout << "\n## cost of full greedy schedule = " << K_best.cbegin()->first
              << "\n" << std::endl;
  #endif

  // random iterations
  for (unsigned random_iter = 1; random_iter < max_random_iter; 
       ++random_iter)
  {
    // reduce verbosity level to 1 at most
    unsigned original_verbosity_level = verbose;
    verbose = (verbose > 1) ? 1 : verbose;
    
    // determine new schedule
    job_schedule_t new_schedule;
    scheduling_step(new_schedule);

    // restore verbosity level
    if (verbose < original_verbosity_level)
      verbose = original_verbosity_level;

    // update best schedule
    update_best_schedule(new_schedule);
  }

  #ifdef SMALL_SYSTEM
    // print minimum cost after random iterations
    std::cout << "\n## best cost = " << K_best.cbegin()->first << std::endl;
  #endif

  // get best solution
  Best_solution& BS = K_best.begin()->second;
  job_schedule_t BS_sch = BS.get_schedule();
  std::swap(nodes,BS.get_open_nodes());
  last_node_idx = BS.get_last_node_idx();

  return BS_sch;
}

void
Random_greedy::scheduling_step (job_schedule_t& new_schedule)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- scheduling step." << std::endl;
  }
  
  // sort and swap list of submitted jobs
  if (r_swap)
  {
    sort_jobs_list();
    random_swap();
  }
  
  // close currently open nodes
  close_nodes();

  // perform assignment of all submitted jobs (STEP #1) until resources are
  // available
  std::string queue = "";
  bool resources = true;
  for (const Job& j : submitted_jobs)
  {
    queue += (j.get_ID() + "; ");
    if (verbose > 1)
      std::cout << base << "\tanalyzing job..." << j.get_ID() << std::endl;
    if (resources)
    {
      perform_assignment(j, new_schedule);
      resources = available_resources();  
    }
    else
      new_schedule[j] = Schedule();
  }

  if (verbose > 0)
    std::cout << base << "\tn_submitted_jobs: " << submitted_jobs.size()
              << "; queue: " << queue
              << "; n_used_nodes: " << last_node_idx << std::endl;

  // perform postprocessing (STEP #2)
  postprocessing(new_schedule);
}

void
Random_greedy::restore_map_size (K_best_t& current_K_best)
{
  K_best_t::iterator it = current_K_best.end();
  current_K_best.erase(--it);
}

bool
Random_greedy::to_be_inserted (const K_best_t& Kmap, K_best_t::iterator ub) const
{
  return (Kmap.empty() || ub != Kmap.cend());
}

void
Random_greedy::update_best_schedule (job_schedule_t& new_schedule)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- update best schedule." << std::endl;
  }
  
  // find execution time of first ending job
  double first_finish_time = find_first_finish_time(new_schedule);

  // compute cost of current schedule
  double current_cost = objective_function(new_schedule, first_finish_time,
                                           nodes);

  if (verbose > 1)
  {
    if (! K_best.empty())
      std::cout << base <<"\tcurrent optimal cost: " << K_best.cbegin()->first 
                <<"; ";
    else
      std::cout << base << "\t";
    std::cout << base << "new proposed cost: " << current_cost << std::endl;
  }

  // check if current solution is one of the best schedules
  K_best_t::iterator ub = K_best.upper_bound(current_cost);
  if (to_be_inserted(K_best, ub))
  {
    Best_solution BS(new_schedule, nodes, last_node_idx, first_finish_time);
    K_best.insert(ub,std::pair<double,Best_solution>(current_cost,BS));

    if (verbose > 1)
      std::cout << base << "\t\t---> INSERTED";

    if (K_best.size() > K)
    {
      restore_map_size(K_best);

      if (verbose > 1)
        std::cout << base << " ---> REMOVED LAST ELEMENT";
    }
    if (verbose > 1)
      std::cout << std::endl;
  }
}

