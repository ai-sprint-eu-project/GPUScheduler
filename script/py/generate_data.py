#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Feb  9 12:12:33 2021

@author: federicafilippini
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


def generate_real_epoch (df_distributions):

    n_distrib = df_distributions.shape[1]
    epochs = range(1, df_distributions.shape[0] + 1)
    distrib = df_distributions.columns[np.random.randint(n_distrib)]
    real_epoch = np.random.choice(epochs, p = df_distributions[distrib])
    avg_epoch = np.average(epochs, weights = df_distributions[distrib])

    return distrib, real_epoch, avg_epoch


def generate_df_times (df_data, selected_jobs, average = "false",
                       df_fractions = pd.DataFrame(),
                       df_distributions = pd.DataFrame(), neglected_types = []):

    # df_times: execution times of the selected jobs with all the available
    # setups
    df_times = pd.DataFrame(columns = ["Jobs", "ID", "Epochs", "GPUtype", "nGPUs",
                                       "GPUf", "ExecutionTime"])

    if not df_distributions.empty:
        epoch_ref = df_distributions.shape[0]

    # list of jobs added to df_times dataframe
    added_jobs = []
    
    # random generator
    rng = np.random.default_rng(myseed)

    for job in selected_jobs:
        if not df_distributions.empty:
            distrib, real_epoch, avg_epoch = generate_real_epoch(df_distributions)
        if job[1] not in added_jobs:
            # select all jobs with the current ID
            jDF = df_data[df_data["Jobs"] == job[1]]
            if not df_fractions.empty:
                memory = jDF["Memory"].iloc[0]
                df_fractions["inflate"] = (df_fractions["inflate_max"] - 1.01) \
                                          * rng.random() + 1.01
            # group rows by GPUtype
            jDF_by_GPUtype = jDF.groupby(["GpuType"])
            for elem in jDF_by_GPUtype:
                GPUtype = elem[0]
                if GPUtype not in neglected_types:
                    # get dataframe
                    GPUtype_DF = elem[1]
                    #rows = GPUtype_DF[GPUtype_DF["GpuNumber"]<= max_nGPUs]
                    if not df_fractions.empty:
                        fractions_DF = df_fractions[(df_fractions["Memory"] >= memory) &
                                                    (df_fractions["GPUtype"] == GPUtype)]
                        if not fractions_DF.empty:
                            temp_sh = pd.DataFrame(columns = df_times.columns)
                            temp_sh["GPUf"] = fractions_DF["GPUf"]
                            temp_sh["Jobs"] = job[1]
                            temp_sh["ID"] = job[0]
                            temp_sh["Epochs"] = GPUtype_DF["Epochs"]
                            temp_sh["GPUtype"] = GPUtype
                            temp_sh["nGPUs"] = 1
                            exec_time = GPUtype_DF[GPUtype_DF["GpuNumber"] == 1]["ExecutionTime"].values
                            temp_sh["ExecutionTime"] = exec_time * fractions_DF["inflate"]
                            temp_sh["min"] = 0.0
                            temp_sh["max"] = 0.0
                            if not df_distributions.empty:
                                temp_sh["Distribution"] = distrib
                                temp_sh["RealExecutionTime"] = temp_sh["ExecutionTime"] * \
                                                               real_epoch / epoch_ref
                            if average == "true":
                                temp_sh["RatioAvg"] = (epoch_ref - avg_epoch) / real_epoch
                                temp_sh["AvgExecutionTime"] = temp_sh["ExecutionTime"] * \
                                                              avg_epoch / epoch_ref
                            else:
                                temp_sh["RatioAvg"] = 0
                            df_times = df_times.append(temp_sh, ignore_index=True)

                    temp = pd.DataFrame()
                    temp["Jobs"] = GPUtype_DF["Jobs"]
                    temp["ID"] = job[0]
                    temp["Epochs"] = GPUtype_DF["Epochs"]
                    temp["GPUtype"] = GPUtype_DF["GpuType"]
                    temp["nGPUs"] = GPUtype_DF["GpuNumber"]
                    temp["GPUf"] = 1.0
                    temp["ExecutionTime"] = GPUtype_DF["ExecutionTime"]
                    temp["min"] = 0.0
                    temp["max"] = 0.0
                    if not df_distributions.empty:
                        temp["Distribution"] = distrib
                        temp["RealExecutionTime"] = temp["ExecutionTime"] * \
                                                    real_epoch / epoch_ref
                    if average == "true":
                        temp["RatioAvg"] = (epoch_ref - avg_epoch) / real_epoch
                        temp["AvgExecutionTime"] = temp["ExecutionTime"] * \
                                                   avg_epoch / epoch_ref
                    else:
                        temp["RatioAvg"] = 0
                    # append rows to time dataframe
                    df_times = df_times.append(temp, ignore_index=True)
            # append current job ID to the list of examined jobs
            added_jobs.append(job[1])
        else:
            that_job = df_times[df_times["Jobs"] == job[1]].copy()
            uid = that_job.groupby(["ID"]).first().index[0]
            that_job = that_job[that_job["ID"] == uid]
            that_job["ID"] = job[0]
            if not df_distributions.empty:
                that_job["Distribution"] = distrib
                that_job["RealExecutionTime"] = that_job["ExecutionTime"] * \
                                                real_epoch / epoch_ref
            if average == "true":
                temp["RatioAvg"] = (epoch_ref - avg_epoch) / real_epoch
                temp["AvgExecutionTime"] = temp["ExecutionTime"] * \
                                           avg_epoch / epoch_ref
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
    max_nGPUs = df_times["nGPUs"].max()
    # group by job id
    times_by_job = times_one_GPU.groupby(["ID"])
    # min execution time per job
    execTime_min = times_by_job.min()["ExecutionTime"]

    lambdas = n_nodes * max_nGPUs / execTime_min
    tuning = 2.5

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
                rates[k_idx][0] += ((lambdas[jid] / 4) / tuning)
            elif local_distribution == "high":
                rates[k_idx][0] += ((lambdas[jid] / 1) / tuning)
            rates[k_idx][1] += 1
    rates = rates.loc[0] / rates.loc[1]

    return (rates, clusters)


