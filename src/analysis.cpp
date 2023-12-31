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

#include "analysis.hpp"

Analysis::Analysis (const std::string& d): directory(d) {}

void
Analysis::perform_analysis (const std::string& filename)
{
  std::string file = directory + "/" + filename;
  std::ifstream ifs(file);
  if (ifs)
  {
    double sumCost = 0.;
    double sumTardi = 0.;
    
    table_t schedule;
    read_csv(ifs, schedule);
    
    unsigned iter_idx = 0;
    unsigned sim_time_idx = 1;
    unsigned nodes_idx = 17;
    unsigned VM_idx = 18;
    unsigned tardi_idx = 22;
    unsigned cost_idx = 25;

    for (const row_t& row : schedule)
    {
      // current iteration number
      unsigned iter = std::stoi(row[iter_idx]);
      
      if(iter > slot_number)
      {
        ++slot_number;
        sim_times.push_back(std::stod(row[sim_time_idx]));
        costs.push_back(0.);
        tardiness.push_back(0.);
        VM_types.push_back(std::unordered_set<std::string>());
        nodes.push_back(std::unordered_set<std::string>());
      }

      double cost = std::stod(row[cost_idx]);
      double tardi = std::stod(row[tardi_idx]);
      costs[slot_number-1] += cost;
      tardiness[slot_number-1] += tardi;
      if (row[VM_idx] != "" && row[nodes_idx] != "")
      {
        VM_types[slot_number-1].insert(row[VM_idx]);
        nodes[slot_number-1].insert(row[nodes_idx]);
      }
      sumCost += cost;
      sumTardi += tardi;
    }
    std::cout << "total cost: " << sumCost << "; tardiness: " 
            << sumTardi << std::endl;
  }
  else
    std::cerr << "ERROR in Analysis::perform_analysis - file "
              << file << " cannot be opened" << std::endl;
}

void
Analysis::print_costs (const std::string& method_name, unsigned seed) const
{
  std::string filename = directory + "/cost-" + method_name;
  if (seed > 1)
    filename += ("_" + std::to_string(seed));
  filename += ".csv";
  
  std::ofstream ofs(filename);
  if (ofs)
  {
    ofs << "slot_number,sim_time,VM_type,NodeID,cost/node\n";
    for (unsigned iter = 0; iter < slot_number; ++iter)
    {
      ofs << (iter+1) << "," << sim_times[iter] << ",\"";
      const std::unordered_set<std::string>& vms = VM_types[iter];
      for (const std::string& s : vms)
        ofs << s << ",";
      ofs << "\",\"";
      const std::unordered_set<std::string>& nns = nodes[iter];
      for (const std::string& s : nns)
        ofs << s << ",";
      ofs << "\",";
      ofs << costs[iter] << "\n";
    }  
  }
  else
    std::cerr << "ERROR in print_costs: file " << filename
              << " cannot be opened" << std::endl;
}

void
Analysis::print_tardiness (const std::string& method_name, unsigned seed) const
{
  std::string filename = directory + "/tardi-" + method_name;
  if (seed > 1)
    filename += ("_" + std::to_string(seed));
  filename += ".csv";
  
  std::ofstream ofs(filename);
  if (ofs)
  {
    ofs << "slot_number,sim_time,tardiness\n";
    for (unsigned iter = 0; iter < slot_number; ++iter)
    {
      ofs << (iter+1) << "," << sim_times[iter] << "," << tardiness[iter] 
          << "\n";
    }
  }
  else
    std::cerr << "ERROR in print_costs: file " << filename
              << " cannot be opened" << std::endl;
}

void
Analysis::print (const std::string& method_name, unsigned seed) const
{
  print_costs(method_name, seed);
  print_tardiness(method_name, seed);
}