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

#ifndef EDF_HH
#define EDF_HH

#include "first_principle_methods.hpp"
#include "FIFO.hpp"

class EDF: public First_principle_methods {

protected:
  /* compare
  *   return true if the 1st job's desdline is lower than the 2nd's
  */
  bool compare (const Job&, const Job&) override;

public:
  /*  constructor
  *
  *   Input:  const std::string&    full path of data directory
  *           const std::string&    name of file with the list of jobs
  *           const std::string&    name of file with execution times of jobs
  *           const std::string&    name of file with the list of nodes
  *
  */
  EDF (const std::string&, const std::string&, const std::string&,
       const std::string&);

  /* destructor
  */
  virtual ~EDF (void) = default;
};

#endif /* EDF_HH */