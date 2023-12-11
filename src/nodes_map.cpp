#include "nodes_map.hpp"

Nodes_map::Nodes_map (const std::string& filename)
{
  n_nodes = load_nodes_list(nodes, filename);
}

gpus_t::iterator
Nodes_map::find_in_list (gpus_t& gpus_list, const std::string& ID,
                         unsigned GPU_ID)
{
  gpus_t::iterator found_it = gpus_list.end();

  // loop over the elements in the list...
  for (gpus_t::iterator it2 = gpus_list.begin();
       it2 != gpus_list.end() && found_it == gpus_list.end(); ++it2)
  {
    // ...until you find the one with the given ID, if it exists
    if (std::get<1>(*it2) == ID && std::get<2>(*it2) == GPU_ID)
      found_it = it2;
  }

  return found_it;
}

nodes_t::iterator
Nodes_map::find_in_list (nodes_t& nodes_list, const std::string& ID)
{
  nodes_t::iterator found_it = nodes_list.end();
  
  // loop over the elements in the list...
  for (nodes_t::iterator it2 = nodes_list.begin();
       it2 != nodes_list.end() && found_it == nodes_list.end(); ++it2)
  {
    // ...until you find the one with the given ID, if it exists
    if (it2->get_ID() == ID)
      found_it = it2;
  }
  
  return found_it;
}

nodes_t::const_iterator
Nodes_map::find_in_list (const nodes_t& nodes_list,const std::string& ID) const
{
  nodes_t::const_iterator found_it = nodes_list.cend();
  
  // loop over the elements in the list...
  for (nodes_t::const_iterator it2 = nodes_list.cbegin();
       it2 != nodes_list.cend() && found_it == nodes_list.cend(); ++it2)
  {
    // ...until you find the one with the given ID, if it exists
    if (it2->get_ID() == ID)
      found_it = it2;
  }
  
  return found_it;
}

std::pair<bool, unsigned>
Nodes_map::get_usedGPUs (const nodes_map_t& nmap, const std::string& GPUtype, 
                         const std::string& node_ID) const
{
  bool found = false;
  unsigned used_GPUs = 0;

  // look among full nodes
  nodes_map_t::const_iterator it1 = nmap.find(GPUtype);
  if (it1 != nmap.cend())
  {
    const nodes_t& nodes_list = it1->second;
    nodes_t::const_iterator it2 = find_in_list(nodes_list, node_ID);
    found = (it2 != nodes_list.cend());
    if (found)
      used_GPUs = it2->get_usedGPUs();
  }

  return {found, used_GPUs};
}

void
Nodes_map::compute_nodeCost (double elapsed_time, const nodes_map_t& nmap,
                             double& nodeCost) const
{
  for (nodes_map_t::const_iterator it1 = nmap.cbegin();
       it1 != nmap.cend(); ++it1)
  {
    for (nodes_t::const_iterator it2 = (it1->second).cbegin();
         it2 != (it1->second).cend(); ++it2)
    {
      nodeCost += (it2->get_cost() * elapsed_time / 3600);
    }
  }
}

std::pair<bool, gpus_t::iterator>
Nodes_map::free_resources (const Node& n, gpus_map_t& gmap,
                           std::unordered_set<std::string>& updated_gpus,
                           const std::string& message)
{
  bool updated = false;
  gpus_t::iterator it2;

  // get node ID, GPU type and GPU fraction
  const std::string& nID = n.get_ID();
  const std::string& GPUtype = n.get_GPUtype();
  double f = n.get_GPUf();
  unsigned GPU_ID = n.get_GPUID();

  // loop over the given map
  gpus_map_t::iterator it1 = gmap.find(GPUtype);
  if (it1 != gmap.end())
  {
    gpus_t& shared_gpus_list = it1->second;
    it2 = find_in_list(shared_gpus_list, nID, GPU_ID);
    updated = (it2 != shared_gpus_list.cend());
    if (updated)
    {
      if (verbose > 0)
        std::cout << "; " << message << "; GPU fraction = " << f;

      // ...add the GPU to the set of updated GPUs
      std::string nGID = nID + "_" + std::to_string(GPU_ID);
      updated_gpus.insert(nGID);

      // ...free the required GPU fraction
      std::get<0>(*it2) += f;

      if (verbose > 0)
        std::cout << "; new GPU fraction = " << std::get<0>(*it2)
                  << std::endl;
    }
  }

  return {updated, it2};
}

