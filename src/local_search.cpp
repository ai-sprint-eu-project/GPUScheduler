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

#include "local_search.hpp"

Local_search::Local_search (const std::string& directory, 
                            const std::string& file_jobs, 
                            const std::string& file_times, 
                            const std::string& file_nodes):
  Random_greedy(directory, file_jobs, file_times, file_nodes)
{}

bool
Local_search::compare_costs (double total_cost, double new_total_cost, 
                             double current_best_cost) const
{
  bool check1 = new_total_cost > total_cost;
  bool check2 = new_total_cost > current_best_cost;
  return (check1 && check2);
}

job_schedule_t
Local_search::perform_scheduling (void)
{
  K_best.clear();

  // (STEP #0)
  preprocessing();

  // initialize the pool of best solutions by a step of pure greedy
  job_schedule_t full_greedy_schedule;
  Greedy::scheduling_step(full_greedy_schedule);
  update_best_schedule(full_greedy_schedule);

  #ifdef SMALL_SYSTEM
  // print full greedy cost
    std::cout << "\n## cost of full greedy schedule = " << K_best.rbegin()->first
              << "; number of used nodes = " 
              << (K_best.rbegin()->second).get_last_node_idx() << std::endl;
  #endif

  // random iterations
  unsigned original_verbosity_level = verbose;
  for (unsigned random_iter = 1; random_iter < max_random_iter; 
       ++random_iter)
  {
    verbose = 0;

    // determine new schedule
    job_schedule_t new_schedule;
    scheduling_step(new_schedule);

    // update the pool of best solutions
    update_best_schedule(new_schedule);
  }

  #ifdef SMALL_SYSTEM
    // print minimum cost and number of used nodes after random iterations
    std::cout << "\n## best cost after random construction = " 
              << K_best.rbegin()->first << "; number of used nodes = " 
              << (K_best.rbegin()->second).get_last_node_idx() << std::endl;
    verbose = original_verbosity_level;
  #endif

  bool go_ahead = true;
  for (unsigned ls_iter = 0; ls_iter < max_ls_iter && go_ahead; ++ls_iter)
  {
    #ifdef SMALL_SYSTEM
      std::cout << "\n## number of elite solutions: " << K_best.size() 
                << std::endl;
    #endif
    local_search();
    go_ahead = (K_best.size() > 1);
  }

  if (verbose > 0)
    std::cout << "arrived to max iter? --> "<< (go_ahead ? "true":"false") 
              << "\n\n";

  Best_solution& BS = K_best.rbegin()->second;
  job_schedule_t BS_sch = BS.get_schedule();
  std::swap(nodes, BS.get_open_nodes());
  last_node_idx = BS.get_last_node_idx();

  postprocessing(BS_sch);

  #ifdef SMALL_SYSTEM
    // print minimum cost and number of used nodes after local search
    std::cout << "\n## best cost = " << K_best.rbegin()->first 
              << "; number of used nodes = " << last_node_idx << std::endl;
  #endif

  return BS_sch;
}

double
Local_search::objective_function (job_schedule_t& new_schedule, 
                                  double elapsed_time,
                                  const std::vector<Node>& opened_nodes)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- new proxy function." << std::endl;
  }
  
  double vmCost = 0.;
  double tardiCost = 0;
  double sim_time = current_time + elapsed_time;
  double time_gain = 0.;

  // loop over last schedule
  for (job_schedule_t::iterator it = new_schedule.begin();
       it != new_schedule.end(); ++it)
  {
    const Job& j = it->first;
    Schedule& sch = it->second;

    if (verbose > 1)
      std::cout << base << "\tanalyzing job... " << j.get_ID() << std::endl;

    // if schedule is not empty, compute cost of VM and tardiness cost
    if (! sch.isEmpty())
    {
      // set as execution time the total required time
      double finish_time = sch.get_selectedTime();
      sch.set_executionTime(finish_time);
      
      // get number of used GPUs on the current node
      unsigned node_idx = sch.get_node_idx();
      unsigned g = opened_nodes[node_idx].get_usedGPUs();

      // compute cost of VM
      sch.compute_vmCost(g);
      vmCost += time_gain / sch.get_vmCost();
      
      // compute tardiness
      double tardiness = std::max(current_time + finish_time - j.get_deadline(), 
                                  0.);
      sch.set_tardiness(tardiness);
      sch.compute_tardinessCost(j.get_tardinessWeight());
      tardiCost += sch.get_tardinessCost();

      // update time gain
      time_gain += j.get_maxExecTime() / (sch.get_vmCost() + sch.get_tardinessCost());

      if (verbose > 1)
        std::cout << base << "\t\texecution_time = " << elapsed_time
                  << "; node_idx = " << node_idx << "; used_GPUs = " << g
                  << "; time_gain = " << time_gain 
                  << "; vm_cost = " << sch.get_vmCost()
                  << "; tardiness = " << tardiness
                  << "; t_cost = " << sch.get_tardinessCost() << std::endl;
    }
  }

  if (verbose > 0)
    std::cout << base << "\tvmCost: " << vmCost << "; tardiCost: " << tardiCost
              << std::endl;

  return (time_gain);
}

void
Local_search::restore_map_size (K_best_t& current_K_best)
{
  current_K_best.erase(current_K_best.begin());
}

bool
Local_search::to_be_inserted (const K_best_t& Kmap, K_best_t::iterator ub) const
{
  return (ub == Kmap.cend());
}