def generate_deadline (SubmissionTime, min_time, max_time):
    return (SubmissionTime + np.random.uniform(min_time, max_time))


def generate_tardinessweight ():
    return (np.random.uniform(0.0253709514333333, 0.0443991650083333))


def generate_nodes (nN, df_costs, neglected_types = []):
    # get available types of GPUs
    df_costs_filtered = df_costs[~df_costs.GPUtype.isin(neglected_types)]
    costs_by_GPUtype = df_costs_filtered.groupby(["GPUtype"])
    types = list(costs_by_GPUtype.indices)
    n_types = len(types)
    min_n_GPUs = costs_by_GPUtype.min()["nGPUs"]
    max_n_GPUs = costs_by_GPUtype.max()["nGPUs"]
    # build nodes
    ID = 1
    Nodes = pd.DataFrame(columns=["ID", "GPUtype", "nGPUs", "cost"])
    for i in range(nN):
        temp = pd.DataFrame()
        temp["ID"] = ["n" + str(ID)]
        #idx = np.random.randint(0, n_types)
        idx = i % n_types
        GPUtype = types[idx]
        #nGPUs = np.random.randint(min_n_GPUs[GPUtype], max_n_GPUs[GPUtype])
        nGPUs = max_n_GPUs[GPUtype]
        temp["GPUtype"] = [GPUtype]
        temp["nGPUs"] = [nGPUs]
        temp["cost"] = [0.0]
        Nodes = Nodes.append(temp, ignore_index=True)
        ID += 1
    return Nodes


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
    if distribution == "exponential":
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


