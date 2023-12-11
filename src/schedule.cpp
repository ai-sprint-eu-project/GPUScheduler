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

#include "schedule.hpp"

Schedule::Schedule (setup_time_t::const_iterator s, unsigned n):
  csetup(s), node_idx(n), empty_schedule(false)
{}

const Setup&
Schedule::get_setup (void) const
{
  assert(! empty_schedule);
  return csetup->first;
}

double 
Schedule::get_selectedTime (void) const 
{
  double t = std::numeric_limits<double>::infinity();
  if (! empty_schedule)
    t = csetup->second;
  return t;
}

unsigned
Schedule::get_node_idx (void) const
{
  assert(! empty_schedule);
  return node_idx;
}

void
Schedule::set_tardiness (double t) 
{
  tardiness = t;
  if (tardiness < TOL)
    tardinessCost = 0.0;
}

void
Schedule::compute_vmCost (unsigned g)
{
  if (! empty_schedule)
  {
    vmCost = execution_time * (csetup->first).get_cost() / 3600 * 
             (csetup->first).get_nGPUs() / g;
  }
}

void
Schedule::compute_tardinessCost (double tw)
{
  if (! empty_schedule)
    tardinessCost = tardiness * tw;
}

void
Schedule::print_names (std::ostream& ofs, char endline)
{
  ofs << "n_iterate,sim_time,";
  Job::print_names(ofs,',');
  ofs << "ExecutionTime,CompletionPercent,StartTime,FinishTime,";
  Node::print_names(ofs,',');
  Setup::print_names(ofs,',');
  ofs << "Tardiness,VMcost,TardinessCost,TotalCost" << endline;
}

void
Schedule::print (const Job& j, const std::vector<Node>& nodes, 
                 std::ostream& ofs, char endline) const
{
  ofs << iter << ',' << sim_time << ',';
  j.print(ofs,',');
  ofs << execution_time << ',' << completion_percent << ',' 
      << start_time     << ',' << finish_time        << ',';
  if (! empty_schedule)
  {
    nodes[node_idx].print(ofs,',');
    (csetup->first).print(ofs,',');
  }
  else
  {
    Node n;
    n.print(ofs, ',');
    Setup s;
    s.print(ofs,',');
  }
  ofs << tardiness << ',' << vmCost << ',' << tardinessCost << ',' 
      << (vmCost+tardinessCost) << endline;
}

bool
operator== (const Schedule& s1, const Schedule& s2)
{
  bool equal = false;

  // if the schedules are not empty, check if the corresponding setups and
  // number of GPUs are equal
  if (! s1.empty_schedule && ! s2.empty_schedule)
  {
    const Setup& stp1 = s1.csetup->first;
    const Setup& stp2 = s2.csetup->first;

    if (stp1.get_nGPUs() == stp2.get_nGPUs())
    {
      equal = (stp1 == stp2);
    }
  }
  // if both schedules are empty, the "setup" is still considered the same
  else if (s1.isEmpty() && s2.isEmpty())
    equal = true;

  return equal;
}

bool
operator!= (const Schedule& s1, const Schedule& s2)
{
  return ! (s1 == s2);
}
