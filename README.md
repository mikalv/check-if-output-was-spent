# Checking which our outputs were already spent
The example shows how to check which outputs in a given [Monero](https://getmonero.org/)
transaction have already been spent. For this purpose, private view and spend keys,
and address are required.


## Pre-requisites

Everything here was done and tested on Xubuntu 16.04 Beta 1 x86_64.

Instruction for Monero compilation:
 - [Ubuntu 16.04 x86_64](https://github.com/moneroexamples/compile-monero-09-on-xubuntu-16-04-beta-1/)


## C++ code
The main part of the example is main.cpp.

```c++
```

## Program options

```
./checkoutputs -h

checkoutputs, check which outputs in a given tx are ours and wich were spent:
  -h [ --help ] [=arg(=1)] (=0) produce help message
  -t [ --txhash ] arg           transaction hash
  -v [ --viewkey ] arg          private view key string
  -s [ --spendkey ] arg         private spend key string
  -a [ --address ] arg          monero address string
  -b [ --bc-path ] arg          path to lmdb blockchain
  --testnet [=arg(=1)] (=0)     is the address from testnet network
```

## Example input and output





## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

Some Monero are also welcome:
```
48daf1rG3hE1Txapcsxh6WXNe9MLNKtu7W7tKTivtSoVLHErYzvdcpea2nSTgGkz66RFP4GKVAsTV14v6G3oddBTHfxP6tU
```
