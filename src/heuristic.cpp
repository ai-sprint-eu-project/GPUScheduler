#include "heuristic.hpp"

template <typename COMP>
Heuristic<COMP>::Heuristic (const System& s, const obj_function_t& pf,
                            unsigned l):
  system(s), proxy_function(pf), level(l)
{}

template <typename COMP>
void
Heuristic<COMP>::sort_jobs_list (void)
{
  // sort list
  std::list<Job>& submitted_jobs = system.get_submittedJobs();
  submitted_jobs.sort(compare_pressure);
}

template <typename COMP>
bool
Heuristic<COMP>::available_resources (void) const
{
  unsigned n_nodes = system.get_n_nodes();
  unsigned n_full_nodes = system.get_n_full_nodes();
  bool avail = (n_full_nodes < n_nodes);
  if (verbose > 2)
  {
    std::string base = prepare_logging(level);
    std::cout << base << "\t\tn_full_nodes = " << n_full_nodes
              << "; n_nodes = " << n_nodes << " --> "
              << (avail ? "AVAIL" : "NOT AVAIL")
              << std::endl;
  }
  return avail;
}

template <typename COMP>
void
Heuristic<COMP>::close_nodes (void)
{
  system.close_nodes();
}

template <typename COMP>
bool
Heuristic<COMP>::assign (const Job& j, 
                            setup_time_t::const_iterator best_stp_it,
                            job_schedule_t& new_schedule)
{
  bool assigned = false;
  const std::string& required_GPUtype = std::get<0>(best_stp_it->first);
  unsigned required_GPUs = std::get<1>(best_stp_it->first);
  double required_GPU_f = std::get<2>(best_stp_it->first);

  // find a node with the required GPU type and update its number of
  // available GPUs if possible
  std::string node_ID = system.assign_to_node(required_GPUtype, required_GPUs,
                                              required_GPU_f);

  // if such a node exists, perform the assignment
  if (! node_ID.empty())
  {
    unsigned GPU_ID = 0;
    // if opt config is fractionary, split ID in node_ID and GPU_ID
    if (required_GPU_f < 1)
    {
      GPU_ID = std::stoi(node_ID.substr(node_ID.find("_") + 1));
      node_ID.erase(node_ID.find("_"));
    }

    if (this->verbose > 1)
    {
      std::string base = prepare_logging(this->level);
      std::cout << base << "\t\t\t---> ASSIGNED to " << node_ID << std::endl;
    }

    assigned = true;
    Schedule sch(node_ID, required_GPUtype, best_stp_it->second, 
                 required_GPUs, required_GPU_f, GPU_ID);
    new_schedule[j] = sch;
  }

  return assigned;
}

template <typename COMP>
unsigned
Heuristic<COMP>::scheduling_step (job_schedule_t& new_schedule)
{
  // print info
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging(level);
    std::cout << base << "--- scheduling step." << std::endl;
  }

  // initialize number of assigned jobs
  unsigned n_assigned_jobs = 0;

  // loop over the submitted jobs...
  const std::list<Job>& submitted_jobs = system.get_submittedJobs();
  std::string queue = "";
  bool resources = available_resources();
  for (const Job& j : submitted_jobs)
  {
    queue += (j.get_ID() + "; ");
    if (verbose > 1)
    {
      std::cout << base << "\n\tanalyzing job...";
      j.print(std::cout);
    }

    // ...and perform assignment until resources are available
    if (resources)
    {
      n_assigned_jobs += perform_assignment(j, new_schedule);
      resources = available_resources();
    }
    else
      new_schedule[j] = Schedule();
  }

  this->postprocessing(new_schedule);

  // print info
  if (verbose > 0)
  {
    std::cout << base << "\tn_submitted_jobs: " << submitted_jobs.size()
              << "; n_assigned_jobs: " << n_assigned_jobs;
    if (verbose > 1)
      std::cout << "; queue: " << queue;
    std::cout << std::endl;
  }

  return n_assigned_jobs;
}

