# Checking which our outputs were already spent
The example shows how to check which outputs in a given [Monero](https://getmonero.org/)
transaction have already been spent. For this purpose, private view and spend keys,
and address are required.


## Pre-requisites

Everything here was done and tested
on Xubuntu 16.04 Beta 1 x86_64.

Instruction for Monero compilation:
 - [Ubuntu 16.04 x86_64](https://github.com/moneroexamples/compile-monero-09-on-xubuntu-16-04-beta-1/)


## C++ code
The main part of the example is main.cpp.

```c++
```

## Program options

```
./checkoutputs -h
```

## Example input and output


## Compile this example
The dependencies are same as those for Monero, so I assume Monero compiles
correctly. If so then to download and compile this example, the following
steps can be executed:

```bash
# download the source code
git clone https://github.com/moneroexamples/ring-signatures.git

# enter the downloaded sourced code folder
cd ring-signatures

# create the makefile
cmake .

# compile
make
```

After this, `rings` executable file should be present in access-blockchain-in-cpp
folder. How to use it, can be seen in the above example outputs.


## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

Some Monero are also welcome:
```
48daf1rG3hE1Txapcsxh6WXNe9MLNKtu7W7tKTivtSoVLHErYzvdcpea2nSTgGkz66RFP4GKVAsTV14v6G3oddBTHfxP6tU
```
