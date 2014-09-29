#ifndef OBELISK_WORKER_NODE_IMPL_HPP
#define OBELISK_WORKER_NODE_IMPL_HPP

#include <bitcoin/bitcoin.hpp>
#include <obelisk/define.hpp>
#include "config.hpp"

namespace obelisk {

class node_impl
{
public:
    typedef std::function<void (size_t, const bc::block_type&)>
        block_notify_callback;
    typedef std::function<void (const bc::transaction_type&)>
        transaction_notify_callback;

    BCS_API node_impl();
    BCS_API bool start(config_type& config);
    BCS_API bool stop();

    BCS_API void subscribe_blocks(block_notify_callback notify_block);
    BCS_API void subscribe_transactions(transaction_notify_callback notify_tx);

    // Access to underlying services.
    BCS_API bc::blockchain& blockchain();
    BCS_API bc::transaction_pool& transaction_pool();
    BCS_API bc::transaction_indexer& transaction_indexer();
    BCS_API bc::network::protocol& protocol();

    // Threadpool for memory related operations.
    BCS_API bc::threadpool& memory_related_threadpool();

private:
    typedef std::vector<block_notify_callback> block_notify_list;
    typedef std::vector<transaction_notify_callback> transaction_notify_list;

    void start_session();
    void wait_and_retry_start(const std::error_code& ec);

    void monitor_tx(const std::error_code& ec, bc::network::channel_ptr node);
    void recv_transaction(const std::error_code& ec,
        const bc::transaction_type& tx, bc::network::channel_ptr node);
    void handle_mempool_store(
        const std::error_code& ec, const bc::index_list& unconfirmed,
        const bc::transaction_type& tx, bc::network::channel_ptr node);

    void reorganize(const std::error_code& ec,
        size_t fork_point,
        const bc::blockchain::block_list& new_blocks,
        const bc::blockchain::block_list& replaced_blocks);

    std::ofstream outfile_, errfile_;
    bc::threadpool network_pool_, disk_pool_, mem_pool_;
    // Services
    bc::network::hosts hosts_;
    bc::network::handshake handshake_;
    bc::network::network network_;
    bc::network::protocol protocol_;
//    bc::leveldb_blockchain chain_;
    bc::poller poller_;
    bc::transaction_pool txpool_;
    bc::transaction_indexer indexer_;
    bc::session session_;

    block_notify_list notify_blocks_;
    transaction_notify_list notify_txs_;

    boost::asio::deadline_timer retry_start_timer_;
};

} // namespace obelisk

#endif

