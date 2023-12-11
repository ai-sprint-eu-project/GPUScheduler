#!/usr/bin/env python3
# -*- coding: utf-8 -*-
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


import os
import sys
import ast
import configparser as cp
import argparse
import logging
import pandas as pd
import matplotlib.pyplot as plt


def createFolder (directory):
    try:
        if not os.path.exists(directory):
            os.makedirs(directory)
    except OSError:
        print ("Error: Creating directory. " +  directory)


def fix_commas (x):
    return float(x.replace(",","."))


def plot_results (path, distr, existing_lambdas, existing_methods, plot_path,
                  baseline):
    
    baseline_col = "TC_" + baseline

    # import data
    filename = distr + "_avg.csv"
    print("Examining " + filename)
    filename = path + "/" + filename
    if os.path.exists(filename):
        data = pd.read_csv(filename, sep=";", decimal=",")
        
        # filter required lambdas
        for lambdaa in existing_lambdas:
            data_l = data[data["Lambda"] == lambdaa].copy()
            
            # plot average total costs
            current_figure = plt.figure()
            ax = current_figure.gca()
            if baseline_col in data_l.columns:
                for method in existing_methods:
                    colname = "TC_" + method
                    if not data_l[colname].dtype == float:
                        data_l[colname] = data_l[colname].apply(fix_commas)
                    if not data_l[baseline_col].dtype == float:
                        data_l[baseline_col] = data_l[baseline_col].apply(fix_commas)
                    data_l["ratio"] = data_l[colname] / data_l[baseline_col]
                    linestyle = "solid"
                    if method.startswith("DP"):
                        linestyle = "dashed"
                    data_l.plot(x = "Nodes", y = "ratio", linestyle = linestyle,
                              ax = ax, label = method, fontsize = 14,
                              linewidth = 3, logy = True)
                handles,labels = ax.get_legend_handles_labels()
                ax.legend(handles, labels, loc='center left', 
                          bbox_to_anchor=(1, 0.5), fontsize = 14)
                plt.xlabel("Number of nodes", fontsize = 14)
                plt.ylabel("Average total cost", fontsize = 14)
                title = "Total costs - " + distr + " - lambda " + str(lambdaa)
                #plt.title(title, fontsize = 14)
                if plot_path == "":
                    plt.show()
                else:
                    title = title.replace(" ", "_")
                    title = title.replace("-","")
                    fig_path = plot_path + "/figures"
                    createFolder(fig_path)
                    current_figure.savefig(fig_path + "/" + title + ".pdf", 
                                           format='pdf', dpi=1000, 
                                           bbox_inches = "tight")
                    plt.close(current_figure)
    
            # plot average percentage gain
            current_figure = plt.figure()
            ax = current_figure.gca()
            for method1 in existing_methods:
                for method2 in existing_methods:
                    colname = method1 + "_VS_" + method2
                    if colname in data_l.columns:
                      if not data_l[colname].dtype == float:
                          data_l[colname] = data_l[colname].apply(fix_commas) * 100
                      else:
                          data_l[colname] = data_l[colname] * 100
                      linestyle = "solid"
                      if method1.startswith("DP") or method2.startswith("DP"):
                          linestyle = "dashed"
                      data_l.plot(x = "Nodes", y = colname, 
                                  linestyle = linestyle, ax = ax, 
                                  label = colname, fontsize = 14,
                                  linewidth = 3)
            handles,labels = ax.get_legend_handles_labels()
            ax.legend(handles, labels, loc='center left', 
                      bbox_to_anchor=(1, 0.5), fontsize = 14)
            plt.xlabel("Number of nodes", fontsize = 14)
            plt.ylabel("Average percentage gain", fontsize = 14)
            title = "Percentage gain - " + distr + " - lambda " + str(lambdaa)
            #plt.title(title, fontsize = 14)
            if plot_path == "":
                plt.show()
            else:
                title = title.replace(" ", "_")
                title = title.replace("-","")
                fig_path = plot_path + "/figures"
                createFolder(fig_path)
                current_figure.savefig(fig_path + "/" + title + ".pdf", 
                                       format='pdf', dpi=1000, 
                                       bbox_inches = "tight")
                plt.close(current_figure)
    else:
        print("ERROR: " + filename + " does not exist")
        

def main ():
    
    parser = argparse.ArgumentParser(description="Plot aggregated results")

    parser.add_argument("results_directory", help="The directory containing aggregated results")
    parser.add_argument("-c", "--config", help="Configuration file", 
                        default="config.ini")
    parser.add_argument("-d", "--debug", help="Enable debug messages", 
                        default=False, action="store_true")
    parser.add_argument("-p", "--plot", 
                        help="Path to the directory where plots should be saved", 
                        default="")
    
    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG, format="%(levelname)s: %(message)s")
    else:
        logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")

    if not os.path.exists(args.results_directory):
        logging.error("%s does not exist", args.results_directory)
        sys.exit(1)

    ##########################################################################

    ## configuration parameters
    config = cp.ConfigParser()
    config.read(args.config)

    # distributions
    all_distributions = ast.literal_eval(config["RandomParameters"]["distributions"])
    #vals = list(range(len(all_distributions)))
    #all_distributions = dict(zip(all_distributions, vals))
    
    # lambdas
    all_lambdas = {"exponential": [75000], 
                   "high": ["\"+30%\""],
                   "low": ["\"+30%\""], 
                   "mixed": ["\"+30%\""]}

    # methods
    all_methods = ast.literal_eval(config["Methods"]["methods"])
    baseline = ast.literal_eval(config["Methods"]["baseline_method"])
    #vals = list(range(len(all_methods)))
    #all_methods = dict(zip(all_methods, vals))


    ##########################################################################
    
    for distr in all_distributions:
        plot_results(args.results_directory, distr, all_lambdas[distr], 
                     all_methods, args.plot, baseline)


if __name__ == "__main__":
    main()