template <typename COMP>
void
Heuristic<COMP>::postprocessing (job_schedule_t& current_schedule)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging(level);
    std::cout << base << "--- postprocessing step." << std::endl;
  }

  // get time table
  time_table_ptr ttime = system.get_ttime();

  // highest speed-up pair
  using best_speedup_t = std::pair<job_schedule_t::iterator,
                                   setup_time_t::const_iterator>;
  best_speedup_t best_speedup = {current_schedule.end(),
                                 (ttime->cbegin())->second.cend()};
  double previous_delta = 0.;

  bool single_job = false;

  // get shared GPUs map and iterate over it
  gpus_map_t& gmap = system.get_nodes().get_shared_gpus();
  gpus_map_t& full_gmap = system.get_nodes().get_full_shared_gpus();

  for (gpus_map_t::iterator it1 = gmap.begin(); it1 != gmap.end(); ++it1)
  {
    const std::string& GPUtype = it1->first;
    gpus_t& glist = it1->second;
    for (gpus_t::iterator it2 = glist.begin(); it2 != glist.end(); ++it2)
    {
      double remaining_f = std::get<0>(*it2);
      const std::string& node_ID = std::get<1>(*it2);
      unsigned GPU_ID = std::get<2>(*it2);

      do
      {
        previous_delta = 0.;
        single_job = false;

        // loop over the current schedule
        for (job_schedule_t::iterator schit = current_schedule.begin();
             schit != current_schedule.end(); ++schit)
        {
          const Job& j = schit->first;
          Schedule& sch = schit->second;

          // if job is executed on the current shared GPU
          if (sch.get_nodeID() == node_ID && sch.get_GPUID() == GPU_ID)
          {
            const setup_time_t& tjvg = ttime->at(j.get_ID());

            // determine the job that would get the highest speed-up by getting
            // the additional resources
            double f = sch.get_GPUf() + remaining_f;
            setup_time_t::const_iterator stp_it =
                          tjvg.find(std::make_tuple(GPUtype,1,sch.get_GPUf()));
            if (stp_it != tjvg.cend())
            {
              double selected_time = stp_it->second;
              ++stp_it;
              while (stp_it != tjvg.cend() && std::get<1>(stp_it->first) == 1 &&
                     std::get<2>(stp_it->first) <= f)
              {
                double delta = selected_time - stp_it->second;
                if (delta > previous_delta)
                {
                  previous_delta = delta;
                  best_speedup = {schit, stp_it};
                  if (std::get<2>(stp_it->first) == 1.)
                    single_job = true;
                }
                ++stp_it;
              }
            }
          }

          if (single_job)
            break;
        }

        // assign the additional resources to the job with highest speed-up
        if (previous_delta > 0.)
        {
          // find the job in the current schedule
          job_schedule_t::iterator schit = best_speedup.first;
          setup_time_t::const_iterator new_stp_it = best_speedup.second;

          if (schit != current_schedule.end())
          {
            // determine the previous schedule and configuration
            Schedule& sch = schit->second;
            double previous_f = sch.get_GPUf();

            // get the new setup
            const setup_t& new_stp = new_stp_it->first;
            double new_time = new_stp_it->second;
            double new_f = std::get<2>(new_stp);

            // update the configuration
            remaining_f += previous_f - new_f;
            sch.change_configuration(new_time, new_f, previous_f);
          }
        }
      }
      while(previous_delta > 0. && remaining_f > 0.);

      if (remaining_f > 0.)
      {
        std::get<0>(*it2) = remaining_f;
        glist.sort();
      }
      else
      {
        gpus_t::iterator it = it2--;
        gpu_t GPU = *it;
        glist.erase(it);
        if (! single_job)
          full_gmap[GPUtype].push_back(GPU);
      }
    }
  }
}

