#ifndef NODES_MAP_HH
#define NODES_MAP_HH

#include "utilities.hpp"
#include "fileIO.hpp"

class Nodes_map {

private:
  nodes_map_t nodes;
  unsigned n_nodes = 0;

  nodes_map_t full_nodes;
  unsigned n_full_nodes = 0;

  gpus_map_t shared_gpus;
  gpus_map_t full_shared_gpus;
  shared_counter_t n_shared_gpus;

  Node non_existing_node;

  unsigned verbose = 0;

  /* find_in_list (GPU version)
  *   look for a GPU with the given ID in the list passed as parameter
  *
  *   Input:    nodes_t&                    nodes list
  *             const std::string&          node ID
  *             unsigned                    GPU_ID
  *
  *   Output:   gpus_t::iterator           iterator to the GPU
  */
  gpus_t::iterator find_in_list (gpus_t&, const std::string&, unsigned);

  /* find_in_list (non-const and const version)
  *   look for a node with the given ID in the list passed as parameter
  *
  *   Input:    nodes_t&                    nodes list
  *             const std::string&          node ID
  *
  *   Output:   nodes_t::iterator           iterator to the node
  */
  nodes_t::iterator find_in_list (nodes_t&, const std::string&);
  nodes_t::const_iterator find_in_list (const nodes_t&, 
                                        const std::string&) const;

  /* get_usedGPUs
  *
  *   Input:    const nodes_map_t&          map where to look for the node
  *             const std::string&          GPU type
  *             const std::string&          node ID
  *
  *   Output:   std::pair<bool, unsigned>   <found, number of used GPUs>
  */
  std::pair<bool, unsigned> get_usedGPUs (const nodes_map_t&, 
                                          const std::string&, 
                                          const std::string&) const;

  /* compute_nodeCost
  *   compute the cost of the open nodes for the given elapsed time
  *
  *   Input:    double                      elapsed time
  *             const nodes_map_t&          map where to look for the node
  *             double&                     cost to be updated
  */
  void compute_nodeCost (double, const nodes_map_t&, double&) const;

  /* free_resources
  *   free fraction of a given GPU
  *
  *   Input:  const Node&                       node whose resources should be
  *                                             released
  *           gpus_map_t&                       map where to look for the GPU
  *           std::unordered_set<std::string>&  already updated GPUs
  *           const std::string&                message to be printed in logs
  *
  *   Output: std::pair<bool,gpus_t::iterator> <found, iterator to the GPU>
  */
  std::pair<bool,gpus_t::iterator> free_resources (const Node&, gpus_map_t&,
                                              std::unordered_set<std::string>&,
                                                    const std::string&);

  /* free_resources
  *   free GPUs of a given node
  *
  *   Input:  const Node&                       node whose resources should be 
  *                                             released
  *           nodes_map_t&                      map where to look for the node
  *           std::unordered_set<std::string>&  already updated nodes
  *           const std::string&                message to be printed in logs
  *
  *   Output: std::pair<bool,nodes_t::iterator> <found, iterator to the node>
  */
  std::pair<bool,nodes_t::iterator> free_resources (const Node&, nodes_map_t&, 
                                              std::unordered_set<std::string>&,
                                                    const std::string&);

  /* print
  *   print a specific map of nodes (either nodes or full_nodes)
  *
  *   Input:    std::ostream&               where to print the maps
  *             const nodes_map_t&          map to be printed
  *             const std::string&          "true" if the map is full_nodes
  */
  void print (std::ostream&, const nodes_map_t&, const std::string&) const;

public:
  /* constructor
  *
  *   Input(1): void                        default
  *
  *   Input(2): const std::string&          name of the file where nodes 
  *                                         information are stored
  */
  Nodes_map (void) = default;
  Nodes_map (const std::string&);

  // getters
  unsigned get_n_nodes (void) const {return n_nodes;}
  unsigned get_n_full_nodes (void) const {return n_full_nodes;}
  gpus_map_t& get_shared_gpus (void) {return shared_gpus;}
  gpus_map_t& get_full_shared_gpus (void) {return full_shared_gpus;}

  /* init
  *
  *   Input:    const std::string&          name of the file where nodes 
  *                                         information are stored
  */
  void init (const std::string&);

