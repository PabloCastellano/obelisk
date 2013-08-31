#include <boost/filesystem.hpp>
#include <obelisk/message.hpp>
#include "echo.hpp"
#include "worker.hpp"
#include "node_impl.hpp"
#include "publisher.hpp"
#include "subscribe_manager.hpp"
#include "service/fullnode.hpp"
#include "service/blockchain.hpp"
#include "service/protocol.hpp"
#include "service/transaction_pool.hpp"

using namespace bc;
using std::placeholders::_1;
using std::placeholders::_2;

int main(int argc, char** argv)
{
    config_map_type config;
    if (argc == 2)
    {
        echo() << "Using config file: " << argv[1];
        load_config(config, argv[1]);
    }
    else
    {
        using boost::filesystem::path;
        path conf_filename = path(SYSCONFDIR) / "worker.cfg";
        load_config(config, conf_filename.native());
    }
    // Create worker.
    request_worker worker;
    worker.start(config["service"]);
    // Fullnode
    node_impl node;
#ifdef OB_PUBLISHER
    // Publisher
    publisher publish(node);
    if (!publish.start(config))
        return 1;
#endif
    // Address subscriptions
    subscribe_manager addr_sub(node);
    // Attach commands
    typedef std::function<void (node_impl&,
        const incoming_message&, zmq_socket_ptr)> basic_command_handler;
    auto attach = [&worker, &node](
        const std::string& command, basic_command_handler handler)
        {
            worker.attach(command,
                std::bind(handler, std::ref(node), _1, _2));
        };
    worker.attach("address.subscribe",
        std::bind(&subscribe_manager::subscribe, &addr_sub, _1, _2));
    attach("address.fetch_history", fullnode_fetch_history);
    attach("blockchain.fetch_history", blockchain_fetch_history);
    attach("blockchain.fetch_transaction", blockchain_fetch_transaction);
    attach("blockchain.fetch_last_height", blockchain_fetch_last_height);
    attach("blockchain.fetch_block_header", blockchain_fetch_block_header);
    attach("protocol.broadcast_transaction", protocol_broadcast_transaction);
    attach("transaction_pool.validate", transaction_pool_validate);
    // Start the node last so that all subscriptions to new blocks
    // don't miss anything.
    if (!node.start(config))
        return 1;
    echo() << "Node started.";
    bool stopped = false;
    std::thread thr([&stopped]()
        {
            while (true)
            {
                std::string user_cmd;
                std::getline(std::cin, user_cmd);
                if (user_cmd == "stop")
                    break;
            }
            stopped = true;
        });
    while (!stopped)
        worker.update();
    thr.join();
#ifdef OB_PUBLISHER
    publish.stop();
#endif
    if (!node.stop())
        return -1;
    return 0;
}
