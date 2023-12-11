#ifndef STOCHASTIC_HH
#define STOCHASTIC_HH

#include "heuristic.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

using namespace restc_cpp;

// type definitions
typedef std::pair<setup_t, double> setup_time;
typedef std::multimap<double, setup_time> st_setup_time_t;

struct Sol {
  double obj;
  std::string tc;
  std::vector<double> x;
};
BOOST_FUSION_ADAPT_STRUCT(Sol, obj, tc, x)

struct Input {
  std::vector<double> s;
  std::vector<double> p;
  double d;
  double e;
  double max_e;
  std::string distribution;
};
BOOST_FUSION_ADAPT_STRUCT(Input, s, p, d, e, max_e, distribution)

template <typename COMP = std::less<double>>

class Stochastic: public Heuristic<COMP> {

protected:  
  /* assign_to_suboptimal:
  *   assign the given job to a suboptimal configuration, available in 
  *   an already opened node
  *
  *   Input:  const Job&                      job to be assigned
  *           const setup_time_t&             execution times of the given job
  *           Optimal_configurations&         see optimal_configurations.hpp
  *           job_schedule_t&                 new proposed schedule
  *
  *   Output: bool                            true if job has been assigned
  */
  virtual bool assign_to_suboptimal (const Job&, const setup_time_t&, Optimal_configurations&, 
                                     job_schedule_t&) override;

  using Heuristic<COMP>::assign;

  /* assign:
  *   assign the given job to the required node
  *
  *   Input:  const Job&                      job to be assigned
  *           st_setup_time_t&                refernce to optimal setup
  *           job_schedule_t&                 new proposed schedule
  *
  *   Output: bool                            true if job has been assigned
  */
  bool assign (const Job&, st_setup_time_t&, job_schedule_t&);

  //commento
  st_setup_time_t stochastic_scheduling (const Job& j, const setup_time_t& tjvg,
                                         const gpu_catalogue_ptr& GPU_costs, 
                                         double ct);

  /* perform_assignment 
  *   assign the selected job to the new proposed schedule
  *
  *   Input:  const Job&                      job to be assigned
  *           job_schedule_t&                 new proposed schedule
  *
  *   Output: bool                            true if job has been assigned
  */
  virtual bool perform_assignment (const Job&, job_schedule_t&) override;

public:
  /*  constructor
  *
  *   Input:  const System&                   system
  *           const obj_function_t&           proxy function
  *           unsigned                        level for logging
  */
  Stochastic (const System&, const obj_function_t&, unsigned);

  /* perform_scheduling
  *   perform the scheduling process that assigns the best configuration to
  *   each submitted jobs (as long as resources are available)
  *
  *   Input:    double                        current time
  *             Elite_solutions<COMP>&        empty map of elite solutions
  *             unsigned                      max number of elite solutions
  *             unsigned                      verbosity level
  */
  virtual void perform_scheduling (double, Elite_solutions<COMP>&, unsigned, 
                                   unsigned = 0) override;
  
  /* destructor
  */
  virtual ~Stochastic (void) = default;

};

#endif /* STOCHASTIC_HH */