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
import numpy as np
import pandas as pd
from math import floor, ceil, modf


def createFolder (directory):
    try:
        if not os.path.exists(directory):
            os.makedirs(directory)
    except OSError:
        print ("Error: Creating directory. " +  directory)


def generate_df_times (df_data, df_costs, selected_jobs, neglected_types = []):
    
    # df_times: execution times of the selected jobs with all the available
    # setups
    df_times = pd.DataFrame(columns = ["Jobs", "UniqueJobsID", "VMtype", 
                                       "GPUtype", "nGPUs", "max_nGPUs", 
                                       "cost", "ExecutionTime"])
    # list of jobs added to df_times dataframe
    added_jobs = []
    
    for job in selected_jobs:
        if job[1] not in added_jobs:
            # select all jobs with the current ID
            jDF = df_data[df_data["Jobs"] == job[1]]
            # group rows by GPUtype
            jDF_by_GPUtype = jDF.groupby(["GpuType"])
            for elem in jDF_by_GPUtype:
                GPUtype = elem[0]
                if GPUtype not in neglected_types:
                    # get dataframe
                    GPUtype_DF = elem[1]
                    # select all VMs with the current GPUtype
                    VMs = df_costs[df_costs["GpuType"] == GPUtype]
                    for idx in VMs.index:
                        max_nGPUs = VMs["GpuNumber"].loc[idx]
                        VMtype = VMs["VMType"].loc[idx]
                        # select all rows with number of GPUs <= max_nGPUs  
                        rows = GPUtype_DF[GPUtype_DF["GpuNumber"]<= max_nGPUs]
                        temp = pd.DataFrame()
                        temp["Jobs"] = rows["Jobs"]
                        temp["UniqueJobsID"] = job[0]
                        temp["VMtype"] = VMtype
                        temp["GPUtype"] = rows["GpuType"]
                        temp["nGPUs"] = rows["GpuNumber"]
                        temp["max_nGPUs"] = max_nGPUs
                        temp["cost"] = VMs["cost"].loc[idx]
                        temp["ExecutionTime"] = rows["ExecutionTime"]
                        temp["min"] = 0.0
                        temp["max"] = 0.0
                        # append rows to time dataframe
                        df_times = df_times.append(temp, ignore_index=True)                
            # append current job ID to the list of examined jobs
            added_jobs.append(job[1])
        else:
            that_job = df_times[df_times["Jobs"] == job[1]].copy()
            uid = that_job.groupby(["UniqueJobsID"]).first().index[0]
            that_job = that_job[that_job["UniqueJobsID"] == uid]
            that_job["UniqueJobsID"] = job[0]
            df_times = df_times.append(that_job, ignore_index=True)

    return (df_times)


def generate_rates (df_times, n_jobs, n_nodes, distribution):
    
    # approximation function
    approx = lambda x : ceil(x) if modf(x)[0] >= 0.5 else floor(x)
    
    # size of job clusters and number of clusters
    K = 10
    n_clusters = approx(n_jobs / K)
    
    # expected execution times with one GPU
    times_one_GPU = df_times[df_times["nGPUs"] == 1].copy()
    max_nGPUs = times_one_GPU["max_nGPUs"].max()
    # group by job id
    times_by_job = times_one_GPU.groupby(["UniqueJobsID"])
    # min execution time per job
    execTime_min = times_by_job.min()["ExecutionTime"]

    lambdas = n_nodes * max_nGPUs / execTime_min
    
    rates = pd.DataFrame()
    clusters = {}
    for k in range(n_clusters):
        if distribution == "high" or (distribution == "mixed" and k%2==0):
            local_distribution = "high"
        else:
            local_distribution = "low"
        k_idx = "k"+str(k)
        rates[k_idx] = [0.0, 0]
        clusters[k_idx] = ""
        for j in range(k, len(lambdas), n_clusters):
            jid = lambdas.index[j]
            clusters[k_idx] = clusters[k_idx] + jid + "; "
            if local_distribution == "low":
                rates[k_idx][0] += ((lambdas[jid] / 12) * 1.3)
            elif local_distribution == "high":
                rates[k_idx][0] += ((lambdas[jid] / 3) * 1.3)
            rates[k_idx][1] += 1
    rates = rates.loc[0] / rates.loc[1]
    
    return (rates, clusters)
 

