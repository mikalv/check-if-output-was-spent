# Checking which outputs are ours and if they were spent
The example shows how to check which outputs in a given [Monero](https://getmonero.org/)
transaction have already been spent. For this purpose, private view and spend keys,
as well the corresponding address are required.


## Prerequisites

The code was written and tested on Ubuntu 16.04 Beta x86_64.

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
    print("address          : {}\n", address);


    // having our transaction, first we check which output in that
    // transactions are ours. For this we need to go through all outputs
    // in a transaction.

    std::vector<size_t> outputs_ids;

    uint64_t money_transfered {0};

    // look for our outputs in the transaction and the corresponding block reward
    cryptonote::lookup_acc_outs(account_keys, tx, outputs_ids, money_transfered);

    print("money received   : {:0.6f}\n\n\n", money_transfered / 1e12);

    // get tx public key from extras field
    crypto::public_key pub_tx_key = cryptonote::get_tx_pub_key_from_extra(tx);

    vector<crypto::key_image> key_images_found;

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
            // the derived key is used to produce the key_image
            // that we want.
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

            key_images_found.push_back(key_image);


            print(" - key image generated: {:s}\n", key_image);

        }
    }
    else
    {
        print("No our outputs were found in this transaction\n");
        return 0;
    }

    cout << endl;

    vector<crypto::key_image> spent_key_images;

    // check which of the key_images generated
    // has already been spend
    for (crypto::key_image& key_img: key_images_found)
    {


        // finally check if the key_image generated is present in the blockchain
        bool is_spent = core_storage.have_tx_keyimg_as_spent(key_img);


        if (is_spent)
        {
            spent_key_images.push_back(key_img);
        }

        print("Is output with key_image {:s} spent?: {}\n",
              key_img, is_spent);
    }


    // search for transactions containing key_images generated
    // which were found to be spent. So basically we want to know
    // in which transaction each key was spent.
    if (find_tx && !spent_key_images.empty())
    {

        print("\nSearching for the transactions having the spend keys found ...\n");

        unordered_map<crypto::key_image, crypto::hash> txs_found;

        txs_found = mcore.find_txs_with_key_images(spent_key_images, true);


        if (txs_found.empty())
        {
            cerr << "\nTransactions not found for spend keys O.o?" << endl;
            return 1;
        }

        cout << endl;

        print("The found transactions are:\n");

        // find transactions containing the spent keys found
        for (auto& key_tx: txs_found)
        {

            // key_tx.first is the key_image
            // key_tx.second is the tx hash with the key

            cryptonote::block blk;

            if (!mcore.get_block_by_tx_hash(key_tx.second, blk))
            {
                cerr << "Cant find block for the given transaction" << endl;
                return false;
            }

            uint64_t blk_height = cryptonote::get_block_height(blk);

            print(" - Key image, tx (block height) found: {:s}, {:s} ({:d})\n",
                  key_tx.first, key_tx.second, blk_height);

        }

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
tx hash          : <cda104278309e1637bbe8da841adb25d5d4f541de428e8e3808c83f9a71fe22e> in block no. 985103

private view key : <9c2edec7636da3fbb343931d6c3d6e11bcd8042ff7e11de98a8d364f31976c04>
private spend key: <950b90079b0f530c11801ef29e99618d3768d79d3d24972ff4b6fd9687b7b20c>
address          : <43A7NUmo5HbhJoSKbw9bRWW4u2b8dNfhKheTR5zxoRwQ7bULK5TgUQeAvPS5EVNLAJYZRQYqXCmhdf26zG2Has35SpiF1FP>
money received   : 1.000000


We found our outputs:
Our output: <d09860a6252b68045d8aa790e3d5e9b19341dcd09c6257fdb1e90918bece5b1d>, amount 1.000000
 - key image generated: <ef42203f6a80c7ecc42a6f4c78ecf126696a7f760eb86550bd639acbe7ab5978>

Is output with key_image <ef42203f6a80c7ecc42a6f4c78ecf126696a7f760eb86550bd639acbe7ab5978> spent?: true

Searching for the transactions having the spend keys found ...
	 - checking tx no: 719318/1583736
	 - tx found for key_img: <ef42203f6a80c7ecc42a6f4c78ecf126696a7f760eb86550bd639acbe7ab5978>
	- all keys found! :-)

The found transactions are:
 - Key image, tx (block height) found: <ef42203f6a80c7ecc42a6f4c78ecf126696a7f760eb86550bd639acbe7ab5978>, <2c186ffeb72b26eb17993e81c9590e970c9432494efdb45694c70f3082583e74> (985113)
```
./checkoutputs -a 41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB -v fed77158ec692fe9eb951f6aeb22c3bda16fe8926c1aac13a5651a9c27f34309 -s 1eaa41781d5f880dc69c9379e281225c781a6db8dc544a26008e7a07890afa03 -f -t 1b27f4d6298e0a893609d965092802bef7dbfc40826e92f5957be9cf62d5031b

## Example result 3

Test non-spend output.


```bash
./checkoutputs -a 41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB -v fed77158ec692fe9eb951f6aeb22c3bda16fe8926c1aac13a5651a9c27f34309 -s 1eaa41781d5f880dc69c9379e281225c781a6db8dc544a26008e7a07890afa03 -f -t 1b27f4d6298e0a893609d965092802bef7dbfc40826e92f5957be9cf62d5031b
```

```bash
tx hash          : <1b27f4d6298e0a893609d965092802bef7dbfc40826e92f5957be9cf62d5031b> in block no. 985026

