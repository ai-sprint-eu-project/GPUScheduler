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

#include "Priority.hpp"

Priority::Priority (const std::string& directory, const std::string& file_jobs, 
                    const std::string& file_times, 
                    const std::string& file_nodes):
  First_principle_methods(directory, file_jobs, file_times, file_nodes)
{}

bool
Priority::compare(const Job& j1, const Job& j2)
{
  return j1.get_tardinessWeight() > j2.get_tardinessWeight();
}