def generate_deadline (SubmissionTime, min_time, max_time):
    return (SubmissionTime + np.random.uniform(min_time, 3 * min_time))


def generate_tardinessweight ():
    return (2 * np.random.uniform(0.0015,0.0075))


def generate_nodes (nN):
    ID=1
    LofNodes=[]
    for i in range(nN):
        LofNodes.append("n"+str(ID))
        ID += 1
    return LofNodes


def generate_custom_submissions (df_times, n_jobs, n_nodes, distribution):
    
    # generate inter-arrival rates and clusters of jobs
    (rates, clusters) = generate_rates(df_times,n_jobs,n_nodes,distribution)
    # inter-arrival times
    times = 1 / rates
    
    SubmissionTimes = {}
    arrivals = []
    first_job = True
    last_time = 0.0
    for cluster in clusters:
        jobs = clusters[cluster].split("; ")[:-1]
        if first_job:
            SubmissionTimes[jobs[0]] = 0.0
            jobs = jobs[1:]
            first_job = False
        s = np.random.exponential(times[cluster], len(jobs))
        for j in range(len(jobs)):
            new_t = s[j]
            arrivals.append(new_t)
            last_time += new_t
            SubmissionTimes[jobs[j]] =  last_time
    
    return (SubmissionTimes, arrivals)


def generate_submissions (selected_jobs, lambdaa, nN, nJ, distribution):

    AttendTimeTemp = {}
    arrivals = []
    
    # generate submission times for the required n_jobs
    if distribution == 'exponential':
      s = np.random.exponential(lambdaa/nN, nJ)
    else:
      s = np.random.poisson(lambdaa/nN, nJ)
    AttendTimeTemp[selected_jobs[0][0]] = 0.0
    last_time = 0.0
    for i in range(len(s)):
        new_t = s[i]
        arrivals.append(new_t)
        last_time += new_t
        AttendTimeTemp[selected_jobs[i+1][0]] = last_time
    
    return (AttendTimeTemp, arrivals)


