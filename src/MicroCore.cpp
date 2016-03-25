//
// Created by mwo on 5/11/15.
//

#include "MicroCore.h"

namespace xmreg
{
    /**
     * The constructor is interesting, as
     * m_mempool and m_blockchain_storage depend
     * on each other.
     *
     * So basically m_mempool is initialized with
     * reference to Blockchain (i.e., Blockchain&)
     * and m_blockchain_storage is initialized with
     * reference to m_mempool (i.e., tx_memory_pool&)
     *
     * The same is done in cryptonode::core.
     */
    MicroCore::MicroCore():
            m_mempool(m_blockchain_storage),
            m_blockchain_storage(m_mempool)
    {}


    /**
     * Initialized the MicroCore object.
     *
     * Create BlockchainLMDB on the heap.
     * Open database files located in blockchain_path.
     * Initialize m_blockchain_storage with the BlockchainLMDB object.
     */
    bool
    MicroCore::init(const string& blockchain_path)
    {
        int db_flags = 0;

        db_flags |= MDB_NOSYNC ;

        BlockchainDB* db = nullptr;
        db = new BlockchainLMDB();

        try
        {
            // try opening lmdb database files
            db->open(blockchain_path, db_flags);
        }
        catch (const std::exception& e)
        {
            cerr << "Error opening database: " << e.what();
            return false;
        }

        // check if the blockchain database
        // is successful opened
        if(!db->is_open())
        {
            return false;
        }

        // initialize Blockchain object to manage
        // the database.
        return m_blockchain_storage.init(db, false);
    }

    /**
    * Get m_blockchain_storage.
    * Initialize m_blockchain_storage with the BlockchainLMDB object.
    */
    Blockchain&
    MicroCore::get_core()
    {
        return m_blockchain_storage;
    }

    /**
     * Get block by its height
     *
     * returns true if success
     */
    bool
    MicroCore::get_block_by_height(const uint64_t& height, block& blk)
    {

        crypto::hash block_id;

        try
        {
            block_id = m_blockchain_storage.get_block_id_by_height(height);
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            return false;
        }


        if (!m_blockchain_storage.get_block_by_hash(block_id, blk))
        {
            cerr << "Block with hash " << block_id
                 << "and height " << height << " not found!"
                 << endl;
            return false;
        }

        return true;
    }


    bool
    MicroCore::get_block_by_tx_hash(const crypto::hash& tx_hash, block& blk)
    {

        try
        {
            // find block in which the given transaction is located
            uint64_t tx_blk_height = m_blockchain_storage.get_db().get_tx_block_height(tx_hash);
            blk = m_blockchain_storage.get_db().get_block_from_height(tx_blk_height);
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            return false;
        }

        return true;
    }



    /**
     * Get transaction tx from the blockchain using it hash
     */
    bool
    MicroCore::get_tx(const crypto::hash& tx_hash, transaction& tx)
    {
        try
        {
            // get transaction with given hash
            tx = m_blockchain_storage.get_db().get_tx(tx_hash);
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            return false;
        }

        return true;
    }




    /**
     * Find output with given public key in a given transaction
     */
    bool
    MicroCore::find_output_in_tx(const transaction& tx,
                                 const public_key& output_pubkey,
                                 tx_out& out,
                                 size_t& output_index)
    {

        size_t idx {0};

        // search in the ouputs for an output which
        // public key matches to what we want
        auto it = std::find_if(tx.vout.begin(), tx.vout.end(),
                               [&](const tx_out& o)
                               {
                                   const txout_to_key& tx_in_to_key
                                           = boost::get<txout_to_key>(o.target);

                                   ++idx;

                                   return tx_in_to_key.key == output_pubkey;
                               });

        if (it != tx.vout.end())
        {
            // we found the desired public key
            out = *it;
            output_index = idx > 0 ? idx - 1 : idx;

            //cout << idx << " " << output_index << endl;

            return true;
        }

        return false;
    }


    bool
    MicroCore::find_tx_with_key_image(const crypto::key_image& key_img,
                                      crypto::hash& tx_hash,
                                      bool show_progress)
    {
        bool tx_found {false};

        uint64_t tx_idx {0};

        uint64_t total_tx_count = m_blockchain_storage.get_db().get_tx_count();

        m_blockchain_storage.for_all_transactions(
                [&](const crypto::hash& hash, const cryptonote::transaction& tx)->bool
                    {



                        if (show_progress)
                        {
                            if (tx_idx % 100)
                            {
                                cout  << "\r" << " - checking tx no: "
                                      << tx_idx << "/" << total_tx_count << flush;
                            }
                        }


                        // go over all key images in a given transaction
                        // to look for the one with matching key image
                        auto it = std::find_if(tx.vin.begin(), tx.vin.end(),
                                               [&](const txin_v& tx_input)
                                               {
                                                   if (tx_input.type() == typeid(txin_to_key))
                                                   {
                                                       const txin_to_key& tx_in_to_key
                                                               = boost::get<txin_to_key>(tx_input);

                                                       return tx_in_to_key.k_image == key_img;
                                                   }

                                                   return false;
                                               });

                        // if we found our key_image
                        if (it != tx.vin.end())
                        {
                            if (show_progress)
                            {
                                cout  << "\r" << " - tx found :-): "
                                      << tx_idx << "/" << total_tx_count << flush;
                            }

                            tx_hash = get_transaction_hash(tx);
                            tx_found = true; // mark that we found the transation

                            return false; // we found, so stop the iteration over transactions
                        }

                        ++tx_idx;

                        return true; // continue the search the iteration
                    });


        return tx_found;
    }



