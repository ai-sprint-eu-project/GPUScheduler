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

#include "path_relinking.hpp"

Path_relinking::Path_relinking (const std::string& directory, 
                                const std::string& file_jobs, 
                                const std::string& file_times, 
                                const std::string& file_nodes):
  Random_greedy(directory, file_jobs, file_times, file_nodes)
{}

job_schedule_t 
Path_relinking::perform_scheduling (void)
{
  // initialize costs
  double cost_FG = INF;
  double cost_RG = INF;
  double cost_PR = INF;

  // clear pool of elite solutions
  K_best.clear();

  // (STEP #0)
  preprocessing();

  // initialization of minimum total cost, best schedule and corresponding
  // index by a step of pure greedy
  job_schedule_t full_greedy_schedule;
  Greedy::scheduling_step(full_greedy_schedule);
  update_best_schedule(full_greedy_schedule);
  cost_FG = K_best.cbegin()->first;

  #ifdef SMALL_SYSTEM
    // print full greedy cost
    std::cout << "\n## cost of full greedy schedule = " << cost_FG
              << "\n" << std::endl;
  #endif

  // random iterations
  unsigned original_verbosity_level = verbose;
  for (unsigned random_iter = 1; random_iter < max_random_iter; 
       ++random_iter)
  {
    verbose = 0;

    // determine new schedule
    job_schedule_t new_schedule;
    scheduling_step(new_schedule);

    // update the pool of best solutions
    update_best_schedule(new_schedule);
  }
  cost_RG = K_best.rbegin()->first;
  verbose = original_verbosity_level;

  #ifdef SMALL_SYSTEM
    // print minimum cost and number of used nodes after random iterations
    std::cout << "\n## best cost after random construction = " 
              << cost_RG << "; number of used nodes = " 
              << (K_best.rbegin()->second).get_last_node_idx() << std::endl;
  #endif

  // CORE OF THE PR: perform the PR between elite solutions
  // This function can be moved or put in a cycle to perform
  // different types of path relinking.
  best_fight();

  /*XXX postprocessing? XXX*/

  //get best solution
  Best_solution& BS = K_best.rbegin()->second;
  job_schedule_t BS_sch = BS.get_schedule();
  std::swap(nodes,BS.get_open_nodes());
  last_node_idx = BS.get_last_node_idx();
  cost_PR = K_best.rbegin()->first;

  #ifdef SMALL_SYSTEM
    // print minimum cost after path relinking
    std::cout << "\n## best cost = " << K_best.rbegin()->first << std::endl;
    verbose = original_verbosity_level;
    // print info about the time step
    print_info(cost_FG, cost_RG, cost_PR);
  #endif

  return BS_sch;
}

double
Path_relinking::objective_function (job_schedule_t& new_schedule, 
                                    double elapsed_time,
                                    const std::vector<Node>& opened_nodes)
{
  std::string base = "";
  if (verbose > 0)
  {
    base = prepare_logging();
    std::cout << base << "--- new proxy function." << std::endl;
  }
  
  double vmCost = 0.;
  double tardiCost = 0;
  double sim_time = current_time + elapsed_time;
  double time_gain = 0.;

  // loop over last schedule
  for (job_schedule_t::iterator it = new_schedule.begin();
       it != new_schedule.end(); ++it)
  {
    const Job& j = it->first;
    Schedule& sch = it->second;

    if (verbose > 1)
      std::cout << base << "\tanalyzing job... " << j.get_ID() << std::endl;

    // if schedule is not empty, compute cost of VM and tardiness cost
    if (! sch.isEmpty())
    {
      // set as execution time the total required time
      double finish_time = sch.get_selectedTime();
      sch.set_executionTime(finish_time);
      
      // get number of used GPUs on the current node
      unsigned node_idx = sch.get_node_idx();
      unsigned g = opened_nodes[node_idx].get_usedGPUs();

      // compute cost of VM
      sch.compute_vmCost(g);
      vmCost += time_gain / sch.get_vmCost();
      
      // compute tardiness
      double tardiness = std::max(current_time + finish_time - j.get_deadline(), 
                                  0.);
      sch.set_tardiness(tardiness);
      sch.compute_tardinessCost(j.get_tardinessWeight());
      tardiCost += sch.get_tardinessCost();

      // update time gain
      time_gain += j.get_maxExecTime() / (sch.get_vmCost() + sch.get_tardinessCost());

      if (verbose > 1)
        std::cout << base << "\t\texecution_time = " << elapsed_time
                  << "; node_idx = " << node_idx << "; used_GPUs = " << g
                  << "; time_gain = " << time_gain 
                  << "; vm_cost = " << sch.get_vmCost()
                  << "; tardiness = " << tardiness
                  << "; t_cost = " << sch.get_tardinessCost() << std::endl;
    }
  }

  if (verbose > 0)
    std::cout << base << "\tvmCost: " << vmCost << "; tardiCost: " << tardiCost
              << std::endl;

  return (time_gain);
}
  