def generate_data (nN, nJ, myseed, distribution, lambdaa, scenario):
    
    # custom distributions
    custom_distributions = ["high", "low", "mixed"]
    
    # set seed to control random number generation and job selection
    np.random.seed(myseed)

    # generate folder to store new data
    folder_name = "../../build/data/tests_new/1-" + str(nN) + "-" + \
                  str(nJ) + "-" + str(myseed) + "-" + \
                  str(distribution) + "-" + str(lambdaa)
    createFolder(folder_name)
    #
    folder_name += "/"

    # load data from data.csv
    #
    # df_data = ["Application", "Images", "Epochs", "Batchsize", "Jobs", 
    #            "GpuType", "GpuNumber", "ExecutionTime", "min", "max"]
    data_file = "../../build/data/data_2d10d_filtered.csv"
    df_data = pd.read_csv(data_file)

    # load costs information from GPU-cost.csv
    #
    # df_costs = ["VMType", "GpuType", "GpuNumber", "cost"]
    if scenario == "datacenter":
        cost_filename = "GPU-cost_server.csv"
    else:
        cost_filename = "GPU-cost.csv"
    costs_file = "../../build/data/" + cost_filename
    df_costs = pd.read_csv(costs_file)

    # generate list of selected jobs
    all_jobs = df_data["Jobs"].unique()
    selected_jobs = []
    for idx in range(1,nJ+2):
        # generate an unique id for the new job
        unique_id = "JJ" + str(idx)
        # randomy pick a job from the list of all available jobs
        random_idx = np.random.randint(0, len(all_jobs))
        # add the corresponding job to the list (unique_id, job_id)
        selected_jobs.append((unique_id, all_jobs[random_idx]))
          
    # generate expected execution times of the selected jobs with all the
    # available setups
    #
    # df_times = ["Jobs", "BatchSize", "VMtype", "GPUtype", "nGPUs", 
    #             "max_nGPUs", "ExecutionTime"]

    df_times = generate_df_times(df_data, df_costs, selected_jobs, 
                                  neglected_types = ["Quadro P600", 
                                                     "GTX 1080Ti"])
    
    # generate submission times of all selected jobs
    if distribution in custom_distributions:
        (SubmissionTimes, arrivals) = generate_custom_submissions(df_times, 
                                                      len(selected_jobs), nN,
                                                      distribution)
    else:
        SubmissionTimes = generate_submissions(selected_jobs, lambdaa, 
                                               nN, nJ, distribution)[0]
    
    # df_jobs: selected jobs with their characteristics
    df_jobs = pd.DataFrame(columns = ["Application", "Images", "Epochs", 
                                      "Batchsize", "Jobs", "UniqueJobsID", 
                                      "SubmissionTime", "Deadline", 
                                      "Tardinessweight", "MinExecTime", 
                                      "MaxExecTime"])
    
    for job in selected_jobs:
        # select all jobs with the current ID
        jDF = df_data[df_data["Jobs"] == job[1]].iloc[0]
        timeDF = df_times[df_times["UniqueJobsID"] == job[0]]
        # get minimum and maximum execution time for the current job
        min_time = timeDF["ExecutionTime"].min()
        max_time = timeDF["ExecutionTime"].max()
        # submission time
        sub_time = SubmissionTimes[job[0]]
        
        # collect data of current job in a dataframe
        temp = pd.DataFrame(jDF.iloc[0:5]).transpose()
        temp["UniqueJobsID"] = job[0]
        temp["SubmissionTime"] = sub_time
        # generate deadline and tardiness weight
        temp["Deadline"] = generate_deadline(sub_time, min_time, max_time)
        temp["Tardinessweight"] = generate_tardinessweight()
        temp["MinExecTime"] = min_time
        temp["MaxExecTime"] = max_time

        # append dataframe to df_jobs
        df_jobs = df_jobs.append(temp, ignore_index=True)

    # write data to files
    df_jobs = df_jobs.sort_values(by="SubmissionTime")
    df_jobs.to_csv(folder_name + "Lof_Selectjobs.csv", index=False)
    df_times.to_csv(folder_name + "SelectJobs_times.csv", 
                    columns=["UniqueJobsID", "VMtype", "GPUtype", "nGPUs", 
                             "max_nGPUs", "cost", "ExecutionTime"],
                    index=False)
    
    # generate nN nodes and save them to file
    with open(folder_name + "tNodes.csv","w") as resultFile:
        resultFile.write('Nodes' + '\n')
        for r in generate_nodes(nN):
            resultFile.write(r + '\n')
    resultFile.close()

    
    

if __name__ == "__main__":
    nN           = int(sys.argv[1])
    nJ           = int(sys.argv[2])
    myseed       = int(sys.argv[3])
    distribution = sys.argv[4]
    scenario     = sys.argv[5]
    if distribution == "poisson" or distribution == "exponential":
        lambdaa = int(sys.argv[6])
    else:
        lambdaa = "000"
    
    possible_distributions = ["poisson", "exponential", "low", "high" ,"mixed"]
    possible_scenarios = ["regular", "datacenter"]
    
    if (nN == 0 or nJ == 0 or distribution not in possible_distributions or \
        scenario not in possible_scenarios):
        print("Sorry!! the inputs are not valuable. Data generation failed.")
        sys.exit(1)
    else:
        generate_data(nN, nJ, myseed, distribution, lambdaa, scenario)
        
