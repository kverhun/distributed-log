#include <iostream>
#include <string>
#include <vector>

#include <RpcUtils.h>
#include <Server.h>

/**
 * In-memory message storage
 */
std::vector<std::string> InMemoryMessageStorage;

/**
 * Returns list of messages as a single string. To be used to form http requests reply.
 */
std::string MessageListResponseStr(const std::vector<std::string>& messages) {
    std::string res = "[";
    if (!messages.empty()) {
        res += messages.front();
    }
    for (auto i = 1u; i < messages.size(); ++i) {
        res += ", ";
        res += messages[i];
    }
    res += "]\n";
    return res;
}

/**
 * Specifies configuration of replication system
 */
struct AppConfig {
    /**
     * App will use this port for server
     */
    size_t this_app_port;

    /**
     * Ports of Secondary nodes to perform replication on.
     * If empty - no replication needed ("Secondary" app mode).
     */
    std::vector<size_t> secondary_apps_ports;
};

/**
 * @brief parse command line arguments and return `AppConfig`
 */
AppConfig ParseCmdArgs(int argc, char** argv) {
    AppConfig config;
    if (argc > 1) {
        config.this_app_port = std::atoi(argv[1]);
    }
    for (int i = 2; i < argc; ++i) {
        config.secondary_apps_ports.push_back(std::atoi(argv[i]));
    }
    return config;
}

int main(int argc, char** argv) {
    std::atomic_bool stopped = false;

    AppConfig app_config = ParseCmdArgs(argc, argv);

    // initialize app server
    network::Server server{app_config.this_app_port};

    // form URLs for "Secondary" nodes
    std::vector<std::string> secondary_nodes_urls;
    for (const auto& port : app_config.secondary_apps_ports) {
        secondary_nodes_urls.push_back("http://localhost:" + std::to_string(port));
    }

    // POST request handler: receive message
    server.SetRequestHandlerPost([&](const std::string& str){
        std::cout << "Post message: " << str << "\n";

        // perform blocking replication to secondary nodes
        for (const auto& secondary_url : secondary_nodes_urls) {
            const auto response = network::PostHttpAndWaitReply(secondary_url, str);
            std::cout << "Response from secondary (" << secondary_url << "): " << response;
            // add proper response check. for now assume response = request successful
        }

        // add to in-memory storage only after replication performed
        InMemoryMessageStorage.push_back(str);

        return std::string("Message \"" + str + "\" accepted\n");
    });

    // GET request handler: return list of messages from in-memory storage
    server.SetRequestHandlerGet([](const std::string& str){
        std::cout << "Get message: " << str << "\n";
        return MessageListResponseStr(InMemoryMessageStorage);
    });

    // run server
    server.Listen(stopped);

    return 0;
}