void
Local_search::get_sorted_jobs (const Best_solution& BS, 
                               sorted_jobs_t& tardi_jobs,
                               sorted_jobs_t& expensive_jobs,
                               std::vector<sorted_jobs_t>& nodes_map) const
{
  using temp_map_t = std::multimap<double,job_schedule_t::const_iterator,
                                    std::greater<double>>;
  temp_map_t tardi_map;
  temp_map_t cost_map;

  const job_schedule_t& schedule = BS.get_schedule();
  for (job_schedule_t::const_iterator cit = schedule.cbegin();
       cit != schedule.cend(); ++cit)
  {
    if (! (cit->second).isEmpty())
    {
      double tardi = (cit->second).get_tardiness();
      double vm_cost = (cit->second).get_vmCost();
      if (tardi > 0.0)
        tardi_map.insert({tardi,cit});
      else
        cost_map.insert({vm_cost,cit});
      nodes_map[(cit->second).get_node_idx()].push_back(cit);
    }
  }

  unsigned n = (k1 < tardi_map.size()) ? k1 : tardi_map.size();
  temp_map_t::const_iterator cit = tardi_map.cbegin();
  for (unsigned j = 0; j < n; ++j)
  {
    tardi_jobs.push_back(cit->second);
    ++cit;
  }

  n = (k1 < cost_map.size()) ? k1 : cost_map.size();
  cit = cost_map.cbegin();
  for (unsigned j = 0; j < n; ++j)
  {
    expensive_jobs.push_back(cit->second);
    ++cit;
  }
}

void
Local_search::local_search (void)
{
  std::string base = "";
  if (verbose > 0)
  {
    std::cout << prepare_logging() << "--- local search." << std::endl;
    ++level;
    base = prepare_logging();
  }

  K_best_t K_best_copy(K_best);
  
  K_best_t new_K_best;
  new_K_best.insert(*K_best.rbegin());

  for (K_best_t::reverse_iterator k_best_it = K_best_copy.rbegin(); 
       k_best_it != K_best_copy.rend(); ++k_best_it)
  {
    // characteristics of the original solution
    double total_cost = k_best_it->first;
    const Best_solution& original_BS = k_best_it->second;
    unsigned original_lni = original_BS.get_last_node_idx();
    const std::vector<Node>& opened_nodes = original_BS.get_open_nodes();
    job_schedule_t original_schedule = original_BS.get_schedule();

    if (verbose > 1)
      std::cout << "\n" << base << "current optimal cost: " << total_cost 
                << std::endl;

    best_change_t best_change({-INF,job_schedule_t()});
    unsigned best_neigh_idx = 0;

    // list of jobs with highest tardiness
    sorted_jobs_t tardi_jobs;
    // list of most expensive jobs
    sorted_jobs_t expensive_jobs;
    // for each node, stores the list of jobs deployed on that node
    std::vector<sorted_jobs_t> nodes_map(original_lni);
    //
    get_sorted_jobs(original_BS, tardi_jobs, expensive_jobs, nodes_map);

    // for some neighborhoods, vector of open nodes must be modified
    std::vector<Node> best_opened_nodes(opened_nodes);

    // third neighborhood:    executed job (low pressure) VS
    //                        postponed job (high pressure)
    best_change_t best_change_third_neigh;
    best_change_third_neigh = third_neighborhood(original_BS, total_cost);
    if (verbose > 1)
      std::cout << base << "\tthird neighborhood proposes " 
                << best_change_third_neigh.first << std::endl;
    if (best_change_third_neigh.first > best_change.first)
    {
      std::swap(best_change_third_neigh, best_change);
      best_neigh_idx = 3;
      if (verbose > 1)
        std::cout << base << "\t\t---> CHANGED" << std::endl;
    }

    if (tardi_jobs.size() > 0)
    {     
      // if the proposed solution involves more than one node...
      if (original_lni > 2)
      {
        // first neighborhood:    jobs executed on different nodes, 
        //                        high tardiness VS high vm cost
        best_change_t best_change_first_neigh;
        best_change_first_neigh = first_neighborhood(tardi_jobs, expensive_jobs,
                                                     original_BS, total_cost);
        if (verbose > 1)
          std::cout << base << "\tfirst neighborhood proposes " 
                    << best_change_first_neigh.first << std::endl;
        if (best_change_first_neigh.first > best_change.first)
        {
          std::swap(best_change_first_neigh, best_change);
          best_neigh_idx = 1;
          if (verbose > 1)
            std::cout << base << "\t\t---> CHANGED" << std::endl;
        }

        // second neighborhood:   jobs executed on different nodes, 
        //                        high tardiness VS low pressure
        best_change_t best_change_second_neigh;
        best_change_second_neigh = second_neighborhood(tardi_jobs, original_BS,
                                                       total_cost);
        if (verbose > 1)
          std::cout << base << "\tsecond neighborhood proposes " 
                    << best_change_second_neigh.first << std::endl;
        if (best_change_second_neigh.first > best_change.first)
        {
          std::swap(best_change_second_neigh, best_change);
          best_neigh_idx = 2;
          if (verbose > 1)
            std::cout << base << "\t\t---> CHANGED" << std::endl;
        }
      }

      // fourth neighborhood:   nodes with the highest number of jobs in tardiness
      //                        more powerful setup (same number of GPUs)
      std::vector<Node> fourth_opened_nodes(opened_nodes);
      best_change_t best_change_fourth_neigh;
      best_change_fourth_neigh = fourth_neighborhood(tardi_jobs,
                                                     original_schedule, 
                                                     fourth_opened_nodes, 
                                                     total_cost, nodes_map);
      if (verbose > 1)
          std::cout << base << "\tfourth neighborhood proposes " 
                    << best_change_fourth_neigh.first << std::endl;
      if (best_change_fourth_neigh.first > best_change.first)
      {
        std::swap(best_change_fourth_neigh, best_change);
        std::swap(best_opened_nodes, fourth_opened_nodes);
        best_neigh_idx = 4;
        if (verbose > 1)
            std::cout << base << "\t\t---> CHANGED" << std::endl;
      }

      // fifth neighborhood:    nodes with the highest number of jobs in tardiness
      //                        double number of GPUs
      std::vector<Node> fifth_opened_nodes(opened_nodes);
      best_change_t best_change_fifth_neigh;
      best_change_fifth_neigh = fifth_neighborhood(tardi_jobs,original_schedule, 
                                                   fifth_opened_nodes, total_cost, 
                                                   nodes_map);
      if (verbose > 1)
          std::cout << base << "\tfifth neighborhood proposes " 
                    << best_change_fifth_neigh.first << std::endl;
      if (best_change_fifth_neigh.first > best_change.first)
      {
        std::swap(best_change_fifth_neigh, best_change);
        std::swap(best_opened_nodes, fifth_opened_nodes);
        best_neigh_idx = 5;
        if (verbose > 1)
            std::cout << base << "\t\t---> CHANGED" << std::endl;
      }

      // sixth neighborhood:    nodes with the highest number of jobs in tardiness
      //                        half number of GPUs (if even)
      std::vector<Node> sixth_opened_nodes(opened_nodes);
      best_change_t best_change_sixth_neigh;
      best_change_sixth_neigh = sixth_neighborhood(tardi_jobs,
                                                   original_schedule, 
                                                   sixth_opened_nodes, total_cost, 
                                                   nodes_map);
      if (verbose > 1)
          std::cout << base << "\tsixth neighborhood proposes " 
                    << best_change_sixth_neigh.first << std::endl;
      if (best_change_sixth_neigh.first > best_change.first)
      {
        std::swap(best_change_sixth_neigh, best_change);
        std::swap(best_opened_nodes, sixth_opened_nodes);
        best_neigh_idx = 6;
        if (verbose > 1)
            std::cout << base << "\t\t---> CHANGED" << std::endl;
      }

      // seventh neighborhood:  nodes with the highest number of jobs in tardiness
      //                        change GPU sharing among jobs
      std::vector<Node> seventh_opened_nodes(opened_nodes);
      best_change_t best_change_seventh_neigh;
      best_change_seventh_neigh = seventh_neighborhood(tardi_jobs, 
                                                     original_schedule, 
                                                     seventh_opened_nodes, 
                                                     total_cost, nodes_map);
      if (verbose > 1)
          std::cout << base << "\tseventh neighborhood proposes " 
                    << best_change_seventh_neigh.first << std::endl;
      if (best_change_seventh_neigh.first > best_change.first)
      {
        std::swap(best_change_seventh_neigh, best_change);
        std::swap(best_opened_nodes, seventh_opened_nodes);
        best_neigh_idx = 7;
        if (verbose > 1)
            std::cout << base << "\t\t---> CHANGED" << std::endl;
      }
    }

    // if there exists a best solution
    if (best_neigh_idx > 0)
    {
      job_schedule_t best_schedule = best_change.second;
      //double best_cost = best_change.first;

      unsigned original_verbosity_level = verbose;
      verbose = (verbose > 0) ? 0 : verbose;
      ++level;
      double new_fft = find_first_finish_time(best_schedule);
      double best_cost = objective_function(best_schedule, new_fft, 
                                            best_opened_nodes);
      verbose = original_verbosity_level;
      --level;
      
      Best_solution new_BS(best_schedule, best_opened_nodes, original_lni, 
                           new_fft);

      //K_best.erase(total_cost);
      new_K_best.insert({best_cost, new_BS});

      if (verbose > 0)
        std::cout << base << "best neighborhood: " << best_neigh_idx
                  << std::endl;
    }
  }

  // check that size of new_K_best does not exceed K
  if (new_K_best.size() > K)
    restore_map_size(new_K_best);

  // swap new and old set of best solutions
  std::swap(K_best, new_K_best);

  if (verbose > 0)
    --level;
}