private view key : <fed77158ec692fe9eb951f6aeb22c3bda16fe8926c1aac13a5651a9c27f34309>
private spend key: <1eaa41781d5f880dc69c9379e281225c781a6db8dc544a26008e7a07890afa03>
address          : <41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB>
money received   : 0.070000


We found our outputs:
Our output: <ab33da6d8827607f224d775c6fbead998d254db767feca3e8285d30fffbf4d6e>, amount 0.070000
 - key image generated: <d3aee79cc00cdeafd34f5b348780cd1590f6cd004eb37235c6858eab0ec185c9>

Is output with key_image <d3aee79cc00cdeafd34f5b348780cd1590f6cd004eb37235c6858eab0ec185c9> spent?: false
```

## Example result 4

What will be the result if the output (i.e., ab33da6d8827607f224d775c6fbead998d254db767feca3e8285d30fffbf4d6e) from example 3 is spent? For this we just repeat the command and get new results.

```bash
./checkoutputs -a 41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB -v fed77158ec692fe9eb951f6aeb22c3bda16fe8926c1aac13a5651a9c27f34309 -s 1eaa41781d5f880dc69c9379e281225c781a6db8dc544a26008e7a07890afa03 -f -t 1b27f4d6298e0a893609d965092802bef7dbfc40826e92f5957be9cf62d5031b
```

```bash
tx hash          : <1b27f4d6298e0a893609d965092802bef7dbfc40826e92f5957be9cf62d5031b> in block no. 985026

private view key : <fed77158ec692fe9eb951f6aeb22c3bda16fe8926c1aac13a5651a9c27f34309>
private spend key: <1eaa41781d5f880dc69c9379e281225c781a6db8dc544a26008e7a07890afa03>
address          : <41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB>
money received   : 0.070000


We found our outputs:
Our output: <ab33da6d8827607f224d775c6fbead998d254db767feca3e8285d30fffbf4d6e>, amount 0.070000
 - key image generated: <d3aee79cc00cdeafd34f5b348780cd1590f6cd004eb37235c6858eab0ec185c9>

Is output with key_image <d3aee79cc00cdeafd34f5b348780cd1590f6cd004eb37235c6858eab0ec185c9> spent?: true

Searching for the transactions having the spend keys found ...
	 - checking tx no: 306969/1583765
	 - tx found for key_img: <d3aee79cc00cdeafd34f5b348780cd1590f6cd004eb37235c6858eab0ec185c9>
	- all keys found! :-)

The found transactions are:
 - Key image, tx (block height) found: <d3aee79cc00cdeafd34f5b348780cd1590f6cd004eb37235c6858eab0ec185c9>, <75d9a1980294a9c6a96736c8e546b40f6f2640c1fba61853ccd986891c597d31> (1010850)
```


## Example result 5

So now lets check the transaction in which we got our change, i.e., (tx hash: 75d9a1980294a9c6a96736c8e546b40f6f2640c1fba61853ccd986891c597d31) from example 4

```bash
./checkoutputs -a 41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB -v fed77158ec692fe9eb951f6aeb22c3bda16fe8926c1aac13a5651a9c27f34309 -s 1eaa41781d5f880dc69c9379e281225c781a6db8dc544a26008e7a07890afa03 -f -t 75d9a1980294a9c6a96736c8e546b40f6f2640c1fba61853ccd986891c597d31
```

```bash
tx hash          : <75d9a1980294a9c6a96736c8e546b40f6f2640c1fba61853ccd986891c597d31> in block no. 1010850

private view key : <fed77158ec692fe9eb951f6aeb22c3bda16fe8926c1aac13a5651a9c27f34309>
private spend key: <1eaa41781d5f880dc69c9379e281225c781a6db8dc544a26008e7a07890afa03>
address          : <41vEA7Ye8Bpeda6g59v5t46koWrVn2PNgEKgzquJjmiKCFTsh9gajr8J3pad49rqu581TAtFGCH9CYTCkYrCpuWUG9GkgeB>
money received   : 0.450000


We found our outputs:
Our output: <8c086f9fe0537aef00bc74e8cb912425f34c71b39f5d5d93fb7ba4c3398ffd5d>, amount 0.050000
 - key image generated: <df1504b992cc9fafba99461d7ca53a7a8cac224864ea4d113fcd331b4c2c052e>
Our output: <ada4ed75182403d858ad1fa26bba7f51c20c0f596e920da71fd8961474d6b535>, amount 0.400000
 - key image generated: <79533fc9827883dc9a497d3d7959cee4c232b675103a970b8c98427cf8137fca>

Is output with key_image <df1504b992cc9fafba99461d7ca53a7a8cac224864ea4d113fcd331b4c2c052e> spent?: false
Is output with key_image <79533fc9827883dc9a497d3d7959cee4c232b675103a970b8c98427cf8137fca> spent?: false
```

The outputs, as expected, havent been spent yet, as we just got them. But we can already
see that key images for these ouputs that will be generated when the outputs are going to
be spend.

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
- [Ubuntu 16.04 x86_64](https://github.com/moneroexamples/compile-monero-09-on-ubuntu-16-04/)

## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

Some Monero are also welcome:
```
48daf1rG3hE1Txapcsxh6WXNe9MLNKtu7W7tKTivtSoVLHErYzvdcpea2nSTgGkz66RFP4GKVAsTV14v6G3oddBTHfxP6tU
```
