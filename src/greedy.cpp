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

#include "greedy.hpp"

Greedy::Greedy (const std::string& directory, const std::string& file_jobs, 
                const std::string& file_times, const std::string& file_nodes):
  Heuristic(directory, file_jobs, file_times, file_nodes)
{}

bool
Greedy::compare_pressure (const Job& j1, const Job& j2)
{
  return j1.get_pressure() > j2.get_pressure();
}

bool
Greedy::compare_configuration (const Setup& stp, const Node& n) const
{
  return (stp.get_GPUtype() == n.get_GPUtype());
}

void
Greedy::sort_jobs_list (void)
{
  // sort list
  submitted_jobs.sort(compare_pressure);
}

double
Greedy::objective_function (job_schedule_t& new_schedule, double elapsed_time,
                            const std::vector<Node>& opened_nodes)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- objective function." << std::endl;
  }

  double vmCost = 0.;
  double tardiCost = 0.;
  double worstTardiCost = 0.;
  double sim_time = current_time + elapsed_time;

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
      // set execution time
      sch.set_executionTime(elapsed_time);
      
      // get number of used GPUs on the current node
      unsigned node_idx = sch.get_node_idx();
      unsigned g = opened_nodes[node_idx].get_usedGPUs();

      // compute cost of VM
      sch.compute_vmCost(g);
      vmCost += sch.get_vmCost();

      // compute tardiness
      double tardiness = std::max(sim_time - j.get_deadline(), 0.);
      sch.set_tardiness(tardiness);
      sch.compute_tardinessCost(j.get_tardinessWeight());
      tardiCost += sch.get_tardinessCost();

      if (verbose > 1)
        std::cout << base << "\t\texecution_time = " << elapsed_time
                  << "; node_idx = " << node_idx << "; used_GPUs = " << g
                  << "; vm_cost = " << sch.get_vmCost()
                  << "; tardiness = " << tardiness
                  << "; t_cost = " << sch.get_tardinessCost() << std::endl;
    }
    else
    {
      double wCT = std::max(current_time + j.get_maxExecTime() - 
                            j.get_deadline(), 0.0);
      double worstCaseTardinessCost = 100 * wCT * j.get_tardinessWeight();
      worstTardiCost += worstCaseTardinessCost;

      if (verbose > 1)
        std::cout << base << "\t\tworstCaseTardiness = " << wCT
                  << "; wCT_cost = " << worstCaseTardinessCost << std::endl;
    }
  }

  if (verbose > 0)
    std::cout << base << "\tvmCost: " << vmCost << "; tardiCost: " << tardiCost
              << "; worstCaseTardiness: " << worstTardiCost << std::endl;

  return (vmCost + tardiCost + worstTardiCost);
}

unsigned
Greedy::select_best_node (const Setup& best_stp)
{
  unsigned best_idx = last_node_idx;
  int best_diff = std::numeric_limits<int>::max();

  for (unsigned idx = 0; idx < last_node_idx; ++idx)
  {
    const Node& node = nodes[idx];

    // compute the difference between the number of idle GPUs on the 
    // current node and the number of required GPUs
    int current_diff = node.get_remainingGPUs() - best_stp.get_nGPUs();

    // if the best setup fits the current node and the current difference is
    // lower than the best, update the best index
    if (compare_configuration(best_stp, node) && current_diff >= 0)
    {
      if (current_diff < best_diff)
      {
        best_idx = idx;
        best_diff = current_diff;
      }
    }
  }

  return best_idx;
}

bool
Greedy::assign_to_existing_node (const Job& j, 
                                 setup_time_t::const_iterator best_stp_it,
                                 job_schedule_t& new_schedule)
{
  bool assigned = false;
  const Setup& best_stp = best_stp_it->first;

  unsigned best_idx = select_best_node(best_stp);
  
  if (best_idx < last_node_idx)
  {
    Node& node = nodes[best_idx];
    assigned = true;
    node.set_remainingGPUs(best_stp.get_nGPUs());

    const std::string& VMtype = node.get_VMtype();
    const std::string& GPUtype = node.get_GPUtype();
    unsigned nGPUs = best_stp.get_nGPUs();

    setup_time_t::const_iterator selected_stp_it;
    selected_stp_it = select_setup(j, VMtype, GPUtype, nGPUs);

    Schedule sch(selected_stp_it, best_idx);
    new_schedule[j] = sch;
  }

  return assigned;
}

