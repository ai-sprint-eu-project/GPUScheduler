#!/usr/bin/env python3
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
import pandas as pd


def createFolder (directory):
  try:
      if not os.path.exists(directory):
          os.makedirs(directory)
  except OSError:
      print ("Error: Creating directory. " +  directory)


def convert_data(nN, nJ, myseed, distribution, lambdaa):
  
  # basedir of original data and basedir for new data
  path = os.path.abspath(sys.argv[0]).split('/')[:-4]
  path = "/".join(path)
  basedir = path + "/GPUspb/build/data/tests_new/"
  new_basedir = path + "/GPUOptmization/data/from_loading_data_" + distribution

  # name of original data folder
  folder_name = "1-" + str(nN) + "-" + str(nJ) + "-" + str(myseed) + "-" + \
                str(distribution) + "-" + str(lambdaa) + "/"
  
  # create new data folder
  new_folder_name = "1-" + str(nN) + "-" + str(nJ) + "-" + str(lambdaa) +\
                    "-1-" + str(myseed) + "/"
  address = new_basedir + "/" + new_folder_name
  createFolder(address)

  # original data:
  #
  # Lof_Selectjobs = ["Application", "Images", "Epochs", "Batchsize", "Jobs", 
  #                   "UniqueJobsID", "SubmissionTime", "Deadline", 
  #                   "Tardinessweight", "MinExecTime", "MaxExecTime"]
  #
  # SelectJobs_times = ["UniqueJobsID", "VMtype", "GPUtype", "nGPUs", 
  #                     "max_nGPUs", "cost", "ExecutionTime"]
  #
  # tNodes = ["Nodes"]
  #
  Lof_Selectjobs = pd.read_csv(basedir + folder_name + "Lof_Selectjobs.csv")
  SelectJobs_times = pd.read_csv(basedir + folder_name + "SelectJobs_times.csv")
  tNodes = pd.read_csv(basedir + folder_name + "tNodes.csv")

  # new data:
  #
  # L_Selectjobs = ['Application', 'Images', 'Epochs', 'Batchsize',
  #                 'JobID', 'Generated_JobID']
  L_Selectjobs = Lof_Selectjobs.iloc[:,:6].copy()
  L_Selectjobs.columns = ['Application', 'Images', 'Epochs', 'Batchsize',
                          'JobID', 'Generated_JobID']
  L_Selectjobs.to_csv(address + "Lof_selectjobs.csv", index=False)
  #
  # L_nodes = ['Nodes']
  tNodes.to_csv(address + "tNodes.csv", index=False)
  #
  # L_time1 = ['Jobs', 'VMs', 'available_GPUs', 'time'] - unique ID
  L_time1 = SelectJobs_times.filter(['UniqueJobsID', 'VMtype', 'nGPUs', 
                                    'ExecutionTime'])
  L_time1.columns = ['Jobs', 'VMs', 'available_GPUs', 'time']
  L_time1.to_csv(address + "ttime.csv", index=False)
  #
  # L_time2 = ['Jobs', 'VMtype', 'GpuNumber', 'ExecutionTime'] - original ID
  L_time2 = pd.DataFrame()
  for job in L_time1.groupby(["Jobs"]):
    ID = job[0]
    original_ID = L_Selectjobs[L_Selectjobs["Generated_JobID"]==ID]["JobID"]
    job[1]["Jobs"] = original_ID.iloc[0]
    L_time2 = L_time2.append(job[1], ignore_index=True)
  L_time2.columns = ['Jobs', 'GpuType', 'GpuNumber', 'ExecutionTime']
  L_time2.to_csv(address + "mytime.csv", index=False)
  #
  # L_jobs = ['Jobs'] - original and unique ID
  Lof_Selectjobs["Jobs"].to_csv(address + "tempJobs.csv", index=False)
  L_jobs = Lof_Selectjobs["UniqueJobsID"]
  L_jobs.columns = ["Jobs"]
  L_jobs.to_csv(address + "tJobs.csv", index=False)
  #
  # L_deadline = ['Jobs', 'deadline']
  L_deadline = Lof_Selectjobs.filter(['UniqueJobsID', 'Deadline'])
  L_deadline.columns = ['Jobs', 'deadline']
  L_deadline.to_csv(address + "tdeadline.csv", index=False)
  #
  # L_tardinessweight = ['Jobs', 'tardinessweight']
  L_tardinessweight = Lof_Selectjobs.filter(['UniqueJobsID', 'Tardinessweight'])
  L_tardinessweight.columns = ['Jobs', 'tardinessweight']
  L_tardinessweight.to_csv(address + "ttardinessweight.csv", index=False)
  #
  # L_SubmissionTime = ['Jobs', 'submission_time']
  L_SubmissionTime = Lof_Selectjobs.filter(['UniqueJobsID', 'SubmissionTime'])
  L_SubmissionTime.columns = ['Jobs', 'submission_time']
  L_SubmissionTime.to_csv(address + "SubmissionTime.csv", index=False)
  #
  # L_VMs = ["VMs"]
  # L_costs = ["VMs", "cost"]
  # L_availGPUs = ['VMs', 'available_GPUs']
  L_VMs = pd.DataFrame()
  L_costs = pd.DataFrame()
  L_availGPUs = pd.DataFrame()
  VMs = []
  costs = []
  GPUs = []
  group_by_VM = SelectJobs_times.groupby(["VMtype"])
  for VM in group_by_VM:
      VMs.append(VM[0])
      costs.append(VM[1].iloc[0]["cost"])
      GPUs.append(VM[1].iloc[0]["max_nGPUs"])
  L_VMs["VMs"] = VMs
  L_costs["VMs"] = VMs
  L_costs["cost"] = costs
  L_availGPUs["VMs"] = VMs
  L_availGPUs["available_GPUs"] = GPUs
  L_VMs.to_csv(address + "myVMs.csv", index=False)
  L_costs.to_csv(address + "mycost.csv", index=False)
  L_availGPUs.to_csv(address + "myavailable_GPUs.csv", index=False)


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
    
  if (nN == 0 or nJ == 0 or distribution not in possible_distributions):
      print("Sorry!! the inputs are not valuable. Data generation failed.")
      sys.exit(1)
  else:
      convert_data(nN, nJ, myseed, distribution, lambdaa)