std::pair<bool, nodes_t::iterator>
Nodes_map::free_resources (const Node& n, nodes_map_t& nmap, 
                           std::unordered_set<std::string>& updated_nodes,
                           const std::string& message)
{
  bool updated = false;
  nodes_t::iterator it2;

  // get node ID, GPU type and number of used GPUs
  const std::string& nID = n.get_ID();
  const std::string& GPUtype = n.get_GPUtype();
  unsigned g = n.get_nGPUs();

  // loop over the given map
  nodes_map_t::iterator it1 = nmap.find(GPUtype);
  if (it1 != nmap.end())
  {
    nodes_t& nodes_list = it1->second;
    it2 = find_in_list(nodes_list, nID);
    updated = (it2 != nodes_list.cend());
    if (updated)
    {
      if (verbose > 0)
        std::cout << "; " << message << "; n_used_GPUs = " << g;

      // ...add the node to the set of updated nodes
      updated_nodes.insert(nID);
      
      // ...free the required number of GPUs
      it2->free_GPUs(g);

      if (verbose > 0)
        std::cout << "; new n_used_GPUs = " << it2->get_usedGPUs() 
                  << std::endl;
    }
  }

  return {updated, it2};
}

void
Nodes_map::print (std::ostream& ofs, const nodes_map_t& nmap, 
                  const std::string& is_full) const
{
  for (nodes_map_t::const_iterator it1 = nmap.cbegin();
       it1 != nmap.cend(); ++it1)
  {
    const nodes_t& nodes_list = it1->second;
    for (nodes_t::const_iterator it2 = nodes_list.cbegin();
         it2 != nodes_list.cend(); ++it2)
    {
      it2->print(ofs, ',');
      ofs << is_full << "\n";
    }
  }
}

void
Nodes_map::init (const std::string& filename)
{
  n_nodes = load_nodes_list(nodes, filename);
}

Node&
Nodes_map::find_node (const std::string& ID, const std::string& GPUtype)
{
  // if a GPU type is specified...
  if (! GPUtype.empty())
  {
    // look for the node in the set of available nodes
    //
    // find the list of nodes with the given GPU type
    nodes_map_t::iterator it1 = nodes.find(GPUtype);
    if (it1 != nodes.end())
    {
      // if it exists, look for the required ID in the list
      nodes_t& nodes_list = it1->second;
      nodes_t::iterator found_it = find_in_list(nodes_list, ID);
      if (found_it != nodes_list.end())
        return *found_it;
    }

    // if the node has not been found, look for it in the set of full nodes
    it1 = full_nodes.find(GPUtype);
    if (it1 != full_nodes.end())
    {
      // if it exists, look for the required ID in the list
      nodes_t& nodes_list = it1->second;
      nodes_t::iterator found_it = find_in_list(nodes_list, ID);
      if (found_it != nodes_list.end())
        return *found_it;
    }
  }
  else
  {
    // if the GPU type is not specified, loop over all the existing GPU types

    // look for the node in the set of available nodes
    //
    for (nodes_map_t::iterator it1 = nodes.begin(); it1 != nodes.end(); ++it1)
    {
      // look for the given node ID in the corresponding list
      nodes_t& nodes_list = it1->second;
      nodes_t::iterator found_it = find_in_list(nodes_list, ID);
      if (found_it != nodes_list.end())
        return *found_it;
    }

    // if the node has not been found, look for it in the set of full nodes
    for (nodes_map_t::iterator it1 = full_nodes.begin(); 
         it1 != full_nodes.end(); ++it1)
    {
      // look for the given node ID in the corresponding list
      nodes_t& nodes_list = it1->second;
      nodes_t::iterator found_it = find_in_list(nodes_list, ID);
      if (found_it != nodes_list.end())
        return *found_it;
    }
  }

  // if the required GPU type is not in the map, or if the required node ID 
  // cannot be found, return non-existing
  return non_existing_node;
}

