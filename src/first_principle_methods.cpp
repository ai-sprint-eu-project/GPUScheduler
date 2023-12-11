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

#include "first_principle_methods.hpp"

First_principle_methods::First_principle_methods (const std::string& directory,
                                                const std::string& file_jobs, 
                                                const std::string& file_times,
                                                const std::string& file_nodes):
  Heuristic(directory, file_jobs, file_times, file_nodes),
  nodes_info(std::vector<node_info>(jobs.size()))
{
  if(scheduling_interval != INF) {
    std::cout << "WARNING: \n  setting the variable 'scheduling_interval' to finite values is useless in this context and could increase the computing time." << std::endl;
  }
}

bool
First_principle_methods::compare_configuration (const Setup& stp, const Node& n) const
{
  return (stp.get_VMtype() == n.get_VMtype() &&
          stp.get_GPUtype() == n.get_GPUtype());
}

double
First_principle_methods::objective_function (job_schedule_t& new_schedule, double elapsed_time,
                                   const std::vector<Node>& opened_nodes)
{
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

    // if schedule is not empty, compute cost of VM and tardiness cost
    if (! sch.isEmpty())
    {
      // set execution time
      sch.set_executionTime(elapsed_time);

      // get number of used GPUs on the current node
      unsigned node_idx = sch.get_node_idx();
      unsigned g = opened_nodes[node_idx].get_usedGPUs();

      if(g != 0) 
      {
        // compute cost of VM
        sch.compute_vmCost(g);
        vmCost += sch.get_vmCost();
      }

      // compute tardiness
      double tardiness = std::max(sim_time - j.get_deadline(), 0.);
      sch.set_tardiness(tardiness);
      sch.compute_tardinessCost(j.get_tardinessWeight());
      tardiCost += sch.get_tardinessCost();
    }
    else
    {
      double worstCaseTardiness = 100 * j.get_maxExecTime() *
                                  j.get_tardinessWeight();
      worstTardiCost += worstCaseTardiness;
    }
  }

  //std::cout << "\tvmCost: " << vmCost << "; tardiCost: " << tardiCost
  //          << "; worstCaseTardiness: " << worstTardiCost << std::endl;
  //std::cout << "," << vmCost << "," << tardiCost
  //          << "," << worstTardiCost << std::endl;
  return (vmCost + tardiCost + worstTardiCost);
}


