#!/bin/bash

syntax=", syntax: ./testFile.sh virusesFile countriesFile numLines duplicatesAllowed"

# argument checking

test -f $1 || { echo "$1 is invalid"$syntax && exit; }
test -f $2 || { echo "$2 is invalid"$syntax && exit; }
[ $3 -gt 0 ] || { echo "$3 is invalid"$syntax && exit; }
[ $4 -eq 0 ] || [ $4 -eq 1 ] || { echo "$4 is invalid"$syntax && exit; }

# virus file
vF=$1

# countries file
cF=$2

# number of output lines
numLines=$3

# duplicates allowed
duplicates=$4

dupflag=""
if [ $duplicates -eq 1 ]; then
	dupflag="-r"
fi

# delete pre-existing input file and create new file
if [ -f inputFiles/inputFile.txt ]; then
	rm inputFiles/inputFile.txt
fi

touch inputFiles/inputFile.txt

# create arrays for every record field
citizenIDs=()
# fill with random numbers from 0-9999 (with or without duplicates depending on arguments)
mapfile citizenIDs < <(shuf -i 0-9999 $dupflag -n $numLines)
fnames=()
lnames=()

# create names
i=0
while [ $i -lt $numLines ]; do
	# random length in [3,12]
	fnameLength=$((RANDOM%10+3))

	# random string of capitals
	fname=$(tr -dc A-Z < /dev/urandom | head -c ${fnameLength})

	lnameLength=$((RANDOM%10+3))

	lname=$(tr -dc A-Z < /dev/urandom | head -c ${lnameLength})

	fnames[$i]=$fname
	lnames[$i]=$lname

	let i+=1;	
done

countries=()
# fill with entries from file allowing duplicates
mapfile countries < <(shuf -r -n $numLines $cF)

ages=()
# fill with numbers in [1-120] with duplicates
mapfile ages < <(shuf -i 1-120 -r -n $numLines)

viruses=()
mapfile viruses < <(shuf -r -n $numLines $vF)

yes=()
# fill with random booleans
mapfile yes < <(shuf -i 0-1 -r -n $numLines)

day=()
mapfile day < <(shuf -i 1-30 -r -n $numLines)

month=()
mapfile month < <(shuf -i 1-12 -r -n $numLines)

year=()
mapfile year < <(shuf -i 1900-2021 -r -n $numLines)

# echo all arrays line and redirect to output file
i=0
while [ $i -lt $numLines ]; do

	echo -n ${citizenIDs[$i]} ${fnames[$i]} ${lnames[$i]}\
		${countries[$i]} ${ages[$i]} ${viruses[$i]} >> inputFiles/inputFile.txt

	# include date if record has YES
	if [ ${yes[$i]} -eq 1 ]; then
		echo -n " YES" ${day[$i]} >> inputFiles/inputFile.txt
		echo -n -${month[$i]} >> inputFiles/inputFile.txt
		echo -${year[$i]} >> inputFiles/inputFile.txt
	else
		echo " NO" >> inputFiles/inputFile.txt
	fi

	let i+=1;
done
