#!/bin/bash

initial_path=$1
nN=$2
echo $initial_path
echo $nN

data_path=$initial_path/results/xxx

for f in $(ls $data_path); do
	echo
	echo $f
	python3 script/py/plot_NJ.py $data_path/$f/1-$nN-${nN}0-1000-exponential-30000/results_$f $nN -o exponential_$f -p $initial_path/figures
	python3 script/py/plot_NJ.py $data_path/$f/1-$nN-${nN}0-1000-high-000/results_$f $nN -o high_$f -p $initial_path/figures
	python3 script/py/plot_NJ.py $data_path/$f/1-$nN-${nN}0-1000-low-000/results_$f $nN -o low_$f -p $initial_path/figures
	python3 script/py/plot_NJ.py $data_path/$f/1-$nN-${nN}0-1000-mixed-000/results_$f $nN -o mixed_$f -p $initial_path/figures
done