setup_time_t::const_iterator
First_principle_methods::select_setup_in_range (range_t range, const Node& node,
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
First_principle_methods::change_nodes_configurations (const job_schedule_t& current_schedule)
{
  const Job& temp_j = current_schedule.cbegin()->first;
  const setup_time_t& temp_tjvg = ttime.at(temp_j.get_ID());

  for (unsigned node_idx = 0; node_idx < nodes.size(); ++node_idx)
  {
    if(is_active(node_idx)) {
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
}

unsigned
First_principle_methods::get_job_index(const Job& j) const {
  unsigned idx = 0;
  unsigned last_iter = 0;
  if(!scheduled_jobs.empty()) {
    last_iter = scheduled_jobs.size() - 1;
    idx = (scheduled_jobs[last_iter].at(j).get_node_idx());
  }
  return idx;
}

void
First_principle_methods::renew_or_assign (const Job& j,
                            setup_time_t::const_iterator best_stp_it,
                            job_schedule_t& new_schedule)
{

  const Setup& best_stp = best_stp_it->first;
  // if the job was in execution at previous iteration and it has not yet completed
  if(is_in_execution(j)) {
    // keep unchanged that node's configuration
    unsigned idx = get_job_index(j);
    // assign the job to the schedule
    Schedule sch(best_stp_it, idx);
    new_schedule[j] = sch;
  }

  else {
    // find a free node and assign the job to it
    bool found = false;
    for(size_t i = 0; i < nodes.size() && !found; i++) {
      if(!is_active(i)) {
        found = true;

        // open a new node with the required configuration
        Node& n = nodes[i];
        n.open_node(best_stp);
        n.set_remainingGPUs(best_stp.get_nGPUs());

        // assign
        Schedule sch(best_stp_it, i);
        new_schedule[j] = sch;

        // update data structures
        set_active(i,true);
        nodes_info[i].first = best_stp_it;
      }
    }
  }
}

bool
First_principle_methods::is_in_execution(const Job& j) const {
  unsigned last_iter;
  if(!scheduled_jobs.empty()) {
    last_iter = scheduled_jobs.size() - 1;
  }
  else {
    return false;
  }
  return ((scheduled_jobs[last_iter].find(j) != scheduled_jobs[last_iter].cend() && is_active(get_job_index(j))));
}

/*setup_time_t::const_iterator
First_principle_methods::get_best_time_setup(const setup_time_t& tjvg) const {
  std::map<double, setup_time_t::const_iterator> DStar_aux;
  for(auto i = tjvg.cbegin(); i != tjvg.cend(); i++) {
    DStar_aux.insert({i->second, i});
  }
  return DStar_aux.cbegin()->second;
}*/

/*
setup_time_t::const_iterator
First_principle_methods::get_best_time_setup(const setup_time_t& tjvg, setup_time_t::const_iterator & best_stp) const {

  std::map<unsigned, setup_time_t::const_iterator> DStar_aux;
  for(auto i = tjvg.cbegin(); i != tjvg.cend(); i++) {
    if((*i).first == (*best_stp).first.get_maxnGPUs()) {
      return i->first;
    }
  }
  return (DStar_aux.cbegin())->second;
}
*/


void
First_principle_methods::perform_assignment (const Job& j, job_schedule_t& new_schedule)
{
  // get list of execution times for the given job
  const setup_time_t& tjvg = ttime.at(j.get_ID());

  // new best setup
  setup_time_t::const_iterator best_stp;

  // if it's the first time we execute the job, then we compute the best setup; 
  // if not, the best setup remains the one we computed at first
  if(! is_in_execution(j)) 
  {
    // determine the best setup:
    //   if it is possible to match deadline, the best setup is the
    //   cheapest; otherwise, the best setup is the fastest
    Dstar dstar(j, tjvg, current_time);
    best_stp = dstar.get_best_setup();
  }
  else
    best_stp = nodes_info[get_job_index(j)].first;

  // renew the assignment of the job (if it was already in execution) or, if it 
  // is possible, open a new node with the optimal configuration
  if (is_in_execution(j) || nodes_in_use < nodes.size())
    renew_or_assign(j, best_stp, new_schedule);
}

void
First_principle_methods::scheduling_step (job_schedule_t& new_schedule)
{
  std::string queue = "";

  // perform assignment of all submitted jobs not yet completed but 
  // partially executed (STEP #1)
  if (!scheduled_jobs.empty()) 
  {
    unsigned last_iter;
    last_iter = scheduled_jobs.size() - 1;
    for (const auto & element : scheduled_jobs[last_iter])
    {
      if(is_in_execution(element.first)) {
        queue += (element.first.get_ID() + "; ");
        perform_assignment(element.first, new_schedule);
      }
    }
  }
  // assigns the remaining ones, if it has remaining nodes
  for (const Job& j : submitted_jobs)
  {
    if (!is_in_execution(j)) 
    {
      queue += (j.get_ID() + "; ");
      perform_assignment(j, new_schedule);
    }
  }

  //std::cout << "\tn_submitted_jobs: " << submitted_jobs.size()
  //          << "; queue: " << queue
  //          << "; n_used_nodes: " << last_node_idx << std::endl;
}

void
First_principle_methods::close_nodes (void)
{
  for (unsigned i = 0; i < nodes.size(); i++)
  {
    if(!is_active(i))
      nodes[i].close_node();
  }
}

void
First_principle_methods::remove_ended_jobs (void)
{
  unsigned last_iter = scheduled_jobs.size() - 1;
  job_schedule_t& last_schedule = scheduled_jobs[last_iter];

  job_schedule_t::const_iterator cit;
  for (cit = last_schedule.cbegin(); cit != last_schedule.cend(); ++cit)
  {
    const Job& j = cit->first;
    const Schedule& sch = cit->second;

    if (sch.get_completionPercent() >= (100-TOL))
    {
      std::list<Job>::iterator it;
      bool found = false;
      for (it=submitted_jobs.begin(); it!=submitted_jobs.end() && !found;
           ++it)
      {
        if (*it == j)
        {
          found = true;
          set_active(get_job_index(*it),false);
          submitted_jobs.erase(it);
        }
      }
    }
  }
}

void First_principle_methods::sort_jobs_list (void)
{
  Job j(submitted_jobs.back());
  auto it = ++submitted_jobs.rbegin();
  bool found = false;
  while(it != submitted_jobs.rend() && !found){
    if (!compare(j,*it)){
      found = true;
      submitted_jobs.insert(it.base(),j);
    }
    it++;
  }
  if (!found)
    submitted_jobs.push_front(j);
  submitted_jobs.pop_back();
}

bool First_principle_methods::is_active(unsigned idx) const 
{
  return nodes_info[idx].second;
}

void First_principle_methods::set_active(unsigned idx, bool b)
{
  nodes_info[idx].second = b;
  if(b)
    nodes_in_use++;
  else
    nodes_in_use--;
}