Local_search::best_change_t
Local_search::first_neighborhood (const sorted_jobs_t& jobs1,
                                  const sorted_jobs_t& jobs2, 
                                  const Best_solution& original_BS,
                                  double total_cost)
{
  std::string base = "";
  if (verbose > 1)
  {
    base = prepare_logging();
    std::cout << base << "\t--- first neighborhood." << std::endl;
  }

  // proposed modifications
  best_change_t best_change({-INF,job_schedule_t()});
  bool changed = false;

  // characteristics of original solution
  const job_schedule_t& original_schedule = original_BS.get_schedule();
  const std::vector<Node>& opened_nodes = original_BS.get_open_nodes();

  // loop over jobs in order of tardiness
  for(sorted_jobs_t::const_iterator cit1 = jobs1.cbegin(); 
      cit1 != jobs1.cend(); ++cit1)
  {
    // get current job and corresponding schedule
    const Job& j1 = (*cit1)->first;
    const Schedule& sch1 = (*cit1)->second;
    unsigned n1 = sch1.get_node_idx();

    // loop over jobs in order of vm cost
    for(sorted_jobs_t::const_iterator cit2 = jobs2.cbegin();
        cit2 != jobs2.cend(); ++cit2)
    {
      const Job& j2 = (*cit2)->first;
      const Schedule& sch2 = (*cit2)->second;
      unsigned n2 = sch2.get_node_idx();

      // jobs are different by construction; if they are running on different
      // nodes, swap configurations
      if (n1 != n2)
      {
        level+=3;
        changed = (changed || swap_running_jobs(j1, j2, sch1, sch2, 
                                                original_schedule, opened_nodes, 
                                                total_cost, best_change));
        level-=3;
      }
    }
  }

  if (changed && verbose > 2)
  {
    std::cout << base << "\t\t--> best choice of first neigh:\n";
    Schedule::print_names(std::cout);
    for (job_schedule_t::const_iterator fit = (best_change.second).cbegin();
         fit != (best_change.second).cend(); ++fit)
    {
      if (! (fit->second).isEmpty())
        (fit->second).print(fit->first, opened_nodes, std::cout);
    }
  }

  return best_change;
}

