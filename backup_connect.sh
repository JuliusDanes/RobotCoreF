#!/bin/bash
jumlah0=0;
n=0;
while [[ true ]]; do
	#statements
	jumlah1=0;
    while IFS="," read -r d1; do
    	let jumlah1++;
    done < backup_connect.txt
	if [[ $jumlah0 == $jumlah1 ]]; then
		let n++;
		if [[ $n == 10 ]]; then
			n=0;
			cd ..
			fuser -fk 8686/tcp
			g++ -pthread -std=c++11 -o main main.cpp
			./main
		fi
	else
		n=0;
	fi
	jumlah0=$jumlah1;
	echo $n;
	sleep 0.1
done