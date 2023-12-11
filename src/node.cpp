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

#include "node.hpp"

const std::string&
Node::get_VMtype (void) const
{
  assert(isOpen);
  return c.get_VMtype();
}
  
const std::string&
Node::get_GPUtype (void) const 
{
  assert(isOpen);
  return c.get_GPUtype();
}

double
Node::get_cost (void) const
{
  assert(isOpen);
  return c.get_cost();
}

unsigned
Node::get_usedGPUs (void) const
{
  assert(isOpen);
  return c.get_usedGPUs();
}

unsigned
Node::get_remainingGPUs (void) const
{
  assert(isOpen);
  return c.get_remainingGPUs();
}

bool
Node::open (void) const
{
  return isOpen;
}

void 
Node::set_remainingGPUs (int g)
{
  c.update_n_GPUs(g);
}

void
Node::change_setup (const Setup& stp)
{
  c.set_configuration(stp);
}

void
Node::open_node (const Setup& stp)
{
  isOpen = true;
  c.set_configuration(stp);
}

void
Node::close_node (void)
{
  isOpen = false;
  c.delete_configuration();
}

void
Node::print_names (std::ostream& ofs, char endline)
{
  ofs << "Nodes" << endline;
}

void
Node::print (std::ostream& ofs, char endline) const
{
  ofs << ID << endline;
}

void
Node::print_open_node (std::ostream& ofs) const
{
  assert(isOpen);
  Node::print_names(ofs,',');
  Configuration::print_names(ofs);
  
  print(ofs,',');
  c.print(ofs);
}

bool
operator== (const Node& n1, const Node& n2)
{
  return (n1.ID == n2.ID);
}