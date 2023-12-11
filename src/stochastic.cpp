#include "stochastic.hpp"

template <typename COMP>
Stochastic<COMP>::Stochastic (const System& s, const obj_function_t& pf, unsigned l):
  Heuristic<COMP>(s, pf, l)
{}

template <typename COMP>
bool
Stochastic<COMP>::assign_to_suboptimal (const Job& j, const setup_time_t& tjvg,
                                        Optimal_configurations& optConfig, 
                                        job_schedule_t& new_schedule)
{
  std::string base = "";
  bool assigned = false;
  while (! optConfig.is_end() && ! assigned)
  {
    // get best setup
    setup_time_t::const_iterator best_stp = optConfig.get_best_setup();

    if (this->verbose > 1)
    {
      base = prepare_logging(this->level);
      std::cout << base << "\t\tnext selected setup: (" 
                << std::get<0>(best_stp->first) << ", "
                << std::get<1>(best_stp->first) << ", " 
                << std::get<2>(best_stp->first) << ")" << std::endl;
    }

    // assign
    assigned = this->assign(j, best_stp, new_schedule);
  }
  return assigned;
}

template <typename COMP>
bool
Stochastic<COMP>::assign (const Job& j, st_setup_time_t& opt_stp,
                          job_schedule_t& new_schedule)
{
  bool assigned = false;

  for (st_setup_time_t::const_iterator cit = opt_stp.cbegin();
       cit != opt_stp.cend() && assigned == false; ++cit)
  {
    const setup_time& stp = cit->second;
    const std::string& required_GPUtype = std::get<0>(stp.first);
    unsigned required_GPUs = std::get<1>(stp.first);
    double required_GPU_f = std::get<2>(stp.first);

    // find a node with the required GPU type and update its number of
    // available GPUs if possible
    std::string node_ID = this->system.assign_to_node(required_GPUtype, required_GPUs,
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
      Schedule sch(node_ID, required_GPUtype, stp.second, 
                   required_GPUs, required_GPU_f, GPU_ID);
      new_schedule[j] = sch;
    }

  }

  return assigned;
}

template <typename COMP>
st_setup_time_t
Stochastic<COMP>::stochastic_scheduling (const Job& j, const setup_time_t& tjvg,
                                         const gpu_catalogue_ptr& GPU_costs, 
                                         double ct)
{
  st_setup_time_t opt_stp;

  Input data;
  data.d = j.get_deadline() - ct;
  data.e = j.get_epochs();
  data.max_e = j.get_maxepochs();
  data.distribution = j.get_distribution();

  unsigned min_gpus = j.get_current_gpus();
  double min_gpu_frac = j.get_current_gpu_frac();
  bool reschedule = j.get_rescheduling_time() - ct <= TOL && min_gpus < 8;

  const catalogue_t& gpu_catalogue = GPU_costs->get_catalogue();
  for (catalogue_t::const_iterator git = gpu_catalogue.cbegin();
       git != gpu_catalogue.cend(); ++git)
  {
    const std::string& GPUtype = git->first;
    data.s.clear();
    data.p.clear();

    setup_t start = std::make_tuple(GPUtype, min_gpus, min_gpu_frac);
    setup_time_t::const_iterator start_it = reschedule ? tjvg.upper_bound(start)
                                            : tjvg.find(start);
    if (start_it == tjvg.cend())
      start_it = tjvg.upper_bound(start);

    for (setup_time_t::const_iterator cit = start_it;
         cit != tjvg.cend() && std::get<0>(cit->first) == GPUtype; ++cit)
    {
      const setup_t& stp = cit->first;
      double t = cit->second;
      double gpu_cost = GPU_costs->get_cost(GPUtype, std::get<1>(stp));
      gpu_cost *= std::get<2>(stp);
      double s_loc = (data.max_e - data.e) / t;
      data.s.push_back(s_loc);
      data.p.push_back(gpu_cost / s_loc);
    }

    // Restc
    std::string url = "http://127.0.0.1:5000/solution";
  	auto rest_client = RestClient::Create();
    Sol sol = rest_client->ProcessWithPromiseT<Sol>([&](Context& ctx)
  	{
  		Sol post;
  		SerializeFromJson(post,
        RequestBuilder(ctx)
          .Put(url)
          .Header("X-Client", "RESTC_CPP")
  				.Data(data)
          .Execute());

      return post;
    })
    .get();

    // get opt_stp from solution
    unsigned g = 1;
    while (sol.x[g] - sol.x[0] < 0.01*data.max_e && g < sol.x.size() - 1)
    {
      start_it++;
      g++;
    }

    double time = (sol.x[g] - sol.x[0]) / data.s[g-1];
    setup_t best_stp = start_it->first;
    opt_stp.insert(std::make_pair(sol.obj, std::make_pair(best_stp, time)));
  }
  
  return opt_stp;
}

template <typename COMP>
bool
Stochastic<COMP>::perform_assignment (const Job& j, job_schedule_t& new_schedule)
{
  std::string base = "";

  time_table_ptr ttime = this->system.get_ttime();
  const setup_time_t& tjvg = ttime->at(j.get_ID());
  
  // determine optimal setups with stochastic method
  st_setup_time_t opt_stp = stochastic_scheduling(j, tjvg,
                              this->system.get_GPUcosts(), this->current_time);

  // assign to the required node
  bool assigned = this->assign(j, opt_stp, new_schedule);

  if (!assigned)
  {
    Optimal_configurations optConfig(j, tjvg, this->system.get_GPUcosts(), 
                                     this->current_time);
    assigned = assign_to_suboptimal(j, tjvg, optConfig, new_schedule);                                 
  }

  // if job j cannot be assigned to any configuration, add an empty schedule
  if (!assigned)
    new_schedule[j] = Schedule();

  return assigned;
}

template <typename COMP>
void
Stochastic<COMP>::perform_scheduling (double ct, Elite_solutions<COMP>& ES, 
                                  unsigned K, unsigned v)
{
  // print info
  this->verbose = v;
  std::string base = "";
  if (this->verbose > 0)
  {
    base = prepare_logging(this->level);
    std::cout << base << "--- perform scheduling." << std::endl;
  }

  // update current time
  this->current_time = ct;

  // sort the list of submitted jobs
  this->sort_jobs_list();

  // compute the schedule
  job_schedule_t best_schedule;
  this->scheduling_step(best_schedule);

  // update the map of elite solutions
  this->update_elite_solutions(best_schedule, ES.get_solutions(), K);

  // print best cost
  std::cout << "\n## best cost = " << ES.get_solutions().cbegin()->first 
            << "\n" << std::endl;
}


/*
* specializations of template classes
*/
template class Stochastic<std::less<double>>;
template class Stochastic<std::greater<double>>;