Local_search::best_change_t
Local_search::second_neighborhood (const sorted_jobs_t& jobs1,
                                   const Best_solution& original_BS,
                                   double total_cost)
{
  if (verbose > 1)
  {
    std::string base = prepare_logging();
    std::cout << base << "\t--- second neighborhood." << std::endl;
  }

  ++level;
  std::string internal_base = prepare_logging();

  // proposed modifications
  best_change_t best_change({-INF,job_schedule_t()});
  bool changed = false;

  // characteristics of original solution
  const job_schedule_t& original_schedule = original_BS.get_schedule();
  const std::vector<Node>& opened_nodes = original_BS.get_open_nodes();

  // loop over jobs in order of tardiness
  for(sorted_jobs_t::const_iterator cit1 = jobs1.cbegin(); 
      cit1 != jobs1.cend(); ++cit1)
  {
    // get current job and corresponding schedule
    const Job& j1 = (*cit1)->first;
    const Schedule& sch1 = (*cit1)->second;
    unsigned n1 = sch1.get_node_idx();

    if (verbose > 3)
      std::cout << internal_base << "\texamining job... " << j1.get_ID() 
                << ": (node_idx: " << n1 << ")" << std::endl;

    // loop over jobs in reverse order of pressure
    for(std::list<Job>::const_reverse_iterator cit2 = submitted_jobs.rbegin();
       cit2 != submitted_jobs.rend(); ++cit2)
    {
      const Job& j2 = *cit2;
      
      // if the two jobs are different and job j2 is running on a different
      // node...
      if (j1 != j2)
      {
        job_schedule_t::const_iterator sch2_it = original_schedule.find(j2);
        if(sch2_it != original_schedule.cend() && ! (sch2_it->second).isEmpty())
        {
          const Schedule& sch2 = sch2_it->second;
          unsigned n2 = sch2.get_node_idx();

          if (verbose > 3)
            std::cout << internal_base << "\t\texamining job... " <<j2.get_ID() 
                      << ": (node_idx: " << n2 << ")" << std::endl;

          // ...swap configurations
          if (n1 != n2)
          {
            level+=3;
            changed = (changed || swap_running_jobs(j1, j2, sch1, sch2, 
                                                    original_schedule, 
                                                    opened_nodes, total_cost, 
                                                    best_change));
            level-=3;
          }
        }
      }
    }
  }

  if (changed && verbose > 2)
  {
    std::cout << internal_base << "\t\t\t--> best choice of second neigh:\n";
    Schedule::print_names(std::cout);
    for (job_schedule_t::const_iterator fit = (best_change.second).cbegin();
         fit != (best_change.second).cend(); ++fit)
    {
      if (! (fit->second).isEmpty())
        (fit->second).print(fit->first, opened_nodes, std::cout);
    }
  }

  --level;

  return best_change;
}

Local_search::best_change_t
Local_search::third_neighborhood (const Best_solution& original_BS,
                                  double total_cost)
{
  std::string base = "";
  if (verbose > 1)
  {
    base = prepare_logging();
    std::cout << base << "\t--- third neighborhood." << std::endl;
  }

  // proposed modifications
  best_change_t best_change({-INF,job_schedule_t()});
  bool changed = false;

  // characteristics of original solution
  const job_schedule_t& original_schedule = original_BS.get_schedule();
  const std::vector<Node>& opened_nodes = original_BS.get_open_nodes();

  // loop over jobs in order of pressure
  for(std::list<Job>::const_iterator cit1 = submitted_jobs.cbegin();
      cit1 != submitted_jobs.cend(); ++cit1)
  {
    const Job& j1 = *cit1;
    job_schedule_t::const_iterator sch1_it = original_schedule.find(j1);
    
    // if job j1 has been postponed
    if(sch1_it != original_schedule.cend() && (sch1_it->second).isEmpty())
    {
      // loop over jobs in reverse order of pressure
      for(std::list<Job>::const_reverse_iterator cit2 = submitted_jobs.rbegin();
       cit2 != submitted_jobs.rend(); ++cit2)
      {
        const Job& j2 = *cit2;
        
        // if the two jobs are different and job j2 is running...
        if (j1 != j2 && j1.get_pressure() >= j2.get_pressure())
        {
          job_schedule_t::const_iterator sch2_it = original_schedule.find(j2);
          if(sch2_it != original_schedule.cend() && ! (sch2_it->second).isEmpty())
          {
            const Schedule& sch2 = sch2_it->second;
            unsigned n2 = sch2.get_node_idx();
            const Setup& stp2 = sch2.get_setup();

            // ...swap the state of the two jobs
            setup_time_t::const_iterator new_it1 = select_setup(j1, 
                                                            stp2.get_VMtype(),
                                                            stp2.get_GPUtype(),
                                                            stp2.get_nGPUs());

            job_schedule_t modified_schedule({{j1,Schedule(new_it1,n2)},
                                              {j2,Schedule()}});

            job_schedule_t new_schedule(original_schedule);

            // create full schedule with the modified jobs
            for (job_schedule_t::iterator change_it=modified_schedule.begin(); 
                 change_it != modified_schedule.end(); ++change_it)
            {
              const Job& change_j = change_it->first;
              job_schedule_t::iterator original_it = new_schedule.find(change_j);
              
              if (original_it != new_schedule.end())
                std::swap(original_it->second,change_it->second);
            }

            unsigned original_verbosity_level = verbose;
            verbose = (verbose > 0) ? 0 : verbose;
            level+=2;
            double new_fft = find_first_finish_time(new_schedule);
            double new_total_cost = objective_function(new_schedule, new_fft, 
                                                       opened_nodes);
            verbose = original_verbosity_level;
            level-=2;

            // save modification if it gives a better solution
            if (compare_costs(total_cost, new_total_cost, best_change.first))
            {
              changed = true;
              best_change.first = new_total_cost;
              std::swap(best_change.second, new_schedule);
            }
          }
        }
      }
    }
  }

  if (changed && verbose > 2)
  {
    std::cout << base << "\t\t--> best choice of third neigh:\n";
    Schedule::print_names(std::cout);
    for (job_schedule_t::const_iterator fit = (best_change.second).cbegin();
         fit != (best_change.second).cend(); ++fit)
    {
      if (! (fit->second).isEmpty())
        (fit->second).print(fit->first, opened_nodes, std::cout);
    }
  }

  return best_change;
}

