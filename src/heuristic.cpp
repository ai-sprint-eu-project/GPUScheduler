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

#include "heuristic.hpp"

Heuristic::Heuristic (const std::string& dir, const std::string& file_jobs, 
                      const std::string& file_times, 
                      const std::string& file_nodes)
{
  #ifdef SMALL_SYSTEM
    verbose = 2;
  #endif

  directory = dir + "/";
  if (verbose > 0)
    std::cout << "--- loading jobs list." << std::endl;
  create_container(jobs, directory + "../" + file_jobs);
  if (verbose > 0)
    std::cout << "--- loading time table." << std::endl;
  create_map(ttime, directory + "../"  + file_times);
  if (verbose > 0)
    std::cout << "--- loading nodes list." << std::endl;
  create_container(nodes, directory + "../"  + file_nodes);
}

std::string
Heuristic::prepare_logging (void) const
{
  std::string new_base = "";
  for (unsigned j = 0; j < level; ++j)
    new_base += "\t";
  return new_base;
}

double
Heuristic::submit_job (double elapsed_time)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- job submission." << std::endl;
  }
  double t = INF;
  if (! jobs.empty())
  {
    t = jobs.front().get_submissionTime();
    if (verbose > 1)
      std::cout << base << "\tt = " << t << " <? " << (current_time + elapsed_time);
    if (t <= (current_time + elapsed_time + TOL))
    {
      submitted_jobs.push_back(jobs.front());
      jobs.pop_front();
      if (verbose > 1)
        std::cout << base << " ---> SUBMITTED";
    }
    if (verbose > 1)
      std::cout << std::endl;
  }
  return t;
}

void
Heuristic::remove_ended_jobs (void)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- remove ended jobs." << std::endl;
  }
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
          submitted_jobs.erase(it);
        }
      }
    }
  }
}

void
Heuristic::close_nodes (void)
{
  for (Node& n : nodes)
    n.close_node();
  last_node_idx = 0;
}

void
Heuristic::update_execution_times (unsigned iter, double elapsed_time)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- update execution times." << std::endl;
  }
  unsigned last_iter = scheduled_jobs.size() - 1;
  job_schedule_t& last_schedule = scheduled_jobs[last_iter];

  job_schedule_t::const_iterator schit;
  for (schit = last_schedule.cbegin(); schit != last_schedule.cend(); ++schit)
  { 
    const Job& j = schit->first;
    const Schedule& sch = schit->second;
    double cp = sch.get_completionPercent();
    double cp_step = sch.get_cP_step();
    if (! sch.isEmpty() && cp < (100-TOL))
    {
      setup_time_t& tjvg = ttime.at(j.get_ID());
      setup_time_t::iterator it;
      for (it = tjvg.begin(); it != tjvg.end(); ++it)
      {
        double t = it->second * (100 - cp_step) / 100;
        it->second = (t > 0) ? t : INF;
      }
    }
  }
}

void
Heuristic::update_minMax_exec_time (Job& j) const
{
  double min_exec_time = INF;
  double max_exec_time = 0.;
  const setup_time_t& tjvg = ttime.at(j.get_ID());
  setup_time_t::const_iterator cit;
  for (cit = tjvg.cbegin(); cit != tjvg.cend(); ++cit)
  {
    min_exec_time = std::min(min_exec_time, cit->second);
    max_exec_time = std::max(max_exec_time, cit->second);
  }
  j.set_minExecTime(min_exec_time);
  j.set_maxExecTime(max_exec_time);
}