  /* isEmpty
  */
  bool isEmpty (void) const {return (n_nodes == 0);}

  /* find_node (non-const and const version)
  *   look for a node, given the ID and (possibly) the GPU type
  */
  Node& find_node (const std::string&, const std::string& = "");
  const Node& find_node (const std::string&, const std::string& = "") const;

  /* isFull
  *   check if the given node is full
  *
  *   Input:    const std::string&          GPU type
  *             const std::string&          node ID
  *
  *   Output:   bool                        true if the given node is full
  */
  bool isFull (const std::string&, const std::string&) const;

  /* isFull
  *   check if the whole set of nodes is full
  */
  bool isFull (void) const {return (n_full_nodes == n_nodes);}

  /* get_remainingGPUs
  *
  *   Input:    const std::string&          GPU type
  *             const std::string&          node ID
  *
  *   Output:   unsigned                    number of free GPUs
  */
  unsigned get_remainingGPUs (const std::string&, const std::string&) const;

  /* get_usedGPUs
  *
  *   Input:    const std::string&          GPU type
  *             const std::string&          node ID
  *
  *   Output:   unsigned                    number of used GPUs
  */
  unsigned get_usedGPUs (const std::string&, const std::string&) const;

  /* add
  *   add a node to the set of available nodes
  */
  void add (const Node&);

  /* add_full_node
  *   add a node to the set of full nodes
  */
  void add_full_node (const Node&);
  
  /* add_in_order
  *   add a node to the set of available nodes (in the given position)
  */
  void add_in_order (const Node&, nodes_t::iterator);

  /* count_open_nodes
  *   count the number of open nodes
  */
  unsigned count_open_nodes (void) const;

  /* compute_nodeCost
  *   compute the cost of all open nodes for the given elapsed time
  */
  double compute_nodeCost (double elapsed_time) const;

  /* assign_to_GPU
  *   update the fraction of the GPU with GPU ID given in the node with node ID
  *   given subtracting the required amount.
  *
  *   Input:    const std::string&          GPU type
  *             double                      fraction of GPU to be subtracted
  *             const std::string&          node ID
  *             unsigned                    GPU ID
  *
  *   Output:   std::string                 ID of updated node + updated GPU
  */
  std::string assign_to_GPU (const std::string&, double, const std::string&,
                             unsigned);

  /* assign_to_GPU
  *   update the fraction of a just opened GPU in a node with a given GPU type
  *   subtracting the required amount. A node ID can be specified if needed
  *
  *   Input:    const std::string&          GPU type
  *             double                      fraction of GPU to be subtracted
  *             const std::string&          node ID
  *
  *   Output:   std::string                 ID of updated node + updated GPU
  */
  std::string assign_to_GPU (const std::string&, double, const std::string&);

  /* assign_to_node
  *   update the number of available GPUs in a node with the given GPU type, 
  *   subtracting the required amount. A node ID can be specified if needed
  *
  *   Input:    const std::string&          GPU type
  *             unsigned                    number of GPUs to be subtracted
  *             double                      fraction of GPU to be subtracted
  *             bool                        true if the node must be set to 
  *                                         full independently of the remaining 
  *                                         GPUs (default: false)
  *             const std::string&          node ID (default: "")
  *
  *   Output:   std::string                 ID of the updated node
  */
  std::string assign_to_node (const std::string&, unsigned, double = 1.,
                              bool = false, const std::string& = "");

  /* free_resources
  *   free GPUs according to the given list of ended jobs (and relative nodes)
  *
  *   Input:    const jn_pairs&             pairs of ended jobs and 
  *                                         corresponding nodes
  *             unsigned                    verbosity level (default: 0)        
  */
  void free_resources (const jn_pairs_t&, unsigned = 0);

  /* close_nodes
  *   close all nodes, releasing the relative resources and setting them as
  *   empty nodes
  */
  void close_nodes (void);

  /* print_names (static)
  *
  *   Input:    std::ostream&               where to print names of map fields
  *             char                        last character to be printed 
  *                                         (default: \n)
  */
  static void print_names (std::ostream&, char = '\n');

  /* print
  *
  *   Input:    std::ostream&               where to print the maps
  */
  void print (std::ostream&) const;

};

#endif /* NODES_MAP_HH */