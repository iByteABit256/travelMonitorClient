#!/bin/bash

syntax=", syntax: ./create_infiles.sh inputFile input_dir numFilesPerDirectory"

# argument checking

	# checks if file exists and is not empty
[ -s $1 ] || { echo "$1 is invalid"$syntax && exit; }
	# check if input directory exists
[ ! -e $2 ] || { echo "$2 must be a non-existing directory"$syntax && exit; }
	# checks if numFiles is greater than zero
[ $3 -gt 0 ] || { echo "$3 is invalid"$syntax && exit; }

# input file
inputFile=$1

# input directory
inputDir=$2

# number of files per directory
numFiles=$3

# create directory
mkdir $inputDir

# parse input file and add records to files with round robin order
cat $inputFile | awk -v n=$numFiles -v dir=$inputDir '
	{
		if (countries[$4] == 0){
			DIR=dir$4
			system("mkdir " DIR)
			for (i = 0; i < n; i++){
				touch dir$4"/"$4"-"i+1".txt"
			}	
		}
		print $0 >> dir$4"/"$4"-"(countries[$4]++%n)+1".txt"
	}
'

