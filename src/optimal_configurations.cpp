#include "optimal_configurations.hpp"

Optimal_configurations::Optimal_configurations (const Job& j, 
                                                const setup_time_t& tjvg, 
                                            const gpu_catalogue_ptr& GPU_costs, 
                                                double ct)
{
  setup_time_t::const_iterator cit;
  for (cit = tjvg.cbegin(); cit != tjvg.cend(); ++cit)
  {
    const setup_t& stp = cit->first;
    double t = cit->second;
    double GPU_cost = GPU_costs->get_cost(std::get<0>(stp), std::get<1>(stp));
    GPU_cost *= std::get<2>(stp);
    if (GPU_cost < INF)
    {
      if (t + ct <= j.get_deadline())
      {
        double cost = GPU_cost * t;
        optConfig_j.insert({cost, cit});
      }
      else
        optConfig_j_C.insert({t, cit});
    }
  }
}

setup_time_t::const_iterator
Optimal_configurations::get_best_setup (void)
{
  assert(! is_end());

  setup_time_t::const_iterator cit;

  if (! optConfig_j.empty())
    cit = random_select(optConfig_j, generator, alpha);
  else
    cit = random_select(optConfig_j_C, generator, alpha);
  
  return cit;
}

bool
Optimal_configurations::is_end (void) const
{
  return (optConfig_j.empty() && optConfig_j_C.empty());
}

void
Optimal_configurations::print (std::ostream& ofs) const
{
  ofs << "optConfig_j" << std::endl;
  ofs << "cost, GPUtype, nGPUs, GPUf, time" << std::endl;
  for (const_iterator cit = optConfig_j.cbegin(); 
       cit != optConfig_j.cend(); ++cit)
  {
    ofs << cit->first << ",";
    setup_time_t::const_iterator cit2 = cit->second;
    ofs << std::get<0>(cit2->first) << "," << std::get<1>(cit2->first) << ","
        << std::get<2>(cit2->first) << "," << cit2->second << std::endl;
  }

  ofs << "\noptConfig_j_C" << std::endl;
  ofs << "cost, GPUtype, nGPUs, GPUf, time" << std::endl;
  for (const_iterator cit = optConfig_j_C.cbegin(); 
       cit != optConfig_j_C.cend(); ++cit)
  {
    ofs << cit->first << ",";
    setup_time_t::const_iterator cit2 = cit->second;
    ofs << std::get<0>(cit2->first) << "," << std::get<1>(cit2->first) << ","
        << std::get<2>(cit2->first) << "," << cit2->second << std::endl;
  }
}