Local_search::best_change_t
Local_search::fourth_neighborhood (const sorted_jobs_t& jobs1,
                                   const job_schedule_t& original_schedule,
                                   std::vector<Node>& opened_nodes,
                                   double total_cost,
                                   const std::vector<sorted_jobs_t>& nodes_map)
{
  std::string base = "";
  if (verbose > 1)
  {
    base = prepare_logging();
    std::cout << base << "\t--- fourth neighborhood." << std::endl;
  }

  // proposed modifications
  best_change_t best_change({-INF,job_schedule_t()});
  bool changed = false;

  // keep track of already visited nodes
  unsigned count = 0;
  std::vector<bool> visited_nodes(opened_nodes.size(),false);

  // loop over jobs in order of tardiness
  for(sorted_jobs_t::const_iterator cit1 = jobs1.cbegin(); 
      cit1 != jobs1.cend() && count < k1; ++cit1)
  {
    const Job& j1 = (*cit1)->first;
    const Schedule& sch1 = (*cit1)->second;
    unsigned n1_idx = sch1.get_node_idx();
    unsigned g1 = sch1.get_setup().get_nGPUs();

    if (! visited_nodes[n1_idx])
    {
      // get maximum number of GPUs on current node
      Node& n1 = opened_nodes[n1_idx];
      unsigned max_n_GPUs = n1.get_usedGPUs() + n1.get_remainingGPUs();
      Node original_n1(n1);

      // look for configurations with same number of GPUs and lowest execution
      // time
      const setup_time_t& tjvg = ttime.at(j1.get_ID());
      setup_time_t::const_iterator new_scit = tjvg.cend();
      for (setup_time_t::const_iterator scit = tjvg.cbegin();
           scit != tjvg.cend(); ++scit)
      {
        const Setup& stp = scit->first;
        if (stp != sch1.get_setup() && stp.get_nGPUs() == g1 && 
            stp.get_maxnGPUs() == max_n_GPUs)
        {
          // get characteristics of the new configuration
          const std::string& VMtype = stp.get_VMtype();
          const std::string& GPUtype = stp.get_GPUtype();

          // get list of jobs running on the current node
          const sorted_jobs_t& r_jobs = nodes_map[n1_idx];
          double original_total_time = 0.0;
          double new_total_time = 0.0;

          // loop over those jobs
          for (sorted_jobs_t::const_iterator r_jobs_it = r_jobs.cbegin();
               r_jobs_it != r_jobs.cend(); ++r_jobs_it)
          {
            const Job& current_job = (*r_jobs_it)->first;
            const Schedule& current_sch = (*r_jobs_it)->second;

            if (current_job != j1)
            {
              unsigned current_g = current_sch.get_setup().get_nGPUs();
              setup_time_t::const_iterator new_it = select_setup(current_job, 
                                                                 VMtype, 
                                                                 GPUtype,
                                                                 current_g);
              original_total_time += current_sch.get_selectedTime();
              new_total_time += new_it->second;
            }
            else
            {
              original_total_time += sch1.get_selectedTime();
              new_total_time += scit->second;
            }
          }

          if (new_total_time < original_total_time)
            new_scit = scit;
        }
      }

      // if such a configuration exists and it is not the same we had before
      if (new_scit != tjvg.cend() && new_scit->first != sch1.get_setup())
      {
        // change configuration on the current node
        n1.change_setup(new_scit->first);

        // instantiate new solution
        job_schedule_t modified_schedule;

        // get characteristics of the new configuration
        const std::string& VMtype = (new_scit->first).get_VMtype();
        const std::string& GPUtype = (new_scit->first).get_GPUtype();

        // get list of jobs running on the current node
        const sorted_jobs_t& r_jobs = nodes_map[n1_idx];

        // loop over those jobs
        for (sorted_jobs_t::const_iterator r_jobs_it = r_jobs.cbegin();
             r_jobs_it != r_jobs.cend(); ++r_jobs_it)
        {
          const Job& current_job = (*r_jobs_it)->first;
          unsigned current_g = ((*r_jobs_it)->second).get_setup().get_nGPUs();
          setup_time_t::const_iterator new_it = select_setup(current_job, 
                                                             VMtype, GPUtype,
                                                             current_g);
          n1.set_remainingGPUs(current_g);

          modified_schedule.insert({current_job,Schedule(new_it,n1_idx)});
        }

        // update the best modification if it has a better cost
        level+=3;
        bool now_changed = cfr_costs(total_cost, original_schedule, 
                                     modified_schedule, opened_nodes, 
                                     best_change);
        level-=3;

        // if the current modification has not been update, restore the status
        // of the current node
        if (! now_changed)
          std::swap(n1, original_n1);

        changed = (changed || now_changed);
      }

      visited_nodes[n1_idx] = true;
      ++count;
    }
  }

  if (changed && verbose > 2)
  {
    std::cout << base << "\t\t--> best choice of fourth neigh:\n";
    Schedule::print_names(std::cout);
    for (job_schedule_t::const_iterator fit = (best_change.second).cbegin();
         fit != (best_change.second).cend(); ++fit)
    {
      if (! (fit->second).isEmpty())
        (fit->second).print(fit->first, opened_nodes, std::cout);
    }
  }

  return best_change;
}

