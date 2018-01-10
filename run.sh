#/bin/bash

fuser -k 6012/tcp
fuser -k 6013/tcp
fuser -k 6014/tcp
fuser -k 6015/tcp
fuser -k 6016/tcp

rm -r ./servidor1
rm -r ./servidor2
rm -r ./servidor3

rm -r ./cliente/files/*
rm -r ./servidor/files/*

gcc -pthread -o ./servidor/servidor ./servidor/servidor.c

cp -r servidor servidor1
cp -r servidor servidor2
cp -r servidor servidor3

cd servidor
xterm -hold -e './servidor' & 

sleep 2
cd ..
cd servidor1
xterm -hold -e './servidor 6014' & 

sleep 2
cd ..
cd servidor2
xterm -hold -e './servidor 6015' & 

sleep 2
cd ..
cd servidor3
xterm -hold -e './servidor 6016' & 

sleep 2
cd ..
cd cliente
xterm -hold -e './cliente' & 


