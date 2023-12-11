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

#ifndef BEST_SOLUTION_HH
#define BEST_SOLUTION_HH

#include "utilities.hpp"
#include "job.hpp"
#include "schedule.hpp"
#include "node.hpp"

class Best_solution {

private:
  job_schedule_t schedule;
  std::vector<Node> open_nodes;
  unsigned last_node_idx;
  double first_finish_time;

public:
  Best_solution (const job_schedule_t&, const std::vector<Node>&, unsigned,
                 double);

  // getters
  const job_schedule_t& get_schedule (void) const {return schedule;}
  job_schedule_t& get_schedule (void) {return schedule;}
  const std::vector<Node>& get_open_nodes (void) const {return open_nodes;}
  std::vector<Node>& get_open_nodes (void) {return open_nodes;}
  unsigned get_last_node_idx (void) const {return last_node_idx;}
  double get_first_finish_time (void) const {return first_finish_time;}
  
};

#endif /* BEST_SOLUTION_HH */