# GPUScheduler

A GPU scheduler to optimise the execution of Deep Learning training jobs on GPU-accelerated clusters, minimising the operational costs and the due dates violations

Files organization:
- include/ folder: header files for the C++ program
- src/ folder: source files for the C++ program
- script/ folder: scripts, organized by type
	- sh/ folder: bash scripts
	- py/ folder: python scripts
- build/ folder: object files and executable of the C++ program are generated here; store here folders for data and results

Intended usage of the program (simple version)
1. edit the configuration file config.ini
2. ```make list```
3. ```make data```
4. ```make tests```

More complex options are available; type ```make help``` for further information