void
Path_relinking::restore_map_size (K_best_t& current_K_best)
{
  current_K_best.erase(current_K_best.begin());
}

bool
Path_relinking::to_be_inserted (const K_best_t& Kmap, K_best_t::iterator ub) const
{
  return (ub == Kmap.cend());
}

void
Path_relinking::best_fight (void)
{
  // copy the starting elite pool
  K_best_t Kb = K_best;

  // loop over elite solutions
  for (auto it = Kb.begin(); it != Kb.end(); ++it)
  {
    // get best solution and corresponding nodes and schedule
    Best_solution& FB = K_best.rbegin()->second;
    nodes = FB.get_open_nodes();
    job_schedule_t FB_sch = FB.get_schedule();

    // next elite
    job_schedule_t& next_elite = it->second.get_schedule();

    // path relinking
    explored_moves.clear();
    relinking_phase(FB_sch, next_elite, 0., 0., MAX_DEPTH);
  }
}

void
Path_relinking::relinking_phase (job_schedule_t &source, 
                                 const job_schedule_t &target, 
                                 double relinking_best, double improvement,
                                 unsigned DEPTH)
{
  // define the step (best improvement - move)
  std::pair<double, move_t> step_best = {};
  
  // get the pool of feasible moves
  pool_t moves = get_moves(source, target);
  
  // initial fitness value
  double fitness = 0;

  // if a move is feasible, perform the linking
  if ((! moves.empty()) && DEPTH > 0)
  {
    // consume a relinking move
    DEPTH--; 

    // explore the moves (not permanently)
    for (const move_t& move : moves)
    {          
      // check if the move has already been explored
      auto fit = explored_moves.find(move);
      if (fit == explored_moves.end())
      {
        // if not, explore the step
        fitness = explore_step(source, target, move, FUTURE_SIGHT);
        explored_moves[move] = fitness;
      }
      else
        fitness = fit->second;

      // find the best step fitness & the associated move!
      if (fitness >= step_best.first || step_best.first == 0)
        step_best = {fitness, move};
    }

    // apply the best move (permanently)
    improvement += explore_step(source, target, step_best.second, false, true);  

    // update the best solution found in the linking if possible
    if (improvement >= relinking_best)
    {
      best_relinking_sch = source;       // schedule of the best solution
      best_relinking_nodes = nodes;      // nodes of the best solution
      relinking_best = improvement;      // update the best relinking found
      one_improv = true;                 // one improvement is found!
    }
    // move to the next step! Recursion!
    relinking_phase(source, target, relinking_best, improvement, DEPTH); 
  }
  // if an improvement has been found during the relinking, update 
  if(one_improv)
  {
    one_improv = false;                       // improve only once
    std::swap(nodes, best_relinking_nodes);   // select the best nodes
    postprocessing(best_relinking_sch);       // apply post-processing
    update_best_schedule(best_relinking_sch); // update the elite pool
  }
}