const Node&
Nodes_map::find_node (const std::string& ID, const std::string& GPUtype) const
{
  // if a GPU type is specified...
  if (! GPUtype.empty())
  {
    // look for the node in the set of available nodes
    //
    // find the list of nodes with the given GPU type
    nodes_map_t::const_iterator it1 = nodes.find(GPUtype);
    if (it1 != nodes.cend())
    {
      // if it exists, look for the required ID in the list
      const nodes_t& nodes_list = it1->second;
      nodes_t::const_iterator found_it = find_in_list(nodes_list, ID);
      if (found_it != nodes_list.cend())
        return *found_it;
    }

    // if the node has not been found, look for it in the set of full nodes
    it1 = full_nodes.find(GPUtype);
    if (it1 != full_nodes.cend())
    {
      // if it exists, look for the required ID in the list
      const nodes_t& nodes_list = it1->second;
      nodes_t::const_iterator found_it = find_in_list(nodes_list, ID);
      if (found_it != nodes_list.cend())
        return *found_it;
    }
  }
  else
  {
    // if the GPU type is not specified, loop over all the existing GPU types

    // look for the node in the set of available nodes
    //
    for (nodes_map_t::const_iterator it1 = nodes.cbegin(); 
         it1 != nodes.cend(); ++it1)
    {
      // look for the given node ID in the corresponding list
      const nodes_t& nodes_list = it1->second;
      nodes_t::const_iterator found_it = find_in_list(nodes_list, ID);
      if (found_it != nodes_list.cend())
        return *found_it;
    }

    // if the node has not been found, look for it in the set of full nodes
    for (nodes_map_t::const_iterator it1 = full_nodes.cbegin(); 
         it1 != full_nodes.cend(); ++it1)
    {
      // look for the given node ID in the corresponding list
      const nodes_t& nodes_list = it1->second;
      nodes_t::const_iterator found_it = find_in_list(nodes_list, ID);
      if (found_it != nodes_list.cend())
        return *found_it;
    }
  }

  // if the required GPU type is not in the map, or if the required node ID 
  // cannot be found, return non-existing
  return non_existing_node;
}

bool
Nodes_map::isFull (const std::string& GPUtype,const std::string& node_ID) const
{
  bool full = false;
  nodes_map_t::const_iterator it1 = full_nodes.find(GPUtype);
  if (it1 != full_nodes.cend())
  {
    const nodes_t& nodes_list = it1->second;
    nodes_t::const_iterator it2 = find_in_list(nodes_list, node_ID);
    full = (it2 != nodes_list.cend());
  }
  return full;
}

unsigned
Nodes_map::get_remainingGPUs (const std::string& GPUtype, 
                              const std::string& node_ID) const
{
  unsigned remainingGPUs = 0;
  nodes_map_t::const_iterator it1 = nodes.find(GPUtype);
  if (it1 != nodes.cend())
  {
    const nodes_t& nodes_list = it1->second;
    nodes_t::const_iterator it2 = find_in_list(nodes_list, node_ID);
    if (it2 != nodes_list.cend())
      remainingGPUs = it2->get_remainingGPUs();
  }
  return remainingGPUs;
}

unsigned
Nodes_map::get_usedGPUs (const std::string& GPUtype, 
                         const std::string& node_ID) const
{
  // look among full nodes
  std::pair<bool, unsigned> pair = get_usedGPUs(full_nodes, GPUtype, node_ID);

  // if the node was not full, look among available nodes
  if (! pair.first)
    pair = get_usedGPUs(nodes, GPUtype, node_ID);

  return pair.second;
}

void
Nodes_map::add (const Node& n)
{
  nodes[n.get_GPUtype()].push_back(n);
  nodes.at(n.get_GPUtype()).sort();
  n_nodes++;
}

