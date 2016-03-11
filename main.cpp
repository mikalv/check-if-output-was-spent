#include "src/MicroCore.h"
#include "src/CmdLineOptions.h"
#include "src/tools.h"

#include "ext/format.h"

using namespace std;
using namespace fmt;

using xmreg::operator<<;
using xmreg::print_sig;


using boost::filesystem::path;

unsigned int epee::g_test_dbg_lock_sleep = 0;



struct for_signatures
{
    crypto::hash tx_hash ;
    crypto::key_image kimg ;
    std::vector<crypto::public_key> outs_pub_keys;
    cryptonote::keypair in_ephemeral;
    size_t real_output {0};
};




int main(int ac, const char* av[]) {

    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    auto help_opt = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return 0;
    }

    // get other options
    auto tx_hash_opt = opts.get_option<string>("txhash");
    auto viewkey_opt = opts.get_option<string>("viewkey");
    auto spendkey_opt = opts.get_option<string>("spendkey");
    auto address_opt = opts.get_option<string>("address");
    auto bc_path_opt = opts.get_option<string>("bc-path");
    bool testnet     = *(opts.get_option<bool>("testnet"));
    bool find_tx     = *(opts.get_option<bool>("find-tx"));

    // get the program command line options, or
    // some default values for quick check
    string tx_hash_str = tx_hash_opt ?
                         *tx_hash_opt :
                         "cda104278309e1637bbe8da841adb25d5d4f541de428e8e3808c83f9a71fe22e";


    string viewkey_str = viewkey_opt ?
                         *viewkey_opt :
                         "9c2edec7636da3fbb343931d6c3d6e11bcd8042ff7e11de98a8d364f31976c04";


    string spendkey_str = spendkey_opt ?
                         *spendkey_opt :
                         "950b90079b0f530c11801ef29e99618d3768d79d3d24972ff4b6fd9687b7b20c";

    string address_str = address_opt ?
                          *address_opt :
                          "43A7NUmo5HbhJoSKbw9bRWW4u2b8dNfhKheTR5zxoRwQ7bULK5TgUQeAvPS5EVNLAJYZRQYqXCmhdf26zG2Has35SpiF1FP";


    crypto::hash tx_hash;

    if (!xmreg::parse_str_secret_key(tx_hash_str, tx_hash))
    {
        cerr << "Cant parse tx hash: " << tx_hash_str << endl;
        return 1;
    }

    crypto::secret_key private_view_key;

    // parse string representing given private viewkey
    if (!xmreg::parse_str_secret_key(viewkey_str, private_view_key))
    {
        cerr << "Cant parse view key: " << viewkey_str << endl;
        return 1;
    }

    crypto::secret_key private_spend_key;

    // parse string representing given private spend
    if (!xmreg::parse_str_secret_key(spendkey_str, private_spend_key))
    {
        cerr << "Cant parse view key: " << spendkey_str << endl;
        return 1;
    }

    cryptonote::account_public_address address;

    // parse string representing given monero address
    if (!xmreg::parse_str_address(address_str,  address, testnet))
    {
        cerr << "Cant parse address: " << address_str << endl;
        return 1;
    }

    path blockchain_path;

    if (!xmreg::get_blockchain_path(bc_path_opt, blockchain_path))
    {
        // if problem obtaining blockchain path, finish.
        return 1;
    }

    print("Blockchain path      : {}\n", blockchain_path);

    // enable basic monero log output
    xmreg::enable_monero_log();

    // create instance of our MicroCore
    xmreg::MicroCore mcore;

    // initialize the core using the blockchain path
    if (!mcore.init(blockchain_path.string()))
    {
        cerr << "Error accessing blockchain." << endl;
        return 1;
    }

    // get the high level cryptonote::Blockchain object to interact
    // with the blockchain lmdb database
    cryptonote::Blockchain& core_storage = mcore.get_core();

    cryptonote::transaction tx;

    try
    {
        // get transaction with given hash
        tx = core_storage.get_db().get_tx(tx_hash);
    }
    catch (const std::exception& e)
    {
        cerr << e.what() << endl;
        return false;
    }

    // create accounts_keys instance
    cryptonote::account_keys account_keys {address,
                                           private_spend_key,
                                           private_view_key};

    // find block in which the given transaction is located
    cryptonote::block blk ;

    if (!mcore.get_block_by_tx_hash(tx_hash, blk))
    {
        cerr << "Cant find block for the given transaction" << endl;
        return false;
    }

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


            print("Key image generated: {:s}\n", key_image);

            // finally check if the key_image generated is present in the blockchain
            bool is_spent = core_storage.have_tx_keyimg_as_spent(key_image);

            print("Is output spent?: {}\n", is_spent);

            if (is_spent && find_tx)
            {
                crypto::hash tx_with_the_key;

                print("\t Searching for the transaction having the key found ...\n");

                if (!mcore.find_tx_with_key_image(key_image, tx_with_the_key, true))
                {
                    cerr << "\nTransaction with the spend key not found O.o?" << endl;
                    return 1;
                }

                print("Tx hash found: {:s}\n", tx_with_the_key);
            }

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
