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

import pandas as pd


def check_increasing_time (df_j, debug = False):
    all_increasing = True
    df_j_by_GPU = df_j.groupby(["GpuType"])
    for elem_t in df_j_by_GPU:
        if debug:
            print("\t gpu type: ", elem_t[0])
        elem = elem_t[1]
        max_nGPUs = elem["GpuNumber"].max()
        min_nGPUs = elem["GpuNumber"].min()
        time_at_max = elem[elem["GpuNumber"] == max_nGPUs]["ExecutionTime"]
        time_at_min = elem[elem["GpuNumber"] == min_nGPUs]["ExecutionTime"]
        if debug:
            print("\t\t time at ", min_nGPUs, " = ", time_at_min.iloc[0])
            print("\t\t time at ", max_nGPUs, " = ", time_at_max.iloc[0])
        if time_at_max.iloc[0] < time_at_min.iloc[0]:
            all_increasing = False
    return (all_increasing)

def filter_execution_times4 (debug = False):
    
    input_file  = "build/data/data.csv"
    output_file = "build/data/data_2d10d_filtered.csv"

    data = pd.read_csv(input_file)
    new_data = pd.DataFrame()

    min_time = 2  * 3600 * 24          # two days
    max_time = 10 * 3600 * 24          # ten days

    # group data by 'Jobs'
    group_by_jobs = data.groupby(['Jobs'])
    
    # for all jobs...
    count = 0
    for job in group_by_jobs:        
        # drop duplicated rows
        newj = job[1].drop_duplicates(subset=['GpuType','GpuNumber'], keep='last')
        check_max = max(newj['ExecutionTime']) < max_time
        check_min = max(newj['ExecutionTime']) > min_time
        if check_max:
            if debug:
                print("Examining job ", newj["Jobs"].iloc[0])
            # check that time is not increasing with the number of GPUs
            all_increasing = check_increasing_time(newj)
            if not all_increasing:
                count += 1
                temp_new_data = pd.DataFrame(newj, copy=True)
                temp_new_data['min'] = min(temp_new_data['ExecutionTime'])
                temp_new_data['max'] = max(temp_new_data['ExecutionTime'])    
                new_data = new_data.append(temp_new_data)

    print('# of considered jobs: ', count)

    new_data.to_csv(output_file, index=False)



if __name__ == "__main__":
    filter_execution_times4()