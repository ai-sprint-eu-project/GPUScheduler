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

#include "setup.hpp"

Setup::Setup (const row_t& info)
{
  typename row_t::const_iterator it = info.cbegin();
  VMtype = *it++;
  GPUtype = *it++;
  nGPUs = std::stoi(*it++);
  max_nGPUs = std::stoi(*it++);
  cost = std::stod(*it);
}

Setup::Setup (const std::string& vm, const std::string& gpu):
  VMtype(vm), GPUtype(gpu)
{}

void
Setup::print_names (std::ostream& ofs, char endline)
{
  ofs << "VMType,GpuType,GpuNumber,cost" << endline;
}

void
Setup::print (std::ostream& ofs, char endline) const
{
  ofs << VMtype << "," << GPUtype << "," << nGPUs << "," 
      << cost << endline;
}

bool
operator== (const Setup& s1, const Setup& s2)
{
  return (s1.VMtype == s2.VMtype && s1.GPUtype == s2.GPUtype);
}

bool operator!= (const Setup& s1, const Setup& s2)
{
  return ! (s1 == s2);
}