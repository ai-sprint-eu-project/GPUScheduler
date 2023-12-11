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

#ifndef CONFIGURATION_HH
#define CONFIGURATION_HH

#include "setup.hpp"

class Configuration {

private:
  std::string VMtype = "";
  std::string GPUtype = "";
  double cost = 0.;
  unsigned used_GPUs = 0;
  unsigned remaining_GPUs = 0;

public:
  /*  constructors
  *
  *   Input(1):  void              default
  */
  Configuration (void) = default;

  // getters
  const std::string& get_VMtype (void) const {return VMtype;}
  const std::string& get_GPUtype (void) const {return GPUtype;}
  double get_cost (void) const {return cost;}
  unsigned get_usedGPUs (void) const {return used_GPUs;}
  unsigned get_remainingGPUs (void) const {return remaining_GPUs;}

  // setters
  void set_configuration (const Setup&);
  void delete_configuration (void);
  void update_n_GPUs (int);

  /*  print_names (static)
  *
  *   Input:  std::ostream&       where to print names of fields stored in
  *                               the class    
  *           char endline='\n'   last character to be printed (default \n)
  */
  static void print_names (std::ostream&, char endline='\n');

  /*  print
  *
  *   Input:  std::ostream&       where to print elements stored in the class
  *           char endline='\n'   last character to be printed (default \n)
  */
  void print (std::ostream&, char endline='\n') const;

};

#endif /* CONFIGURATION_HH */