bool
Heuristic::update_scheduled_jobs (unsigned iter, double elapsed_time)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- update scheduled jobs." << std::endl;
  }

  bool all_completed = true;
  
  // select last schedule
  unsigned last_iter = scheduled_jobs.size() - 1;
  job_schedule_t& last_schedule = scheduled_jobs[last_iter];

  // compute simulation time
  double sim_time = current_time + elapsed_time;

  // loop over last schedule
  for (job_schedule_t::iterator it = last_schedule.begin();
       it != last_schedule.end(); ++it)
  {
    const Job& j = it->first;
    Schedule& sch = it->second;
    
    if (verbose > 1)
      std::cout << base << "\tanalyzing job... " << j.get_ID() << std::endl;

    // set iteration number and simulation time
    sch.set_iter(last_iter+1);
    sch.set_simTime(sim_time);
    
    // if schedule is not empty, compute execution time, completion percent
    // and cost of VM
    double cp = 0.0;
    if (! sch.isEmpty())
    {
      sch.set_executionTime(elapsed_time);
      cp = elapsed_time * 100 / sch.get_selectedTime();
      sch.set_cP_step(cp);

      // get number of used GPUs on the current node
      unsigned node_idx = sch.get_node_idx();
      unsigned g = nodes[node_idx].get_usedGPUs();

      // compute cost of VM
      sch.compute_vmCost(g);
      total_vm_cost += sch.get_vmCost();

      if (verbose > 1)
        std::cout << base << "\t\texecution_time = " << elapsed_time
                  << "; node_idx = " << node_idx << "; used_GPUs = " << g
                  << "; vm_cost = " << sch.get_vmCost() 
                  << "; partial cp = " << cp << std::endl;
    }

    // in all iterations after the first one, update start time and completion
    // percent of jobs that have already been partially executed
    if (iter > 1)
    {
      const job_schedule_t& previous_schedule = scheduled_jobs[last_iter-1];
      job_schedule_t::const_iterator p_sch = previous_schedule.find(j);
      if (p_sch != previous_schedule.cend())
      {
        sch.set_startTime((p_sch->second).get_startTime());
        double prev_cp = (p_sch->second).get_completionPercent();
        cp = prev_cp + cp * (100 - prev_cp) / 100;

        if (verbose > 1)
          std::cout << base << "\t\tpreviously running --> previous cp = "
                    << prev_cp << "; cp = " << cp
                    << std::endl;
      }
      else
        sch.set_startTime(current_time);
    }
    else
      sch.set_startTime(current_time);

    // set completion percent
    sch.set_completionPercent(cp);

    // set finish time, tardiness and tardiness cost of jobs that are 
    // completed
    if (cp >= (100-TOL))
    {
      sch.set_finishTime(sim_time);
      double tardiness = std::max(sim_time - j.get_deadline(), 0.);
      sch.set_tardiness(tardiness);
      sch.compute_tardinessCost(j.get_tardinessWeight());
      total_tardi += sch.get_tardiness();
      total_tardi_cost += sch.get_tardinessCost();

      if (verbose > 1)
        std::cout << base << "\t\ttardiness = " << tardiness
                  << "; t_cost = " << sch.get_tardinessCost() << std::endl;
    }
    else
    {
      sch.set_tardiness(0.0);
      all_completed = false;
    }
  }

  return all_completed;
}

double
Heuristic::find_first_finish_time (const job_schedule_t& last_schedule) const
{
  double fft = INF;

  job_schedule_t::const_iterator cit;
  for (cit = last_schedule.cbegin(); cit != last_schedule.cend(); ++cit)
    fft = std::min(fft, (cit->second).get_selectedTime());

  return fft;
}

bool
Heuristic::available_resources (void) const
{
  bool available = false;

  // check if there are still closed nodes
  if (last_node_idx < nodes.size())
    available = true;
  else
  {
    // loop over nodes starting from the last one, until free resources are 
    // found
    for (int idx = last_node_idx - 1; idx >= 0 && ! available; --idx)
    {
      const Node& n = nodes[idx];
      if (n.get_remainingGPUs() > 0)
        available = true;
    }
  }
  
  return available;
}

void
Heuristic::preprocessing (void)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- preprocessing step." << std::endl;
  }

  // update minimum execution time, maximum execution time and pressure of 
  // all submitted jobs
  std::string queue = "";
  for (Job& j : submitted_jobs)
  {
    queue += (j.get_ID() + "; ");
    update_minMax_exec_time(j);
    j.update_pressure(current_time);
  }

  if (verbose > 1)
    std::cout << base << "\tlist of submitted jobs: " << queue << std::endl;

  // sort the queue
  sort_jobs_list();
}

