#!/bin/bash

# build name of directory
DIR=$PWD
DIR=${DIR%output*}

# run experiment
$DIR/main "$@"
