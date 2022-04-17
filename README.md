# DS Project Spring 2022

## Instructions to run:

Inside the directory DS_Project:

### Run make (-j2 is optional) and make directory for Server 2:
```
make -j2
mkdir Server_2
```
### Copy a.out into Server_2 dir:
```
cp a.out Server_2
```
### Open another terminal inside Server_2 dir. For each terminal, run the following to start up the two servers. 
```
./a.out 127.0.0.1 < PORTNUMBER >
```
### Choose PORTNUMBER between 12341 to 12350 (both inclusive) different for each server. For example inside DS_Project dir:
```
./a.out 127.0.0.1 12345
```
### Inside Server_2 dir:
```
./a.out 127.0.0.1 12342
```
Similarly more servers can be started up (max. of 10 servers). You can also start servers in the sane directory (parent).

If you want to submit a new job press y , followed by entering executable file name: arrsum and input file name: inp.in. You can continue doing this.

## Note that you can submit job to the server started within DS_Project only (directory containing the executable and input file).


