#include "first_principle_methods.hpp"

template <typename COMP>
First_principle_methods<COMP>::First_principle_methods (const System& s, 
                                                      const obj_function_t& pf,
                                                        unsigned l):
  Heuristic<COMP>(s, pf, l)
{}

template <typename COMP>
bool
First_principle_methods<COMP>::assign_to_suboptimal (const Job& j, 
                                                     const setup_time_t& tjvg,
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
      std::cout << base << "\tnext selected setup: (" 
                << std::get<0>(best_stp->first) << ", "
                << std::get<1>(best_stp->first) << ", "
                << std::get<2>(best_stp->first) << ")" << std::endl;
    }

    // assign
    assigned = this->assign(j, best_stp, new_schedule);

    if (this->verbose > 1 && assigned)
      std::cout << base << "\t\t---> ASSIGNED" << std::endl;
  }
  return assigned;
}

template <typename COMP>
bool
First_principle_methods<COMP>::perform_assignment (const Job& j, 
                                                  job_schedule_t& new_schedule)
{
  std::string base = "";

  // get list of execution times for the given job
  time_table_ptr ttime = this->system.get_ttime();
  const setup_time_t& tjvg = ttime->at(j.get_ID());
  Optimal_configurations optConfig(j, tjvg, this->system.get_GPUcosts(), 
                                   this->current_time);

  // determine the best setup:
  //   if it is possible to match deadline, the best setup is the 
  //   cheapest; otherwise, the best setup is the fastest
  setup_time_t::const_iterator best_stp = optConfig.get_best_setup();

  if (this->verbose > 1)
  {
    base = prepare_logging(this->level);
    std::cout << base << "\tfirst selected setup: (" 
              << std::get<0>(best_stp->first) << ", "
              << std::get<1>(best_stp->first) << ", "
              << std::get<2>(best_stp->first) << ")" << std::endl; 
  }

  // assign to the required node
  bool assigned = this->assign(j, best_stp, new_schedule);

  // if the assignment was not feasible, assign to an available suboptimal 
  // configuration
  if (! assigned)
    assigned = assign_to_suboptimal(j, tjvg, optConfig, new_schedule);
  else
    if (this->verbose > 1)
      std::cout << base << "\t\t---> ASSIGNED" << std::endl;

  // if job j cannot be assigned to any configuration, add an empty schedule
  if (!assigned)
    new_schedule[j] = Schedule();

  return assigned;
}

template <typename COMP>
void
First_principle_methods<COMP>::perform_scheduling (double ct, 
                                                   Elite_solutions<COMP>& ES, 
                                                   unsigned K, unsigned v)
{
  // print info
  this->verbose = v;
  std::string base = "";
  if (this->verbose > 0)
  {
    base = prepare_logging(this->level);
    std::cout << base <<  "--- perform scheduling." << std::endl;
  }

  // update current time
  this->current_time = ct;

  // sort the list of submitted jobs
  sort_jobs_list();

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
template class First_principle_methods<std::less<double>>;
template class First_principle_methods<std::greater<double>>;