void
Nodes_map::add_full_node (const Node& n)
{
  full_nodes[n.get_GPUtype()].push_back(n);
  n_full_nodes++;
}
  
void
Nodes_map::add_in_order (const Node& n, nodes_t::iterator pos)
{
  const std::string& GPUtype = n.get_GPUtype();
  nodes_map_t::iterator it1 = nodes.find(GPUtype);
  if (it1 != nodes.end())
  {
    nodes_t& nodes_list = it1->second;
    nodes_list.insert(pos, n);
  }
  else
    nodes[GPUtype].push_back(n);
}

unsigned
Nodes_map::count_open_nodes (void) const
{
  unsigned n_open_nodes = 0;

  // loop over all nodes
  for (nodes_map_t::const_iterator it1 = nodes.cbegin(); 
       it1 != nodes.cend(); ++it1)
  {
    const nodes_t& nodes_list = it1->second;
    for (nodes_t::const_iterator it2 = nodes_list.cbegin();
         it2 != nodes_list.cend(); ++it2)
    {
      // increment the counter if the node is open
      if (it2->isOpen())
        n_open_nodes++;
    }
  }

  return (n_open_nodes + n_full_nodes);
}

double
Nodes_map::compute_nodeCost (double elapsed_time) const
{
  double nodeCost = 0.;

  // compute cost of full nodes
  compute_nodeCost(elapsed_time, full_nodes, nodeCost);

  // compute cost of used nodes
  compute_nodeCost(elapsed_time, nodes, nodeCost);

  return nodeCost;
}

std::string
Nodes_map::assign_to_GPU (const std::string& GPUtype, double f,
                          const std::string& node_ID, unsigned GPU_ID)
{
  std::string ID = node_ID + "_" + std::to_string(GPU_ID);
  gpu_t GPU = std::make_tuple(1-f, node_ID, GPU_ID);

  gpus_map_t::iterator it1 = shared_gpus.find(GPUtype);

  // if no shared GPU of type GPUtype already exists, insert and end
  if (it1 == shared_gpus.end())
    shared_gpus[GPUtype].push_back(GPU);
  // otherwise insert in the right place in the list
  else
  {
    gpus_t& shared_gpus_list = it1->second;
    bool inserted = false;

    if (f > 0.5)
      for (gpus_t::iterator it2 = shared_gpus_list.begin();
           it2 != shared_gpus_list.end() && inserted == false; ++it2)
        {
          if (1-f >= std::get<0>(*it2))
          {
            shared_gpus_list.insert(it2, GPU);
            inserted = true;
          }
        }
    if (f <= 0.5 || inserted == false)
      shared_gpus_list.push_back(GPU);
  }

  return ID;
}

std::string
Nodes_map::assign_to_GPU (const std::string& GPUtype, double f,
                          const std::string& node_ID)
{
  std::string ID = "";

  // find the list of shared GPUs with the required GPU type
  gpus_map_t::iterator it1 = shared_gpus.find(GPUtype);
  if (it1 != shared_gpus.end())
  {
    gpus_t& shared_gpus_list = it1->second;
    gpus_t::iterator found_it = shared_gpus_list.end();
    std::map<double, gpus_t::iterator> prev_its;

    // find the GPU with the minimum sufficient number of available fraction
    for (gpus_t::iterator it2 = shared_gpus_list.begin();
         it2 != shared_gpus_list.end() && found_it == shared_gpus_list.end(); ++it2)
    {
      double current_f_GPU = std::get<0>(*it2);

      // check if the current GPU has enough space and if specified, check
      // whether the node has the correct ID.
      // if not, insert it2 in the vector of iterators to previous elements
      if ((node_ID.empty() || std::get<1>(*it2) == node_ID) && current_f_GPU >= f)
        found_it = it2;
      else
        prev_its.insert(prev_its.end(), {current_f_GPU, it2});
    }

    // if such a GPU exists
    if (found_it != shared_gpus_list.end())
    {
      gpu_t GPU = *found_it;
      ID = std::get<1>(GPU) + "_" + std::to_string(std::get<2>(GPU));
      std::get<0>(GPU) -= f;
      double remained_fraction = std::get<0>(GPU);

      // erase the GPU from the old position ...
      shared_gpus_list.erase(found_it);
      // ... and move it to the correct position or to the full list
      if (remained_fraction > 0.)
      {
        auto pos = prev_its.upper_bound(remained_fraction);
        if (pos != prev_its.end())
          shared_gpus_list.insert(pos->second, GPU);
        else
          shared_gpus_list.push_front(GPU);
      }
      else
        full_shared_gpus[GPUtype].push_back(GPU);
    }
  }

  return ID;
}

