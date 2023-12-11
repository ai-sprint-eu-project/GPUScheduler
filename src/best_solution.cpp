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

#include "best_solution.hpp"

Best_solution::Best_solution (const job_schedule_t& sch, 
                              const std::vector<Node>& nodes, 
                              unsigned lni, double fft):
  schedule(sch), open_nodes(nodes), last_node_idx(lni), 
  first_finish_time(fft)
{}