void
Greedy::assign_to_new_node (const Job& j, 
                            setup_time_t::const_iterator best_stp_it,
                            job_schedule_t& new_schedule)
{
  std::string base = "";
  const Setup& best_stp = best_stp_it->first;

  // determine a new setup with the same GPUtype and nGPUs, but the largest
  // possible number of available GPUs
  const std::string& GPUtype = best_stp.get_GPUtype();
  unsigned nGPUs = best_stp.get_nGPUs();
  setup_time_t::const_iterator largest_it = select_largest_setup(j, nGPUs, 
                                                                 GPUtype);
  const Setup& largest_stp = largest_it->first;

  if (verbose > 1)
  {
    base = prepare_logging();
    std::cout << base << "\t\tlargest setup: (" << largest_stp.get_VMtype() 
              << ", " << largest_stp.get_maxnGPUs() << ")\n"
              << "\tnode idx: " << last_node_idx << std::endl;
  }

  // open a new node with the required configuration
  Node& n = nodes[last_node_idx];
  n.open_node(largest_stp);
  n.set_remainingGPUs(largest_stp.get_nGPUs());

  // assign
  Schedule sch(largest_it, last_node_idx);
  new_schedule[j] = sch;

  if (verbose > 1)
    std::cout << base << "\t\t---> ASSIGNED" << std::endl;

  // update index of last used node
  last_node_idx++;
}