std::string 
Nodes_map::assign_to_node (const std::string& GPUtype, unsigned g, double f,
                           bool unique, const std::string& node_ID)
{
  std::string ID = "";

  // find the list of nodes with the required GPU type
  nodes_map_t::iterator it1 = nodes.find(GPUtype);
  if (it1 != nodes.end())
  {
    // if only a fraction of a GPU is required, try to assign to an already
    // open GPU with sufficient available fraction. If such a GPU exists, end
    if (f < 1.)
    {
      ID = assign_to_GPU(GPUtype, f, node_ID);
      if (! ID.empty())
        return ID;
    }

    nodes_t& nodes_list = it1->second;
    nodes_t::iterator found_it = nodes_list.end();
    std::vector<nodes_t::iterator> prev_its;

    // find the node with the minimum sufficient number of available GPUs
    for (nodes_t::iterator it2 = nodes_list.begin();
         it2 != nodes_list.end() && found_it == nodes_list.end(); ++it2)
    {
      unsigned current_r_GPUs = it2->get_remainingGPUs();

      // check if the current node has enough space and if specified, check 
      // whether the node has the correct ID
      if ((node_ID.empty() || it2->get_ID() == node_ID) && current_r_GPUs >= g)
      {
        found_it = it2;
        prev_its.resize(current_r_GPUs, ++it2);
      }
      else
      {
        // if not, insert it2 in the vector of iterators to previous elements
        prev_its.resize(current_r_GPUs, it2);
      }
    }

    // if such a node exists
    if (found_it != nodes_list.end())
    {
      Node n = *found_it;
      ID = n.get_ID();
      n.set_remainingGPUs(g);

      // erase the node from the old position in the list of nodes
      nodes_list.erase(found_it);
      
      // if the node is full, insert it in the corresponding container and
      // update the relative counter
      unsigned remained_GPUs = n.get_remainingGPUs();
      if (unique || remained_GPUs == 0)
      {
        full_nodes[GPUtype].push_back(n);
        n_full_nodes++;
      }
      else
      {
        // otherwise, move it to the correct position in the list
        nodes_list.insert(prev_its[remained_GPUs-1], n);
      }

      // insert the just opened GPU with the remaining fraction in the list of
      // shared GPUs
      if (f < 1.)
      {
        shared_counter_t::iterator it3 = n_shared_gpus.insert({ID, 0}).first;
        unsigned GPU_ID = ++(it3->second);
        ID = assign_to_GPU(GPUtype, f, ID, GPU_ID);
      }
    }
  }
  else
    std::cerr << "ERROR in Nodes_map::assign_to_node --- GPU type " << GPUtype 
              << " does not exist" << std::endl;

  return ID;
}

