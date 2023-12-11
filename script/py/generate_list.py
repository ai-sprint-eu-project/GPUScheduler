"""
Copyright 2020-2021 Federica Filippini

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

import sys
import os
import ast
import configparser as cp
import argparse

def createFolder(directory):
    try:
        if not os.path.exists(directory):
            os.makedirs(directory)
    except OSError:
        print ("Error: Creating directory. " +  directory)


def createInput(basedir, inputs, delta, method, existing_random, n_cpp_seed, 
                myseed, from_cpp_seed, n_random_iter, verbose, LList_file):
    directory = basedir + "/" + inputs.replace(" ", "-") + "/results_" + \
                str(delta)
    createFolder(directory)
    
    if method in existing_random:
        ncppseed = n_cpp_seed                    
        for j in range(ncppseed):
            cppseed = myseed + 10*(j+from_cpp_seed+2)
            benchmark_name = method + "_" + inputs.replace(" ", "_") + "_" +\
                     		 str(delta) + "_" + str(cppseed)
            Linputs = "method=" + method +\
                      " folder=" + directory +\
                      " seed=" + str(cppseed) +\
                      " iter=" + str(n_random_iter) +\
                      " verbose=" + str(verbose) +\
                      " benchmark_name=" + benchmark_name
            LList_file.write(Linputs+"\n")
    else:
        benchmark_name = method + "_" + inputs.replace(" ", "_") + "_" +\
                     	 str(delta)
        Linputs = "method=" + method + \
                  " folder=" + directory +\
                  " verbose=" + str(verbose) +\
                  " benchmark_name=" + benchmark_name
        LList_file.write(Linputs+"\n")

def main ():

    parser = argparse.ArgumentParser(description="Generate input list")

    parser.add_argument('-c', "--config", help="Configuration file", 
                        default="config.ini")
    parser.add_argument('-o', "--output",
                        help="String to be added to names of the list files",
                        default="")

    args = parser.parse_args()
    
    ##########################################################################
    
    # absolute path of current script
    current_script = os.path.abspath(sys.argv[0])
    basedir = current_script.split("/")[:-3]
    basedir = "/".join(basedir) + "/build/data"

    ##########################################################################

    ## configuration parameters
    config = cp.ConfigParser()
    config.read(args.config)

    # number of nodes
    nodes_min         = int(config['Nodes']['nodes_min'])
    nodes_max         = int(config['Nodes']['nodes_max'])
    nodes_step        = int(config['Nodes']['nodes_step'])

    # number of instances
    instances       = int(config['Instances']['instances'])
    from_seed       = int(config['Instances']['from_seed'])

    # parameters for randomization
    distributions   = ast.literal_eval(config['RandomParameters']['distributions'])
    lambdaas        = ast.literal_eval(config['RandomParameters']['lambdaas'])
    n_random_iter   = int(config['RandomParameters']['n_random_iter'])
    n_cpp_seed      = int(config['RandomParameters']['n_cpp_seed'])
    from_cpp_seed   = int(config['RandomParameters']['from_cpp_seed'])

    # scenario for data generation
    scenario        = config['Scenario']['scenario']

    # methods
    methods         = ast.literal_eval(config['Methods']['methods'])
    existing_heur   = ast.literal_eval(config['Methods']['existing_heuristics'])
    existing_greedy = ast.literal_eval(config['Methods']['existing_greedy'])
    existing_random = ast.literal_eval(config['Methods']['existing_random'])

    # jobs
    nInitialJ       = int(config['Jobs']['nInitialJ'])
    job_times_nodes = float(config['Jobs']['job_times_nodes'])

    # other parameters
    delta           = float(config['OtherParams']['delta'])
    verbose         = int(config['OtherParams']['verbose'])

    ##########################################################################

    ## list generation
    folder = basedir + "/lists/"
    createFolder(folder)
    List_file = open(folder + "list_of_data" + args.output + ".txt", "w")
    LList_file = open(folder + "list_of_inputs" + args.output + ".txt", "w")
    nodes = nodes_min
    
    basedir = basedir + "/tests_new"
    
    while nodes <= nodes_max:
        
        Jobs = int(job_times_nodes * nodes)
        print(nodes, " - ", Jobs)
    
        for distribution in distributions:
            not_custom = (distribution == "poisson" or distribution == "exponential")
            for i in range(instances):
                myseed = 1000 * ((i+from_seed+1)**2)
                inputs = str(nodes) + " " + str(Jobs) + " " + str(myseed)
                if not_custom:
                    for lambdaa in lambdaas:
                        inputs2 = inputs + " " + str(distribution) + " " + \
                                  str(scenario) + " " + str(lambdaa)
                        List_file.write(inputs2 +"\n")
                else:
                    inputs2 = inputs + " " + str(distribution) + " " + \
                              str(scenario)
                    List_file.write(inputs2 +"\n")
                    
                inputs = str(nInitialJ) + " " + inputs

                for method in methods:
                    inputs2 = inputs + " " + str(distribution)
                    if not_custom:
                        for lambdaa in lambdaas:
                            inputs3 = inputs2 + " " + str(lambdaa)
                            createInput(basedir, inputs3, delta, method, 
                                        existing_random, n_cpp_seed, 
                                        myseed, from_cpp_seed, n_random_iter, 
                                        verbose, LList_file)
                    else:
                        inputs3 = inputs2 + " 000"
                        createInput(basedir, inputs3, delta, method, 
                                    existing_random, n_cpp_seed, 
                                    myseed, from_cpp_seed, n_random_iter, 
                                    verbose, LList_file)

        nodes = nodes + nodes_step

    List_file.close()
    

if __name__ == "__main__":
    main()
