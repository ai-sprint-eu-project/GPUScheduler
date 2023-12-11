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

#ifndef PATH_RELINKING_HH
#define PATH_RELINKING_HH

#include "random_greedy.hpp"

// specialization of the hash function for type move_t (see type definition 
// below)
namespace std{
  template <>
  struct hash<std::pair<Job, int>> {  
    size_t operator()(const std::pair<Job, int>& p) const
    { 
      auto hash1 = std::hash<std::string>() (p.first.get_ID());
      return hash1 ^ p.second; 
    }
  }; 
}

// class definition
class Path_relinking: public Random_greedy {

protected:
  // type definitions
  typedef std::pair<Job, int> move_t;
  typedef std::list<move_t> pool_t;
  typedef std::unordered_map<move_t, double> moves_t;

  // maximum number of best solutions to be saved
  unsigned K = 10;

  //maximum number of steps in the relinking
  unsigned MAX_DEPTH = nodes.size();            /*XXX check: quando viene 
                                                        inizializzata?? XXX*/

  // explore one step further each time
  bool FUTURE_SIGHT = true;

  // parameters for randomization
  double alpha = 0.3;                    // parameter for setup selection

  // pool of moves
  pool_t moves;

  // map of the already explored moves
  moves_t explored_moves;

  // best solution found by path relinking
  //Best_solution best_relinking_solution;  /*XXX update XXX*/

  job_schedule_t best_relinking_sch;        //Best relinking schedule
  std::vector<Node> best_relinking_nodes;   //Best relinking active 

  // simple check to update just once the elite pool    /*XXX ? XXX*/
  bool one_improv = false;

  /* perform_scheduling
  *   perform the scheduling process that assigns the best configuration to
  *   each submitted jobs (as long as resources are available)
  *
  *   Output: job_schedule_t        proposed schedule
  */
  virtual job_schedule_t perform_scheduling (void) override;

  /* objective_function
  *   evaluate objective function of the current schedule
  *
  *   Input:  job_schedule_t&             new proposed schedule
  *           double                      elapsed time between previous and 
  *                                       current iteration
  *           const std::vector<Node>&    vector of open nodes
  *
  *   Output: double              total cost of the proposed schedule
  */
  virtual double objective_function (job_schedule_t&, double,
                                     const std::vector<Node>&) override;

  /* restore_map_size
  *   erase the element in the map with the worst cost (the first element), so 
  *   that the size of the map returns less than or equal to the maximum size K
  *
  *  Input:     K_best_t&           map to be modified
  */
  virtual void restore_map_size (K_best_t&) override;

  /* to_be_inserted
  *   return true if the element characterized by the given upper bound should
  *   be inserted in the map
  */
  virtual bool to_be_inserted (const K_best_t&, K_best_t::iterator) const override;

  /* best_fight
  *   iterate trough the elite solutions and perform forward relinking between 
  *   the best elite and another elite solution
  */
  void best_fight (void);

  /* path relinking between two solutions:
  *   recursive function that explores the space until no moves between the 
  *   current schedule and the guiding one are available or until the maximum 
  *   number of moves is reached. At most one improvement is made in the elite 
  *   pool (avoid homogeneization of the elite)
  *
  *   Input:    job_schedule_t&         source schedule
  *             const job_schedule_t&   guiding schedule
  *             double                  best improvement reached
  *             double                  current improvement             
  *             unsigned                depth of exploration
  * 
  *   Output:   Updated elite pool of solutions
  * 
  */
  void relinking_phase (job_schedule_t&, const job_schedule_t&, double, 
                        double, unsigned);

  /* Explore the effect of applying a step to a schedule_t
  *   if the bool is set to true, the move is applied, else the solution is 
  *   reverted back to normal. The functions also has a recursive part to 
  *   search forbidden moves
  * 
  *   Input:    job_schedule_t&               source solution
  *             const job_schedule_t&         target solution       
  *             const move_t&                 move to apply
  *             int                           foresight (for recursion)
  *             bool                          permament move (false by default)
  *
  *   Output:   double                        improvement of the modified 
  *                                           schedule
  *
  *   The source solution is modified if permanent == true
  * 
  */
  double explore_step (job_schedule_t &, const job_schedule_t&,
                       const move_t&, bool, bool permanent = false);

  /* get_moves
  *   search for possibile moves, given two schedules
  *   a move is valid if a node can accomodate the setup of the guiding 
  *   solution, will generate a map containing all the possible moves
  *
  *   Input:  job_schedule_t&                 source solution
  *           job_schedule_t&                 guiding solution
  * 
  *   Output: unordered_map<Job, Int>
  * 
  */
  pool_t get_moves (job_schedule_t&, const job_schedule_t&);

  /* compatible
  *   return the index of a node that is able to accomodate a proposed 
  *   schedule (-1 if no node is available)
  *   
  *   Input:  job_schedule_t&                 source schedule
  *           const Job&                      job to modify
  *           const Schedule&                 schedule to accomodate
  * 
  *   Output: int                             node index
  * 
  */
  int compatible(job_schedule_t &, const Job &, const Schedule &);
  
  /* Created to avoid computing each time the proxy function to test
  *   a move. It will apply the difference between the
  *   proxy before the move, and the proxy after the move.
  *   This results in computing the cost of the modified job,
  *   instead of the whole schedule. This function just a single
  *   iteration of greedy::obective_function()
  *
  * Input: const Job&       Job that will change setup
  *        const Schedule&  Outgoing schedule
  *        const Schedule&  Incoming schedule
  *        unsigned         GPUs of the node in the old Schedule      
  * 
  * Output: double          difference between the two proxy function
  * 
  */
  double update_best_cost(const Job&, const Schedule&, const Schedule&, unsigned);

  /* Print out some usefull infomation about 
  *  the path relinking method (in the output file)
  * 
  *  Input: double Greedy cost
  *         double RandomGreedy cost
  *         double PathRelinking cost 
  *   
  *  Output: current time, queue pressure,
  *          jobs in queue, nodes opened, 
  *          RG improvement, PR improvement
  *           
  */
  void print_info(double, double, double) const;

  /*  print the moves in the output file (Job, VM and number of GPUs)
  *   
  *   Input : source and target schedule 
  *           pool of moves
  * 
  *   Output : list of moves in the output file
  */
  void print_moves(const job_schedule_t&, const job_schedule_t &, 
                   const pool_t &) const;

public:
  /*  constructor
  *
  *   Input:  const std::string&    full path of data directory
  *           const std::string&    name of file with the list of jobs
  *           const std::string&    name of file with execution times of jobs
  *           const std::string&    name of file with the list of nodes
  *
  */
  Path_relinking (const std::string&, const std::string&, const std::string&, 
                  const std::string&);

  /* destructor
  */
  virtual ~Path_relinking (void) = default;

};

#endif /* PATH_RELINKING_HH */