Local_search::best_change_t
Local_search::fifth_neighborhood (const sorted_jobs_t& jobs1,
                                  const job_schedule_t& original_schedule,
                                  std::vector<Node>& opened_nodes,
                                  double total_cost,
                                  const std::vector<sorted_jobs_t>& nodes_map)
{
  std::string base = "";
  if (verbose > 1)
  {
    base = prepare_logging();
    std::cout << base << "\t--- fifth neighborhood." << std::endl;
  }

  // proposed modifications
  best_change_t best_change({-INF,job_schedule_t()});
  bool changed = false;

  // keep track of already visited nodes
  unsigned count = 0;
  std::vector<bool> visited_nodes(opened_nodes.size(),false);

  // loop over jobs in order of tardiness
  for(sorted_jobs_t::const_iterator cit1 = jobs1.cbegin(); 
      cit1 != jobs1.cend() && count < k1; ++cit1)
  {
    const Job& j1 = (*cit1)->first;
    const Schedule& sch1 = (*cit1)->second;
    unsigned n1_idx = sch1.get_node_idx();
    unsigned g1 = sch1.get_setup().get_nGPUs();

    if (! visited_nodes[n1_idx])
    {
      // get maximum number of GPUs on current node
      Node& n1 = opened_nodes[n1_idx];
      unsigned max_n_GPUs = n1.get_usedGPUs() + n1.get_remainingGPUs();
      const std::string& GPUtype = n1.get_GPUtype();
      Node original_n1(n1);

      // look for configurations with same GPUtype and double number of GPUs
      const setup_time_t& tjvg = ttime.at(j1.get_ID());
      setup_time_t::const_iterator new_scit = tjvg.cend();
      bool found = false;
      for (setup_time_t::const_iterator scit = tjvg.cbegin();
           scit != tjvg.cend() && !found; ++scit)
      {
        const Setup& stp = scit->first;
        if (stp.get_nGPUs() == (g1*2) && stp.get_GPUtype() == GPUtype &&
            stp.get_maxnGPUs() == (max_n_GPUs*2))
        {
          new_scit = scit;
          found = true;
        }
      }

      // if a new configuration exists
      if (new_scit != tjvg.cend())
      {
        // change configuration on the current node
        n1.change_setup(new_scit->first);

        // instantiate new solution
        job_schedule_t modified_schedule;

        // get characteristics of the new configuration
        const std::string& VMtype = (new_scit->first).get_VMtype();

        // get list of jobs running on the current node
        const sorted_jobs_t& r_jobs = nodes_map[n1_idx];

        // loop over those jobs
        for (sorted_jobs_t::const_iterator r_jobs_it = r_jobs.cbegin();
             r_jobs_it != r_jobs.cend(); ++r_jobs_it)
        {
          const Job& current_job = (*r_jobs_it)->first;
          unsigned current_g = ((*r_jobs_it)->second).get_setup().get_nGPUs();
          setup_time_t::const_iterator new_it = select_setup(current_job, 
                                                             VMtype, GPUtype,
                                                             (2*current_g));
          n1.set_remainingGPUs(2*current_g);

          modified_schedule.insert({current_job,Schedule(new_it,n1_idx)});
        }

        // update the best modification if it has a better cost
        level+=3;
        bool now_changed = cfr_costs(total_cost, original_schedule, 
                                     modified_schedule, opened_nodes, 
                                     best_change);
        level-=3;

        // if the current modification has not been update, restore the status
        // of the current node
        if (! now_changed)
          std::swap(n1, original_n1);

        changed = (changed || now_changed);
      }

      visited_nodes[n1_idx] = true;
      ++count;
    }
  }

  if (changed && verbose > 2)
  {
    std::cout << base << "\t\t--> best choice of fifth neigh:\n";
    Schedule::print_names(std::cout);
    for (job_schedule_t::const_iterator fit = (best_change.second).cbegin();
         fit != (best_change.second).cend(); ++fit)
    {
      if (! (fit->second).isEmpty())
        (fit->second).print(fit->first, opened_nodes, std::cout);
    }
  }

  return best_change;
}