    unordered_map<crypto::key_image, crypto::hash>
    MicroCore::find_txs_with_key_images(const vector<crypto::key_image>& key_imgs,
                                        bool show_progress)
    {

        unordered_map<crypto::key_image, crypto::hash> tx_hashes_found;

        size_t no_of_keys_to_find = key_imgs.size();

        uint64_t tx_idx {0};

        uint64_t total_tx_count = m_blockchain_storage.get_db().get_tx_count();


        m_blockchain_storage.for_all_transactions(
                [&](const crypto::hash& hash, const cryptonote::transaction& tx)->bool {

                    if (show_progress)
                    {
                        if (tx_idx % 100)
                        {
                            cout << "\r" << "\t - checking tx no: "
                            << tx_idx << "/" << total_tx_count << flush;
                        }
                    }

                    for (const crypto::key_image& key_img: key_imgs)
                    {

                        // go over all key images in a given transaction
                        // to look for the one with matching key image
                        auto it = std::find_if(tx.vin.begin(), tx.vin.end(),
                                               [&](const txin_v &tx_input) {

                                                   if (tx_input.type() == typeid(txin_to_key)) {

                                                       const txin_to_key &tx_in_to_key
                                                               = boost::get<txin_to_key>(tx_input);

                                                       return tx_in_to_key.k_image == key_img;
                                                   }

                                                   return false;
                                               });

                        // if we found our key_image
                        if (it != tx.vin.end())
                        {

                            if (show_progress)
                            {
                                cout << "\n" << "\t - tx found for key_img: " << key_img;
                            }

                            // save the found tx into the output map
                            tx_hashes_found[key_img] = get_transaction_hash(tx);

                            if (--no_of_keys_to_find == 0)
                            {

                                if (show_progress)
                                {
                                    cout << "\n\t" << "- all keys found! :-)" << endl;
                                }

                                return false; // we found everything
                            }
                        }
                    }

                    ++tx_idx;

                    return true; // continue the search the iteration
                });


        return tx_hashes_found;
    }




    /**
     * Returns tx hash in a given block which
     * contains given output's public key
     */
    bool
    MicroCore::get_tx_hash_from_output_pubkey(const public_key& output_pubkey,
                                              const uint64_t& block_height,
                                              crypto::hash& tx_hash,
                                              cryptonote::transaction& tx_found)
    {

        tx_hash = null_hash;

        // get block of given height
        block blk;
        if (!get_block_by_height(block_height, blk))
        {
            cerr << "Cant get block of height: " << block_height << endl;
            return false;
        }


        // get all transactions in the block found
        // initialize the first list with transaction for solving
        // the block i.e. coinbase.
        list<transaction> txs {blk.miner_tx};
        list<crypto::hash> missed_txs;

        if (!m_blockchain_storage.get_transactions(blk.tx_hashes, txs, missed_txs))
        {
            cerr << "Cant find transcations in block: " << block_height << endl;
            return false;
        }

        if (!missed_txs.empty())
        {
            cerr << "Transactions not found in blk: " << block_height << endl;

            for (const crypto::hash& h : missed_txs)
            {
                cerr << " - tx hash: " << h << endl;
            }

            return false;
        }


        // search outputs in each transactions
        // until output with pubkey of interest is found
        for (const transaction& tx : txs)
        {

            tx_out found_out;

            // we dont need here output_index
            size_t output_index;

            if (find_output_in_tx(tx, output_pubkey, found_out, output_index))
            {
                // we found the desired public key
                tx_hash = get_transaction_hash(tx);
                tx_found = tx;

                return true;
            }

        }

        return false;
    }




    void
    MicroCore::check_ring_signature(const crypto::hash &tx_prefix_hash,
                                          const crypto::key_image &key_image,
                                          const std::vector<crypto::public_key> &pubkeys,
                                          const std::vector<crypto::signature>& sig,
                                          uint64_t &result)
    {
        std::vector<const crypto::public_key *> p_output_keys;

        for (auto &key : pubkeys)
        {
            p_output_keys.push_back(&key);
        }

        result = crypto::check_ring_signature(tx_prefix_hash,
                                              key_image,
                                              p_output_keys,
                                              sig.data()) ? 1 : 0;
    }





    /**
     * De-initialized Blockchain.
     *
     * since blockchain is opened as MDB_RDONLY
     * need to manually free memory taken on heap
     * by BlockchainLMDB
     */
    MicroCore::~MicroCore()
    {
       delete &m_blockchain_storage.get_db();
    }
}