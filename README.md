# Checking which outputs are ours and which were already spent
The example shows how to check which outputs in a given [Monero](https://getmonero.org/)
transaction have already been spent. For this purpose, private view and spend keys,
as well the corresponding address are required.


## Prerequisites

The code was written and tested on Xubuntu 16.04 Beta 1 x86_64.

Instruction for Monero compilation:
 - [Ubuntu 16.04 x86_64](https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04/)

The Monero C++ development environment was set as shown in the above link.

## C++ code
The main part of the example is main.cpp.

```c++
int main(int ac, const char* av[]) {

    // ...
    // argument parsing and processing removed
    // to save some space here

    print("\n\ntx hash          : {} in block no. {}\n\n",
          tx_hash, cryptonote::get_block_height(blk));

    // lets check our keys
    print("private view key : {}\n", private_view_key);
    print("private spend key: {}\n", private_spend_key);
    print("address          : {}\n\n\n", address);


    // having our transaction, first we check which output in that
    // transactions are ours. For this we need to go through all outputs
    // in a transaction.

    std::vector<size_t> outputs_ids;

    uint64_t money_transfered {0};
    uint64_t miner_money_transfered {0};

    // look for our outputs in the transaction and the corresponding block reward
    cryptonote::lookup_acc_outs(account_keys, tx, outputs_ids, money_transfered);
    cryptonote::lookup_acc_outs(account_keys, blk.miner_tx, outputs_ids, miner_money_transfered);

    // get tx public key from extras field
    crypto::public_key pub_tx_key = cryptonote::get_tx_pub_key_from_extra(tx);

    if (outputs_ids.size())
    {
        print("We found our outputs: \n");

        // iterate over each output found
        // and create its key_image.
        //
        // to check if the given output is spend, we just need to check
        // whether the correspoding key_image is present in the blockchain
        for (size_t ouput_i: outputs_ids)
        {
            // get tx output
            const cryptonote::tx_out& tx_output =  tx.vout[ouput_i];

            // get tx output public key
            const cryptonote::txout_to_key& tx_out_to_key
                = boost::get<cryptonote::txout_to_key>(tx_output.target);

            print("Our output: {:s}, amount {:0.6f}\n",
                  tx_out_to_key.key, tx_output.amount / 1e12);

            // public tx key is combined with our private view key
            // to create, so called, derived key.
            crypto::key_derivation derivation;

            if (!generate_key_derivation(pub_tx_key, private_view_key, derivation))
            {
                cerr << "Cant get derived key for output with: " << "\n"
                     << "pub_tx_key: " << private_view_key << endl;

                return 1;
            }

            //print("Derived key: {:s}\n", derivation);

            // generate key_image of this output
            crypto::key_image key_image;

            if (!xmreg::generate_key_image(derivation,
                                           ouput_i, /* positoin in the tx */
                                           private_spend_key,
                                           account_keys.m_account_address.m_spend_public_key,
                                           key_image))
            {
                cerr << "Cant generate key image for output: "  << pub_tx_key << endl;
                return 1;
            }


            print("Key image generated: {:s}\n", key_image);

            // finally check if the key_image generated is present in the blockchain
            bool is_spent = core_storage.have_tx_keyimg_as_spent(key_image);

            print("Is output spent?: {}\n", is_spent);

            cout << endl;
        }
    }
    else
    {
        print("No our outputs were found in this transaction\n");
    }

    cout << "\nEnd of program." << endl;

    return 0;
}
```

## Program options

```
./checkoutputs -h

checkoutputs, check which outputs in a given tx are ours and if were spent:
  -h [ --help ] [=arg(=1)] (=0)    produce help message
  -t [ --txhash ] arg              transaction hash
  -v [ --viewkey ] arg             private view key string
  -s [ --spendkey ] arg            private spend key string
  -f [ --find-tx ] [=arg(=1)] (=0) find transaction containing key generated if
                                   it is spend (time consuming search)
  -a [ --address ] arg             monero address string
  -b [ --bc-path ] arg             path to lmdb blockchain
  --testnet [=arg(=1)] (=0)        is the address from testnet network
```

## Example result 1

Execute program with default parameters

```bash
./checkoutputs
```

```bash
tx hash          : <cda104278309e1637bbe8da841adb25d5d4f541de428e8e3808c83f9a71fe22e> in block no. 985103

private view key : <9c2edec7636da3fbb343931d6c3d6e11bcd8042ff7e11de98a8d364f31976c04>
private spend key: <950b90079b0f530c11801ef29e99618d3768d79d3d24972ff4b6fd9687b7b20c>
address          : <43A7NUmo5HbhJoSKbw9bRWW4u2b8dNfhKheTR5zxoRwQ7bULK5TgUQeAvPS5EVNLAJYZRQYqXCmhdf26zG2Has35SpiF1FP>
money received   : 1.000000


We found our outputs:
Our output: <d09860a6252b68045d8aa790e3d5e9b19341dcd09c6257fdb1e90918bece5b1d>, amount 1.000000
 - key image generated: <ef42203f6a80c7ecc42a6f4c78ecf126696a7f760eb86550bd639acbe7ab5978>

Is output with key_image <ef42203f6a80c7ecc42a6f4c78ecf126696a7f760eb86550bd639acbe7ab5978> spent?: true
```

## Example result 2

Execute program with default parameters, but this time look for transactions which cointain
the spent outputs.

```bash
./checkoutputs -f
```

```bash

```


## Example result 3

```bash

```

```bash

```


## Compile this example

If so then to download and compile this example, the following
steps can be executed:

```bash
# download the source code
git clone https://github.com/moneroexamples/check-if-output-was-spent.git

# enter the downloaded sourced code folder
cd check-if-output-was-spent

# create the makefile
cmake .

# compile
make
```

The Monero C++ development environment was set as shown in the following link:
- [Ubuntu 16.04 x86_64](https://github.com/moneroexamples/compile-monero-09-on-xubuntu-16-04-beta-1/)

## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

Some Monero are also welcome:
```
48daf1rG3hE1Txapcsxh6WXNe9MLNKtu7W7tKTivtSoVLHErYzvdcpea2nSTgGkz66RFP4GKVAsTV14v6G3oddBTHfxP6tU
```