Local_search::best_change_t
Local_search::sixth_neighborhood (const sorted_jobs_t& jobs1,
                                  const job_schedule_t& original_schedule,
                                  std::vector<Node>& opened_nodes,
                                  double total_cost,
                                  const std::vector<sorted_jobs_t>& nodes_map)
{
  if (verbose > 1)
  {
    std::string base = prepare_logging();
    std::cout << base << "\t--- sixth neighborhood." << std::endl;
  }

  ++level;
  std::string internal_base = prepare_logging();

  // proposed modifications
  best_change_t best_change({-INF,job_schedule_t()});
  bool changed = false;

  // keep track of already visited nodes
  unsigned count = 0;
  std::vector<bool> visited_nodes(opened_nodes.size(),false);

  // loop over jobs in order of tardiness
  for(sorted_jobs_t::const_iterator cit1 = jobs1.cbegin(); 
      cit1 != jobs1.cend() && count < k1; ++cit1)
  {
    const Job& j1 = (*cit1)->first;
    const Schedule& sch1 = (*cit1)->second;
    unsigned n1_idx = sch1.get_node_idx();
    unsigned g1 = sch1.get_setup().get_nGPUs();

    if (verbose > 3)
      std::cout << internal_base << "\texamining job... " << j1.get_ID() 
                << ": (node_idx: " << n1_idx << ", nGPUs: " << g1 << ")" 
                << std::endl;

    if (! visited_nodes[n1_idx] && (g1%2==0))
    {
      // get maximum number of GPUs on current node
      Node& n1 = opened_nodes[n1_idx];
      unsigned max_n_GPUs = n1.get_usedGPUs() + n1.get_remainingGPUs();
      const std::string& GPUtype = n1.get_GPUtype();
      Node original_n1(n1);

      if (verbose > 3)
        std::cout << internal_base << "\t\texamining node... " << n1.get_ID() 
                  << ": (" << n1.get_VMtype() << ", " << GPUtype << ", "
                  << n1.get_usedGPUs() << ", " << n1.get_remainingGPUs()
                  << ")" << std::endl;

      // look for configurations with same GPUtype and half number of GPUs
      const setup_time_t& tjvg = ttime.at(j1.get_ID());
      setup_time_t::const_iterator new_scit = tjvg.cend();
      bool found = false;
      for (setup_time_t::const_iterator scit = tjvg.cbegin();
           scit != tjvg.cend() && !found; ++scit)
      {
        const Setup& stp = scit->first;
        if (stp.get_nGPUs() == (g1/2) && stp.get_GPUtype() == GPUtype &&
            stp.get_maxnGPUs() == (max_n_GPUs/2))
        {
          new_scit = scit;
          found = true;

          if (verbose > 3)
            std::cout << internal_base << "\t\t\tfound " << stp.get_nGPUs()
                      << " --> new_max_nGPUs = " << stp.get_maxnGPUs() 
                      << std::endl;
        }
      }

      // if a new configuration exists
      if (new_scit != tjvg.cend())
      {
        Node original_n1(n1);

        if (verbose > 3)
          n1.print_open_node(std::cout);

        // change configuration on the current node
        n1.change_setup(new_scit->first);

        if (verbose > 3)
          n1.print_open_node(std::cout);

        // instantiate new solution
        job_schedule_t modified_schedule;

        // get characteristics of the new configuration
        const std::string& VMtype = (new_scit->first).get_VMtype();

        // get list of jobs running on the current node
        const sorted_jobs_t& r_jobs = nodes_map[n1_idx];

        // loop over those jobs
        bool unfeasible = false;
        for (sorted_jobs_t::const_iterator r_jobs_it = r_jobs.cbegin();
             r_jobs_it != r_jobs.cend() && !unfeasible; ++r_jobs_it)
        {
          const Job& current_job = (*r_jobs_it)->first;
          unsigned current_g = ((*r_jobs_it)->second).get_setup().get_nGPUs();
          if (current_g % 2 == 0)
          {
            setup_time_t::const_iterator new_it = select_setup(current_job, 
                                                               VMtype, GPUtype,
                                                               (current_g/2));
            n1.set_remainingGPUs(current_g/2);
            modified_schedule.insert({current_job,Schedule(new_it,n1_idx)});
          }
          else
            unfeasible = true;
        }

        if (verbose > 3)
          n1.print_open_node(std::cout);

        if (! unfeasible)
        {
          // update the best modification if it has a better cost
          level+=3;
          bool now_changed = cfr_costs(total_cost, original_schedule, 
                                       modified_schedule, opened_nodes, 
                                       best_change);
          level-=3;

          // if the current modification has not been update, restore the 
          // status of the current node
          if (! now_changed)
            std::swap(n1, original_n1);
          
          if (verbose > 3)
            n1.print_open_node(std::cout);

          changed = (changed || now_changed);
        }
      }

      visited_nodes[n1_idx] = true;
      ++count;
    }
  }

  if (changed && verbose > 2)
  {
    std::cout << internal_base << "\t\t\t--> best choice of sixth neigh:\n";
    Schedule::print_names(std::cout);
    for (job_schedule_t::const_iterator fit = (best_change.second).cbegin();
         fit != (best_change.second).cend(); ++fit)
    {
      if (! (fit->second).isEmpty())
        (fit->second).print(fit->first, opened_nodes, std::cout);
    }
  }

  --level;

  return best_change;
}

Local_search::best_change_t
Local_search::seventh_neighborhood (const sorted_jobs_t& jobs1,
                                  const job_schedule_t& original_schedule,
                                  std::vector<Node>& opened_nodes,
                                  double total_cost,
                                  const std::vector<sorted_jobs_t>& nodes_map)
{
  if (verbose > 1)
  {
    std::string base = prepare_logging();
    std::cout << base << "\t--- seventh neighborhood." << std::endl;
  }

  ++level;
  std::string internal_base = prepare_logging();

  // proposed modifications
  best_change_t best_change({-INF,job_schedule_t()});
  bool changed = false;

  // keep track of already visited nodes
  unsigned count = 0;
  std::vector<bool> visited_nodes(opened_nodes.size(),false);

  // loop over jobs in order of tardiness
  for(sorted_jobs_t::const_iterator cit1 = jobs1.cbegin(); 
      cit1 != jobs1.cend() && count < k1; ++cit1)
  {
    const Schedule& sch1 = (*cit1)->second;
    unsigned n1_idx = sch1.get_node_idx();

    if (! visited_nodes[n1_idx])
    {
      // instantiate new solution
      job_schedule_t modified_schedule;

      // get maximum number of GPUs on current node
      Node& n1 = opened_nodes[n1_idx];
      unsigned max_n_GPUs = n1.get_usedGPUs() + n1.get_remainingGPUs();
      Node original_n1(n1);
      
      // get VMtype and GPUtype of current node
      const std::string& VMtype = n1.get_VMtype();
      const std::string& GPUtype = n1.get_GPUtype();

      if (verbose > 2)
        std::cout << internal_base << "\texamining node... " << n1.get_ID() 
                  << ": (" << VMtype << ", " << GPUtype << ", "
                  << n1.get_usedGPUs() << ", " << n1.get_remainingGPUs()
                  << ")" << std::endl;

      // get list of jobs running on the current node
      const sorted_jobs_t& r_jobs = nodes_map[n1_idx];

      unsigned new_total_g = 0;
      bool unfeasible = false;

      // loop over those jobs
      for (sorted_jobs_t::const_iterator r_jobs_it1 = r_jobs.cbegin();
           r_jobs_it1 != r_jobs.cend() && ! unfeasible; ++r_jobs_it1)
      {
        // get nGPUs and selected time of first job
        const Job& current_job1 = (*r_jobs_it1)->first;
        const Schedule& current_sch1 = (*r_jobs_it1)->second;
        unsigned current_g1 = current_sch1.get_setup().get_nGPUs();

        if (verbose > 2)
          std::cout << internal_base << "\t\texamining job... " 
                    << current_job1.get_ID() << " (" << current_g1 << ")"
                    << std::endl;

        // look for configurations with same VMtype and GPUtype and different 
        // number of GPUs
        const setup_time_t& tjvg = ttime.at(current_job1.get_ID());
        setup_time_t::const_iterator new_scit = tjvg.cend();
        bool found = false;
        for (setup_time_t::const_iterator scit = tjvg.cbegin();
             scit != tjvg.cend() && !found; ++scit)
        {
          const Setup& stp = scit->first;
          if (stp.get_VMtype() == VMtype && stp.get_GPUtype() == GPUtype &&
              stp.get_maxnGPUs() == max_n_GPUs && 
              stp.get_nGPUs() != current_g1)
          {
            new_scit = scit;
            new_total_g += stp.get_nGPUs();
            found = true;

            if (verbose > 2)
              std::cout << internal_base << "\t\t\tfound " << stp.get_nGPUs()
                        << " --> new_total_g = " << new_total_g << std::endl;
          }
        }

        unfeasible = (new_total_g > max_n_GPUs);

        if (found && !unfeasible)
          modified_schedule.insert({current_job1,Schedule(new_scit,n1_idx)});
      }

      // update setup and number of used GPUs in the current node
      if (new_total_g > 0)
      {
        if (verbose > 3)
          n1.print_open_node(std::cout);
        // update setup
        n1.change_setup((modified_schedule.cbegin()->second).get_setup());
        if (verbose > 3)
          n1.print_open_node(std::cout);
        // update number of used GPUs
        n1.set_remainingGPUs(new_total_g);
        if (verbose > 3)
          n1.print_open_node(std::cout);
      }

      // update the best modification if it has a better cost
      level+=3;
      bool now_changed = cfr_costs(total_cost, original_schedule, 
                                   modified_schedule, opened_nodes, 
                                   best_change);
      level-=3;

      // if the current modification has not been update, restore the status
      // of the current node
      if (! now_changed)
        std::swap(n1, original_n1);

      changed = (changed || now_changed);
      visited_nodes[n1_idx] = true;
      ++count;
    }
  }

  if (changed && verbose > 2)
  {
    std::cout << internal_base << "\t\t\t--> best choice of seventh neigh:\n";
    Schedule::print_names(std::cout);
    for (job_schedule_t::const_iterator fit = (best_change.second).cbegin();
         fit != (best_change.second).cend(); ++fit)
    {
      if (! (fit->second).isEmpty())
        (fit->second).print(fit->first, opened_nodes, std::cout);
    }
  }

  --level;

  return best_change;
}