/*
template <typename COMP>
void
Heuristic<COMP>::postprocessing (job_schedule_t& current_schedule)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging(level);
    std::cout << base << "--- postprocessing step." << std::endl;
  }

  // get time table
  time_table_ptr ttime = system.get_ttime();

  // highest speed-up pair
  using best_speedup_t = std::pair<job_schedule_t::iterator, 
                                   setup_time_t::const_iterator>;
  best_speedup_t best_speedup = {current_schedule.end(), 
                                 (ttime->cbegin())->second.cend()};
  double previous_delta = 0.;

  // loop over the current schedule
  for (job_schedule_t::iterator schit = current_schedule.begin();
       schit != current_schedule.end(); ++schit)
  {
    const Job& j = schit->first;
    Schedule& sch = schit->second;
    const setup_time_t& tjvg = ttime->at(j.get_ID());

    // if the schedule is not empty
    if (! sch.isEmpty())
    {
      const std::string& nodeID = sch.get_nodeID();
      const std::string& GPUtype = sch.get_GPUtype();

      // get the number of available GPUs
      unsigned remaining_GPUs = system.get_remainingGPUs(GPUtype, nodeID);

      // if the node is not full
      if (remaining_GPUs > 0)
      {
        // determine the job that would get the highest speed-up by getting the 
        // additional resources
        unsigned new_g = sch.get_nGPUs() + remaining_GPUs;
        setup_time_t::const_iterator new_stp_it = tjvg.find(std::make_tuple(GPUtype,new_g,1.));

        if (new_stp_it != tjvg.cend())
        {
          double delta = sch.get_selectedTime() - new_stp_it->second;

          if (delta > previous_delta)
          {
            previous_delta = delta;
            best_speedup = {schit, new_stp_it};
          }
        }
      }
    }
  }

  // assign the additional resources to the job with highest speed-up
  if (previous_delta > 0.)
  {
    // find the job in the current schedule
    job_schedule_t::iterator schit = best_speedup.first;
    setup_time_t::const_iterator new_stp_it = best_speedup.second;
    
    if (schit != current_schedule.end())
    {
      // determine the previous schedule and configuration
      Schedule& sch = schit->second;
      unsigned previous_g = sch.get_nGPUs();
      
      // get the new setup
      const setup_t& new_stp = new_stp_it->first;
      double new_time = new_stp_it->second;
      unsigned new_g = std::get<1>(new_stp);

      // update the configuration
      system.assign_to_node(sch.get_GPUtype(), new_g - previous_g, 1., false,
                            sch.get_nodeID());
      sch.change_configuration(new_time, new_g);
    }
  }
}
*/

template <typename COMP>
bool
Heuristic<COMP>::to_be_inserted (const K_best_t& Kmap, 
                                 typename K_best_t::iterator ub) const
{
  return (Kmap.empty() || ub != Kmap.cend());
}

template <typename COMP>
void
Heuristic<COMP>::restore_map_size (K_best_t& current_K_best)
{
  typename K_best_t::iterator it = current_K_best.end();
  current_K_best.erase(--it);
}

template <typename COMP>
void
Heuristic<COMP>::update_elite_solutions (const job_schedule_t& new_schedule,
                                         K_best_t& K_best, unsigned K)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging(level);
    std::cout << base << "--- update elite solutions." << std::endl;
  }

  // initialize candidate solution
  Solution BS(new_schedule, system.get_nodes(), current_time);

  // compute the expected cost of the schedule
  double new_total_cost = proxy_function(BS, system.get_GPUcosts(), verbose,
                                         level+1);

  if (verbose > 1)
  {
    if (! K_best.empty())
      std::cout << base <<"\tcurrent optimal cost: " << K_best.cbegin()->first 
                <<"; ";
    else
      std::cout << base << "\t";
    std::cout << "new proposed cost: " << new_total_cost << std::endl;
  }

  // check if current solution is one of the best schedules
  typename K_best_t::iterator ub = K_best.upper_bound(new_total_cost);
  if (to_be_inserted(K_best, ub))
  {
    // insert the new solution in the map
    K_best.insert(ub, std::pair<double,Solution>(new_total_cost, BS));

    if (verbose > 1)
      std::cout << base << "\t\t---> INSERTED";

    // if the number of elite solutions exceeds the maximum value K, restore 
    // the map size
    if (K_best.size() > K)
    {
      restore_map_size(K_best);

      if (verbose > 1)
        std::cout << base << " ---> REMOVED LAST ELEMENT";
    }
  }
  if (verbose > 1)
      std::cout << std::endl;
}

template <typename COMP>
void
Heuristic<COMP>::init (const System& s, const obj_function_t& pf)
{
  system = s;
  proxy_function = pf;
}


/*
*   specializations of template classes
*/
template class Heuristic<std::less<double>>;
template class Heuristic<std::greater<double>>;