double
Path_relinking::explore_step (job_schedule_t &source, 
                              const job_schedule_t &target, const move_t& move, 
                              bool FORESIGHT, bool permanent)
{
  Job j = move.first;                   // job to be modified
  Schedule ins = target.at(j);          // schedule to insert
  Schedule rem = source.at(j);          // schedule to remove

  unsigned node_idx = 0;                // initialization of node index  
  unsigned GPU_source = 0;              // required number of GPUs

  unsigned new_node_idx = move.second;  // node of the new schedule
  int GPU_target = 0;                   // GPUs of the new schedule
  unsigned GPU_rem = 0;                 // total GPUs in the node

  // remove the old setup
  if (! rem.isEmpty())
  {
    GPU_source = source[j].get_setup().get_nGPUs(); // number of GPUs to free
    node_idx = source[j].get_node_idx();            // node containing the job
    GPU_rem = nodes[node_idx].get_usedGPUs();       // GPUs used before removal
    
    // GPUs are freed from the node
    nodes[node_idx].set_remainingGPUs(-GPU_source);

    // if there are no more jobs in the node, we close it
    if (nodes[node_idx].get_usedGPUs() == 0)
      nodes[node_idx].close_node();
  }

  // insert the new setup
  if (! ins.isEmpty())
  {
    GPU_target = ins.get_setup().get_nGPUs();          // GPUs of the new setup

    // open the node if necessary
    if (! nodes[new_node_idx].open())
      nodes[new_node_idx].open_node(ins.get_setup());

    nodes[new_node_idx].set_remainingGPUs(GPU_target); // GPUs are activated
    ins.change_node(new_node_idx);                     // set the node index
  }

  // modify the Schedule
  source[j] = ins; 

  // check what is the difference in old and new proxy function 
  double cost = update_best_cost(j, ins, rem, GPU_rem); 

  if (FORESIGHT)
  {
    // calculate the new moves
    pool_t moves_FS = get_moves(source, target); 
    // remove moves that were already available before
    for (auto it = moves.begin(); it != moves.end(); ++it)
      moves_FS.remove(*it);

    // the base has to stay the same each iteration
    double baseline = cost; 
    for (const move_t& move : moves_FS)
    {
      // explore one step ahead
      double price = baseline + explore_step(source, target, move, false);
      
      // update the cost of the move with the best one
      if (price > cost)
        cost = price;
    }
  }

  // revert the situation to the previous one if not permanent
  if (! permanent)
  {
    // guiding setup is removes
    if (! ins.isEmpty())
    {
      // GPUs are freed from the node
      nodes[new_node_idx].set_remainingGPUs(-GPU_target);

      //Check the only job condition and close the corresponding node
      if (nodes[new_node_idx].get_usedGPUs() == 0)
        nodes[new_node_idx].close_node();
    }
    
    // starting setup is inserted
    if (! rem.isEmpty())
    {
      // if necessary, open the node with the correct setup
      if (! nodes[node_idx].open())
        nodes[node_idx].open_node(rem.get_setup());

      // set the number of GPUs
      nodes[node_idx].set_remainingGPUs(GPU_source);
    }
    
    // schedule is reverted back to normal
    source[j] = rem; 
  }

  // cost is returned
  return cost; 
}

Path_relinking::pool_t
Path_relinking::get_moves(job_schedule_t& source, const job_schedule_t& target)
{
  pool_t c_moves = {};  // pool of moves
  int node_idx = {};    // node idx

  for (Job& j : submitted_jobs)
  {
    if (! target.at(j).isEmpty())
    {
      // schedules must be different
      if (source.at(j) != target.at(j))
      {
        // search a node that can accomodate the setup
        node_idx = compatible(source, j, target.at(j)); 
        
        if (node_idx != -1)
        {
          // add move to the pool
          c_moves.push_back({j, node_idx});
        }
      }
    }
    // there is always space to remove a schedule
    else if (! source.at(j).isEmpty() && target.at(j).isEmpty())
    {
      c_moves.push_back({j, -1}); //-1 is just a placeholder
    }
  }

  if (verbose > 2)
  {
    // print the moves in the output file (for debugging)
    print_moves(source, target, moves);
  }
  
  return c_moves;
}

int
Path_relinking::compatible(job_schedule_t& source, const Job &j, 
                           const Schedule& candidate_schedule)
{
  // GPUs needed by the new schedule
  unsigned Required_GPU = candidate_schedule.get_setup().get_nGPUs();
  // index of the new node
  unsigned node_idx = 0;

  // first check the node the old setup was at:
  if (! source[j].isEmpty())
  {  
    node_idx = source[j].get_node_idx();
    // if the job was the only one in the node, we can accomodate the new 
    // setup for sure
    if (nodes[node_idx].get_usedGPUs() == source[j].get_setup().get_nGPUs())
      return node_idx;

    // if the job is in a shared node, we search if there is space for the new 
    // setup
    // Note: now the VM type is important here as it cannot be changed
    else
    {
      // check VM type and GPU type
      if (source[j].get_setup() == candidate_schedule.get_setup())
      {
        // calculate if enough GPUs are available for the new setup
        int GPU_avail = nodes[node_idx].get_remainingGPUs();                           
        int GPU_diff  = source[j].get_setup().get_nGPUs() + GPU_avail - Required_GPU;  
        if (GPU_diff >= 0)
          return node_idx;
      }
    }
  }

  //If the old node is not available, search in the remaining ones
  node_idx = 0;
  for (Node& node : nodes)
  {
    if (node.open())
    {
      // checks: same VM type, same GPU type and enough free GPUs
      bool same_VM    = node.get_VMtype() == candidate_schedule.get_setup().get_VMtype();   
      bool same_GPU   = node.get_GPUtype() == candidate_schedule.get_setup().get_GPUtype(); 
      bool enough_GPU = node.get_remainingGPUs() >= Required_GPU;           

      // if all conditions are satisfied return the index of the node that can 
      // host the setup
      if (same_VM && same_GPU && enough_GPU)
        return node_idx;
    }
    else
    {
      // return the idx of the closed node
      return node_idx;
    }
    // check next node
    node_idx++; 
  }
      
  return -1; // special value if no nodes have been found
}