job_schedule_t 
Heuristic::perform_scheduling (void)
{
  // (STEP #0)
  preprocessing();

  // compute best schedule
  job_schedule_t best_schedule;
  scheduling_step(best_schedule);

  #ifdef SMALL_SYSTEM
    double fft = find_first_finish_time(best_schedule);
    double minTotalCost = objective_function(best_schedule, fft, nodes);
    std::string base = prepare_logging();
    std::cout << base << "best cost = " << minTotalCost << std::endl;
  #endif

  return best_schedule;
}

void
Heuristic::algorithm (unsigned v)
{
  #ifdef SMALL_SYSTEM
    // change verbosity level
    verbose = v;
  #endif

  // intialization
  unsigned iter = 0;
  double first_finish_time = INF;
  bool all_completed = false;
  bool stop = false;
  
  while (!stop)
  {
    #ifdef SMALL_SYSTEM
      std::cout << "\n######################## ITER " << iter 
                << " ########################\n" << std::endl;
    #endif

    // add new job to the queue
    double elapsed_time = std::min(
                                scheduling_interval,
                                first_finish_time
                              );
    double next_submission_time = submit_job(elapsed_time);

    // compute elapsed time between current and previous iteration
    elapsed_time = std::min(
                          elapsed_time,
                          next_submission_time - current_time
                        );

    if (verbose > 1)
      std::cout << "current_time = " << current_time
                << "; first_finish_time = " << first_finish_time
                << "; next_submission_time = " << next_submission_time
                << "; elapsed_time = " << elapsed_time << std::endl;

    // update scheduled_jobs to compute execution costs
    if (iter > 0)
    {
      all_completed = update_scheduled_jobs(iter, elapsed_time);
      // if there is a gap between the completion of the last job and the
      // submission of the new one, force the submission of a new job
      if (all_completed && (next_submission_time < INF))
      {
        elapsed_time = submit_job(INF) - current_time;
        if (verbose > 1)
          std::cout << "\tnew elapsed_time = " << elapsed_time << std::endl;
      }
    }
    
    // check stopping cryterion
    stop = (next_submission_time == INF) && all_completed;

    if (!stop)
    {
      // close all currently opened nodes
      close_nodes();

      // update current_time according to the elapsed time between current 
      // and previous iteration
      current_time += elapsed_time;

      if (verbose > 1)
        std::cout << "\t new current_time = " << current_time 
                  << "\n" << std::endl;

      if (iter > 0)
      {
        // remove ended jobs from the queue
        remove_ended_jobs();
        // update execution time of jobs that have been partially executed
        update_execution_times(iter, elapsed_time);
      }

      // determine the best schedule
      job_schedule_t best_schedule = perform_scheduling();
      
      // add the best schedule to scheduled_jobs
      scheduled_jobs.push_back(best_schedule);

      // find execution time of first ending job
      first_finish_time = find_first_finish_time(scheduled_jobs[iter]);
      iter++;
    }
  }
}

void
Heuristic::algorithm (unsigned v, unsigned myseed, unsigned mri)
{
  // set seed for random number generation and maximum number of iterations
  generator.seed(myseed);
  max_random_iter = mri;

  algorithm(v);
}

void
Heuristic::print_data (void) const
{
  print_container(jobs, directory + "jobs_list.csv");
  print_map(ttime, directory + "ttime.csv");
  print_container(nodes, directory + "nodes.csv");
}

void
Heuristic::print_schedule (const std::string& filename) const
{
  #ifdef SMALL_SYSTEM
    print_result(scheduled_jobs, nodes, directory + filename);
  #else
    std::ofstream ofs(directory + "all_costs.csv", std::ios_base::app);
    ofs << filename << ", " << total_tardi << ", "
        << total_tardi_cost << ", " << total_vm_cost << ", "
        << (total_vm_cost+total_tardi_cost) << std::endl;
  #endif
}