bool
Greedy::assign_to_suboptimal (const Job& j, const setup_time_t& tjvg,
                              Dstar& dstar, job_schedule_t& new_schedule)
{
  std::string base = "";
  bool assigned = false;
  while (!dstar.is_end() && !assigned)
  {
    setup_time_t::const_iterator best_stp = dstar.get_best_setup();

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

setup_time_t::const_iterator
Greedy::select_largest_setup (const Job& j, unsigned nGPUs,
                              const std::string& GPUtype) const
{
  const setup_time_t& tjvg = ttime.at(j.get_ID());
  setup_time_t::const_iterator cit;
  setup_time_t::const_iterator found_it = tjvg.cend();
  unsigned maxnGPUs = 0;
  for (cit = tjvg.cbegin(); cit != tjvg.cend(); ++cit)
  {
    const Setup& new_stp = cit->first;
    if (new_stp.get_GPUtype() == GPUtype && new_stp.get_nGPUs() == nGPUs)
    {
      if (new_stp.get_maxnGPUs() > maxnGPUs)
      {
        maxnGPUs = new_stp.get_maxnGPUs();
        found_it = cit;
      }
    }
  }
  return found_it;
}

setup_time_t::const_iterator
Greedy::select_setup (const Job& j, const std::string& VMtype, 
                      const std::string& GPUtype, unsigned g) const
{
  const setup_time_t& tjvg = ttime.at(j.get_ID());
  bool found = false;

  Setup temp_stp(VMtype, GPUtype);

  range_t range_it = tjvg.equal_range(temp_stp);

  setup_time_t::const_iterator new_stp_it = range_it.first;
  while (new_stp_it != range_it.second && ! found)
  {
    const Setup& new_stp = new_stp_it->first;
    if (new_stp.get_nGPUs() == g)  
      found = true;
    else
      ++new_stp_it;
  }
  return new_stp_it;
}

setup_time_t::const_iterator
Greedy::select_setup_in_range (range_t range, const Node& node, 
                               unsigned g) const
{
  unsigned GPUs_on_node = node.get_usedGPUs() + node.get_remainingGPUs();

  bool found = false;
  setup_time_t::const_iterator new_stp_it = range.first;
  while (new_stp_it != range.second && ! found)
  {
    const Setup& new_stp = new_stp_it->first;
    if (new_stp.get_nGPUs() == g && new_stp.get_maxnGPUs() == GPUs_on_node)
      found = true;
    else
      ++new_stp_it;
  }

  return new_stp_it;
}

void
Greedy::change_nodes_configurations (const job_schedule_t& current_schedule)
{
  const Job& temp_j = current_schedule.cbegin()->first;
  const setup_time_t& temp_tjvg = ttime.at(temp_j.get_ID());
  
  for (unsigned node_idx = 0; node_idx < last_node_idx; ++node_idx)
  {
    Node& node = nodes[node_idx];
    if (node.get_remainingGPUs() > 0)
    {
      double minCost = node.get_cost();
      setup_time_t::const_iterator new_it = temp_tjvg.cend();
      
      for (setup_time_t::const_iterator temp_it = temp_tjvg.cbegin();
           temp_it != temp_tjvg.cend(); ++temp_it)
      {
        const Setup& temp_stp = temp_it->first;
        bool checkGPUtype = (temp_stp.get_GPUtype() == node.get_GPUtype());
        bool checkGPUs_1 = (temp_stp.get_maxnGPUs() == node.get_usedGPUs());
        bool checkGPUs_2 = (temp_stp.get_maxnGPUs() == node.get_usedGPUs()+1);
        bool checkCost = (temp_stp.get_cost() < minCost);
        if (checkGPUtype && (checkGPUs_1 || checkGPUs_2) && checkCost)
        {
          minCost = temp_stp.get_cost();
          new_it = temp_it;
        }
      }

      if (new_it != temp_tjvg.cend())
      {
        const Setup& new_stp = new_it->first;
        node.change_setup(new_stp);
      }
    }
  }  
}

void
Greedy::perform_assignment (const Job& j, job_schedule_t& new_schedule)
{
  std::string base = "";

  // determine the setups s.t. deadline is matched
  const setup_time_t& tjvg = ttime.at(j.get_ID());
  Dstar dstar(j, tjvg, current_time);

  // determine the best setup:
  //   if it is possible to match deadline, the best setup is the 
  //   cheapest; otherwise, the best setup is the fastest
  setup_time_t::const_iterator best_stp = dstar.get_best_setup();

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

void
Greedy::scheduling_step (job_schedule_t& new_schedule)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << "--- scheduling step." << std::endl;
  }
  
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
      Greedy::perform_assignment(j, new_schedule);
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
Greedy::postprocessing (job_schedule_t& current_schedule)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- postprocessing step." << std::endl;
  }

  // highest speed-up pair
  std::pair<Job, setup_time_t::const_iterator> best_speedup;
  double previous_delta = 0.;

  // change configuration of nodes with idle GPUs
  change_nodes_configurations(current_schedule);

  // loop over the current schedule
  for (job_schedule_t::iterator schit = current_schedule.begin();
       schit != current_schedule.end(); ++schit)
  {
    const Job& j = schit->first;
    Schedule& sch = schit->second;
    const setup_time_t& tjvg = ttime.at(j.get_ID());

    // if the schedule is not empty
    if (! sch.isEmpty())
    {
      unsigned node_idx = sch.get_node_idx();
      const Setup& stp = sch.get_setup();

      Node& node = nodes[node_idx];
      Setup node_stp(node.get_VMtype(), node.get_GPUtype());

      // if the configuration of node has been changed, change the current 
      // schedule accordingly
      if (stp != node_stp)
      {
        range_t range = tjvg.equal_range(node_stp);

        setup_time_t::const_iterator new_stp_it;
        new_stp_it = select_setup_in_range (range, node, stp.get_nGPUs());
        
        if (new_stp_it != range.second)
        {
          const Setup& new_stp = new_stp_it->first;
          node.set_remainingGPUs(new_stp.get_nGPUs());
          sch.change_setup(new_stp_it);
        }
      }
    }
  }

  // loop over the current schedule
  for (job_schedule_t::iterator schit = current_schedule.begin();
       schit != current_schedule.end(); ++schit)
  {
    const Job& j = schit->first;
    Schedule& sch = schit->second;
    const setup_time_t& tjvg = ttime.at(j.get_ID());

    // if the schedule is not empty
    if (! sch.isEmpty())
    {
      unsigned node_idx = sch.get_node_idx();

      Node& node = nodes[node_idx];
      Setup node_stp(node.get_VMtype(), node.get_GPUtype());

      // if there are still idle GPUs in the current configuration, determine 
      // the job that would get the highest speed-up by getting the 
      // additional resources
      if (node.get_remainingGPUs() > 0)
      {
        const Setup& stp = sch.get_setup();

        range_t range = tjvg.equal_range(node_stp);

        unsigned new_nGPUs = stp.get_nGPUs() + node.get_remainingGPUs();
        setup_time_t::const_iterator new_stp_it;
        new_stp_it = select_setup_in_range (range, node, new_nGPUs);

        if (new_stp_it != range.second)
        {
          double delta = sch.get_selectedTime() - new_stp_it->second;

          if (delta > previous_delta)
          {
            previous_delta = delta;
            best_speedup = {j, new_stp_it};
          }
        }
      }
    }
  }

  // assign the additional resources to the job with highest speed-up
  if (previous_delta > 0.)
  {
    const Job& temp_j = best_speedup.first;
    setup_time_t::const_iterator new_stp_it = best_speedup.second;
    job_schedule_t::iterator schit = current_schedule.find(temp_j);
    if (schit != current_schedule.end())
    {
      Schedule& sch = schit->second;
      unsigned node_idx = sch.get_node_idx();
      Node& node = nodes[node_idx];
      const Setup& new_stp = new_stp_it->first;

      unsigned previous_g = sch.get_setup().get_nGPUs();

      node.set_remainingGPUs(new_stp.get_nGPUs() - previous_g);
      sch.change_setup(new_stp_it);
    }
  }
}

