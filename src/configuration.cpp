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

#include "configuration.hpp"

void
Configuration::set_configuration (const Setup& stp)
{
  VMtype = stp.get_VMtype();
  GPUtype = stp.get_GPUtype();
  cost = stp.get_cost();
  used_GPUs = 0;
  remaining_GPUs = stp.get_maxnGPUs();
}

void
Configuration::update_n_GPUs (int g)
{
  used_GPUs += g;
  remaining_GPUs -= g;
}

void
Configuration::delete_configuration (void)
{
  VMtype = "";
  GPUtype = "";
  cost = 0.;
  used_GPUs = 0;
  remaining_GPUs = 0;
}

void
Configuration::print_names (std::ostream& ofs, char endline)
{
  ofs << "VMtype,GPUtype,cost,used_GPUs,remaining_GPUs" << endline;
}

void
Configuration::print (std::ostream& ofs, char endline) const
{
  ofs << VMtype << "," << GPUtype << "," << cost << "," << used_GPUs << ","
      << remaining_GPUs << endline;
}