double
Path_relinking::update_best_cost(const Job& j, const Schedule& ins, 
                                 const Schedule& rem, unsigned used_GPU_rem)
{
  //This function is extrimely ugly, it just perform the difference
  //between two proxy functions, these are just operations, nothing
  //much interesting happens here

  double price_gain =0;

  if (! rem.isEmpty())
  {
    // compute the old setup price
    double finish_time = rem.get_selectedTime();
    unsigned GPUs_rem = rem.get_setup().get_nGPUs();
    double vm_cost = finish_time * rem.get_setup().get_cost() / 3600 * GPUs_rem / used_GPU_rem;
    double tardi_cost = j.get_tardinessWeight() * std::max(current_time + finish_time - j.get_deadline(), 0.0);

    price_gain = - j.get_maxExecTime() / (vm_cost + tardi_cost);
  }

  if (! ins.isEmpty())
  {
    // compute the new setup price
    unsigned node_idx = ins.get_node_idx();
    unsigned g = nodes[node_idx].get_usedGPUs();
    double finish_time = ins.get_selectedTime();
    unsigned GPUs_ins = ins.get_setup().get_nGPUs();
    double vm_cost = finish_time * ins.get_setup().get_cost() / 3600 * GPUs_ins / g;
    double tardi_cost = j.get_tardinessWeight() * std::max(current_time + finish_time - j.get_deadline(), 0.0);

    price_gain += j.get_maxExecTime() / (vm_cost + tardi_cost);
  }

  return price_gain;
}

void
Path_relinking::print_info (double cost_FG, double cost_RG, 
                            double cost_PR) const
{ 
  // compute the number of opened nodes
  unsigned n_opened_nodes = 0;
  for(const Node& node : nodes)
    //I just wanted to use a regex
    (node.open()) ? n_opened_nodes++ : 0;

  // compute the job pressure, normalized by the system size
  double job_p = 0;
  for(const Job& j: submitted_jobs)
    job_p += j.get_pressure() / nodes.size();

  // get the improvement obtained by path relinking over randomized greedy
  double improv = ((cost_RG + 1) < cost_PR) ? cost_PR - cost_RG : 0.;

  // get the improvement obtained by randomized greedy over full greedy
  double improv_RG = ((cost_FG + 1) < cost_RG) ? cost_RG - cost_FG : 0.;

  std::cout << "current_time: " << current_time
            << ", system pressure: " << job_p
            << ", n_jobs_in_queue: " << submitted_jobs.size()
            << ", n_opened_nodes: " << n_opened_nodes
            << ", RG_improvement: " << improv_RG
            << ", PR_improvement: " << improv << std::endl;
}

void 
Path_relinking::print_moves(const job_schedule_t &source, 
                            const job_schedule_t &target, 
                            const pool_t &moves) const
{
  if (! moves.empty())
  {
    std::cout << std::endl;
    for (const move_t& move : moves)
    {
      if (! source.at(move.first).isEmpty() && ! target.at(move.first).isEmpty())
      {
        std::cout << move.first.get_ID() << " from: " 
                  << source.at(move.first).get_setup().get_VMtype()
                  << ", " << source.at(move.first).get_setup().get_nGPUs() 
                  << " to: " << target.at(move.first).get_setup().get_VMtype()
                  << ", " << target.at(move.first).get_setup().get_nGPUs() 
                  << std::endl;
      }
      else if (! target.at(move.first).isEmpty())
      {
        std::cout << move.first.get_ID() << " from: EMPTY SCHEDULE" << " to: " 
                  << target.at(move.first).get_setup().get_VMtype()
                  << ", " << target.at(move.first).get_setup().get_nGPUs() 
                  << std::endl;
      }
      else
      {
        std::cout << move.first.get_ID() << " from: " 
                  << source.at(move.first).get_setup().get_VMtype()
                  << ", " << source.at(move.first).get_setup().get_nGPUs() 
                  << " to: " << "EMPTY SCHEDULE" << std::endl;
      }
    }
    std::cout << std::endl;
  }
}
