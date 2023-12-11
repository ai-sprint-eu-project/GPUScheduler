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

#ifndef DSTAR_HH
#define DSTAR_HH

#include "utilities.hpp"
#include "setup.hpp"
#include "job.hpp"

#include <cassert>
#include <random>
#include <cmath>

class Dstar {

private:
  // type definitions
  typedef std::multimap<double, setup_time_t::const_iterator> dstar_map_t;
  typedef dstar_map_t::const_iterator const_iterator;

  // setups s.t. job j is executed before deadline (ordered by min cost)
  dstar_map_t D_star_j;

  // setups s.t. job j is NOT executed before deadline (ordered by min 
  // execution time)
  dstar_map_t D_star_j_C;

  // parameter for randomization and random numbers generator
  double alpha = 0.;
  std::default_random_engine generator;

public:
  /*  constructor
  *
  *   input:  const Job&               job j
  *           const setup_time_t&      tjvg (see utilities.hpp for type def)
  *           double                   current time
  *
  */
  Dstar (const Job&, const setup_time_t&, double);

  /* set parameter for randomization
  */
  void set_random_parameter (double a) {alpha = a;}

  /* set random number generator
  */
  void set_generator (const std::default_random_engine& g) {generator = g;}

  /* get random number generator
  */
  const std::default_random_engine& get_generator (void) {return generator;}

  /* determine the best setup:
  *   if it is possible to match deadline, the best setup is the cheapest
  *   otherwise, the best setup is the fastest
  */
  setup_time_t::const_iterator get_best_setup (void);

  /* is_end
  *   return true if D_star_j.empty() AND D_star_j_C.empty()
  */
  bool is_end (void) const;

  /* print
  *   print D_star_j and D_star_j_C
  *
  *   Input:  std::ostream&       where to print the containers
  */
  void print (std::ostream&) const;
};

#endif /* DSTAR_HH */