def generate_data (nN, nJ, myseed, distribution, lambdaa, scenario, stochastic):

    # custom distributions
    custom_distributions = ["high", "low", "mixed"]

    # neglected GPU n_types
    neglected_types = ["Quadro P600", "GTX 1080Ti"]

    # set seed to control random number generation and job selection
    np.random.seed(myseed)

    # generate folder to store new data
    folder_name = "../../build/calls/call_1-" + str(nN) + "-" + \
                  str(nJ) + "-" + str(myseed) + "-" + str(distribution) + \
                  "-" + gpu_sharing + "-" + stochastic + "-" + str(lambdaa)
    createFolder(folder_name)
    #
    folder_name += "/"

    # load data costs and distributions
    if scenario == "datacenter":
        # ["Application","Images","Epochs","Batchsize","Jobs","GpuType",
        #  "GpuNumber","ExecutionTime","min","max"]
        data_filename = "data_2d10d_filtered.csv"
        # ["GPUtype","nGPUs","PowerConsumption[W]","EnergyCost[€/kWh]",
        #  "PUE","cost"]
        cost_filename = "GPU-cost_server.csv"
    elif scenario == "regular":
        # ["Application","Images","Epochs","Batchsize","Jobs","GpuType",
        #  "GpuNumber","ExecutionTime","min","max"]
        data_filename = "data_2d10d_filtered.csv"
        # ["GPUtype","nGPUs","cost"]
        cost_filename = "GPU-cost.csv"
    else:
        # ["Jobs","Application","Epochs","Batchsize","GpuType","GpuNumber",
        #  "TimePerEpoch","ExecutionTime"]
        data_filename = "data_server.csv"
        # ["GPUtype","nGPUs","PowerConsumption[W]","EnergyCost[€/kWh]",
        #  "PUE","cost"]
        cost_filename = "GPU-cost_server_4-2.csv"
    data_file = "../../build/data/" + data_filename
    df_data = pd.read_csv(data_file)
    costs_file = "../../build/data/" + cost_filename
    df_costs = pd.read_csv(costs_file)

    if gpu_sharing == "True" or gpu_sharing == "true":
        fractions_file = "../../build/data/fractions.csv"
        df_fractions = pd.read_csv(fractions_file)
    else:
        df_fractions = pd.DataFrame()

    if stochastic == "True" or stochastic == "true" or stochastic_m == "true":
        distributions_file = "../../build/data/distributions.csv"
        df_distributions = pd.read_csv(distributions_file)
    else:
        df_distributions = pd.DataFrame()

    # generate list of selected jobs
    print("\tgenerate list of selected jobs")
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
    print("\tgenerate expected execution times")
    df_times = generate_df_times(df_data, selected_jobs, average,
                                 df_fractions, df_distributions,
                                 neglected_types)

    np.random.seed(myseed)

    # generate submission times of all selected jobs
    print("\tgenerate submission times")
    if distribution in custom_distributions:
        (SubmissionTimes, arrivals) = generate_custom_submissions(df_times,
                                                      len(selected_jobs), nN,
                                                      distribution)
    else:
        SubmissionTimes = generate_submissions(selected_jobs, lambdaa,
                                               nN, nJ, distribution)[0]

    # df_jobs: selected jobs with their characteristics
    print("\tgenerate jobs characteristics")
    df_jobs = pd.DataFrame(columns = ["ID", "SubmissionTime", "Deadline",
                                      "TardinessWeight", "MinExecTime",
                                      "MaxExecTime"])
    for job in selected_jobs:
        # select all jobs with the current ID
        timeDF = df_times[df_times["ID"] == job[0]]
        timeDF = timeDF[timeDF["GPUf"] >= 1]
        # get minimum and maximum execution time for the current job
        min_time = timeDF["ExecutionTime"].min()
        max_time = timeDF["ExecutionTime"].max()
        # submission time
        sub_time = SubmissionTimes[job[0]]

        # collect data of current job in a dataframe
        temp = pd.DataFrame()
        temp["ID"] = [job[0]]
        temp["SubmissionTime"] = [sub_time]
        # generate deadline and tardiness weight
        temp["Deadline"] = [generate_deadline(sub_time, min_time, max_time)]
        temp["TardinessWeight"] = [generate_tardinessweight()]
        if average == "true":
            min_time = timeDF["AvgExecutionTime"].min()
            max_time = timeDF["AvgExecutionTime"].max()
        temp["MinExecTime"] = [min_time]
        temp["MaxExecTime"] = [max_time]
        ratioavg = timeDF["RatioAvg"].iloc[0]
        temp["RatioAvg"] = [ratioavg]
        if stochastic_m == "true":
        # get epochs and distribution for the current job
            epochs = df_data[df_data["Jobs"] == job[1]]["Epochs"].iloc[0]
            distrib = timeDF["Distribution"].iloc[0]
            temp["Epochs"]      = [epochs]
            temp["Distribution"]= [distrib]

        # append dataframe to df_jobs
        df_jobs = df_jobs.append(temp, ignore_index=True)

    # write data to files
    print("\texporting data to files")
    df_jobs = df_jobs.sort_values(by="SubmissionTime")
    df_jobs.to_csv(folder_name + "Lof_Selectjobs.csv", index=False)

    if average == "true":
        df_times["ExecutionTime"] = df_times["AvgExecutionTime"]
    if stochastic == "True" or stochastic == "true":
        df_times.to_csv(folder_name + "SelectJobs_times.csv",
                        columns=["ID", "GPUtype", "nGPUs", "GPUf", "ExecutionTime",
                        "RealExecutionTime"], index=False)
    else:
        df_times.to_csv(folder_name + "SelectJobs_times.csv",
                        columns=["ID", "GPUtype", "nGPUs", "GPUf", "ExecutionTime"],
                        index=False)
    df_costs = df_costs[~df_costs.GPUtype.isin(neglected_types)]
    df_costs.to_csv(folder_name + "GPU-costs.csv", index=False)

    # generate nN nodes and save them to file
    df_nodes = generate_nodes(nN, df_costs, neglected_types)
    df_nodes.to_csv(folder_name + "tNodes.csv", index=False)


if __name__ == "__main__":
    nN           = int(sys.argv[1])
    nJ           = int(sys.argv[2])
    myseed       = int(sys.argv[3])
    distribution = sys.argv[4]
    scenario     = sys.argv[5]
    gpu_sharing  = sys.argv[6]
    stochastic   = sys.argv[7]
    stochastic_m = sys.argv[8]
    average      = sys.argv[9]
    if distribution == "poisson" or distribution == "exponential":
        lambdaa = int(sys.argv[10])
    else:
        lambdaa = "000"

    possible_distributions = ["poisson", "exponential", "low", "high" ,"mixed"]
    possible_scenarios = ["regular", "datacenter", "ANDREAS"]

    if (nN == 0 or nJ == 0 or distribution not in possible_distributions or \
        scenario not in possible_scenarios):
        print("Sorry!! the inputs are not valuable. Data generation failed.")
        sys.exit(1)
    else:
        print("Generating data for nN={}, nJ={}, seed={}, distribution={}, \
               scenario={}, lambda={}".format(nN, nJ, myseed, distribution,
                                              scenario, lambdaa))
        generate_data(nN, nJ, myseed, distribution, lambdaa, scenario, stochastic)