void
Nodes_map::free_resources (const jn_pairs_t& ended_jobs, unsigned v)
{
  verbose = v;

  // initialize set of already updated nodes and GPUs
  std::unordered_set<std::string> updated_gpus;
  std::unordered_set<std::string> updated_nodes;

  // loop over pairs of ended jobs and corresponding nodes
  for (const jn_pair_t& jn_pair : ended_jobs)
  {
    // get node and corresponding ID, GPU type
    const Node& n = jn_pair.second;
    const std::string& nID = n.get_ID();
    const std::string& GPUtype = n.get_GPUtype();

    if (verbose > 0)
        std::cout << "\tJob " << jn_pair.first.get_ID() 
                  << " ended on " << nID;

    bool updated = false;
    bool emptied_shared_gpu = false;

    // determine if j was assigned to a fraction of a GPU
    if (n.get_GPUf() < 1.)
    {
      // if the GPU has never been updated...
      std::string nGID = nID + "_" + std::to_string(n.get_GPUID());
      std::unordered_set<std::string>::iterator it0 = updated_gpus.find(nGID);
      if (it0 == updated_gpus.cend())
      {
        // ...look for the GPU in the set of full shared GPUs
        std::pair<bool,gpus_t::iterator> pair = free_resources(n, full_shared_gpus,
                                                                updated_gpus,
                                                                "GPU was full");
        updated = pair.first;
        if (updated)
        {
          shared_gpus[GPUtype].push_back(*pair.second);
          shared_gpus.at(GPUtype).sort();
          full_shared_gpus.at(GPUtype).erase(pair.second);
        }
      }

      // if the GPU was not full or if it has already been updated at least
      // once...
      if (! updated)
      {
        // ...look for the GPU in the set of available shared GPUs
        std::pair<bool,gpus_t::iterator> pair = free_resources(n, shared_gpus,
                                                                updated_gpus,
                                                                "GPU was open");

        // ... if the GPU is now empty, erase from shared_gpus list
        updated = pair.first;
        if (updated)
        {
          emptied_shared_gpu = (std::get<0>(*pair.second) >= 1.);
          if (emptied_shared_gpu)
          {
            shared_gpus.at(GPUtype).erase(pair.second);
          }
          else
            shared_gpus.at(GPUtype).sort();
        }
      }
    }

    // if j was not assigned to a fraction or was the only one assigned to
    // a shared GPU
    if( n.get_GPUf() == 1. || emptied_shared_gpu)
    {
      // if the node has never been updated...
      std::unordered_set<std::string>::iterator it0 = updated_nodes.find(nID);
      if (it0 == updated_nodes.cend())
      {
        // ...look for the node in the set of full nodes
        std::pair<bool,nodes_t::iterator> pair = free_resources(n, full_nodes,
                                                                updated_nodes,
                                                                "node was full");

        // ...move the node to the set of available nodes
        updated = pair.first;
        if (updated)
        {
          add(*(pair.second));
          n_nodes--;
          full_nodes.at(GPUtype).erase(pair.second);
          n_full_nodes--;
        }
      }

      // if the node was not full or if it has already been updated at least
      // once...
      if (! updated)
      {
        // ...look for the node in the set of available nodes
        std::pair<bool,nodes_t::iterator> pair = free_resources(n, nodes,
                                                                updated_nodes,
                                                                "node was open");

        // ...sort the corresponding list
        updated = pair.first;
        if (updated)
          nodes.at(GPUtype).sort();
      }
    }
  }
}

void
Nodes_map::close_nodes (void)
{
  // transfer full nodes to available nodes
  for (nodes_map_t::iterator it = full_nodes.begin(); 
       it != full_nodes.end(); ++it)
  {
    std::list<Node>& nodes_list = nodes.at(it->first);
    nodes_list.splice(nodes_list.begin(), it->second);
  }
  
  // loop over all nodes
  for (nodes_map_t::iterator it = nodes.begin(); it != nodes.end(); ++it)
  {
    std::list<Node>& nodes_list = it->second;

    // free GPUs in all nodes
    for (Node& n : nodes_list)
      n.free_GPUs(n.get_usedGPUs());
    
    // sort list of nodes according to the number of available GPUs
    nodes_list.sort();
  }

  // clear the two maps related to shared_gpus
  shared_gpus.clear();
  n_shared_gpus.clear();
}

void
Nodes_map::print_names (std::ostream& ofs, char endline)
{
  Node::print_names(ofs, ',');
  ofs << "is_full" << endline;
}

void
Nodes_map::print (std::ostream& ofs) const
{
  // nodes
  print(ofs, nodes, "false");

  // full nodes
  print(ofs, full_nodes, "true");
}