bool
Local_search::swap_running_jobs (const Job& j1, const Job& j2, 
                                 const Schedule& sch1, const Schedule& sch2,
                                 const job_schedule_t& original_schedule,
                                 const std::vector<Node>& opened_nodes,
                                 double total_cost, best_change_t& best_change)
{
  std::string base = prepare_logging();
  if (verbose > 3)
    std::cout << base << "--- swap running jobs." << std::endl;

  bool changed = false;

  // swap the schedules of the two jobs
  unsigned n1 = sch1.get_node_idx();
  unsigned n2 = sch2.get_node_idx();
  const Setup& stp1 = sch1.get_setup();
  const Setup& stp2 = sch2.get_setup();

  setup_time_t::const_iterator new_it1 = select_setup(j1, stp2.get_VMtype(),
                                                      stp2.get_GPUtype(),
                                                      stp2.get_nGPUs());
  setup_time_t::const_iterator new_it2 = select_setup(j2, stp1.get_VMtype(),
                                                      stp1.get_GPUtype(),
                                                      stp1.get_nGPUs());

  job_schedule_t modified_schedule({{j1,Schedule(new_it1,n2)},
                                    {j2,Schedule(new_it2,n1)}});

  job_schedule_t new_schedule(original_schedule);

  // create full schedule with the modified jobs
  for (job_schedule_t::iterator change_it = modified_schedule.begin(); 
       change_it != modified_schedule.end(); ++change_it)
  {
    const Job& change_j = change_it->first;
    job_schedule_t::iterator original_it = new_schedule.find(change_j);

    if (verbose > 3)
      std::cout << base << "\tmodifying job " << change_j.get_ID() << "\n";

    if (original_it != new_schedule.end())
      std::swap(original_it->second,change_it->second);
  }

  unsigned original_verbosity_level = verbose;
  verbose = (verbose > 0) ? 0 : verbose;
  level+=2;
  double new_fft = find_first_finish_time(new_schedule);
  double new_total_cost = objective_function(new_schedule, new_fft, 
                                             opened_nodes);
  verbose = original_verbosity_level;
  level-=2;

  // save modification if it gives a better solution
  if (compare_costs(total_cost, new_total_cost, best_change.first))
  {
    changed = true;
    best_change.first = new_total_cost;
    std::swap(best_change.second, new_schedule);
    if (verbose > 3)
      std::cout << base << "\t--> CHANGED" << std::endl;
  }

  return changed;
}

bool
Local_search::cfr_costs (double total_cost, 
                         const job_schedule_t& original_schedule,
                         job_schedule_t& modified_schedule,
                         const std::vector<Node>& opened_nodes,
                         best_change_t& best_change)
{
  std::string base = prepare_logging();
  if (verbose > 3)
    std::cout << base << "--- cfr costs." << std::endl;

  bool changed = false;

  job_schedule_t new_schedule(original_schedule);

  // create full schedule with the modified jobs
  for (job_schedule_t::iterator change_it = modified_schedule.begin(); 
       change_it != modified_schedule.end(); ++change_it)
  {
    const Job& change_j = change_it->first;
    job_schedule_t::iterator original_it = new_schedule.find(change_j);

    if (verbose > 3)
      std::cout << base << "\tmodifying job " << change_j.get_ID() << "\n";

    if (original_it != new_schedule.end())
      std::swap(original_it->second,change_it->second);
  }

  unsigned original_verbosity_level = verbose;
  verbose = (verbose > 0) ? 0 : verbose;
  level+=2;
  double new_fft = find_first_finish_time(new_schedule);
  double new_total_cost = objective_function(new_schedule, new_fft, 
                                             opened_nodes);
  verbose = original_verbosity_level;
  level-=2;

  // save modification if it gives a better solution
  if (compare_costs(total_cost, new_total_cost, best_change.first))
  {
    changed = true;
    best_change.first = new_total_cost;
    std::swap(best_change.second, new_schedule);
    if (verbose > 3)
      std::cout << base << "\t--> CHANGED" << std::endl;
  }

  return changed;
}

