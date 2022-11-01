#include <RpcUtils.h>
#include <Server.h>

#include <chrono>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <vector>

int RandomTimeoutMs()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(100, 3000);
    return distr(gen);
}

/**
 * @brief Helper class to measure time
 */
class Timer
{
public:
    Timer(const std::string& title) : m_title(title), m_start_time(std::chrono::steady_clock::now()) {}

    ~Timer()
    {
        const auto duration = std::chrono::steady_clock::now() - m_start_time;
        const auto ms = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        std::cout << "Timer " << m_title << ": " << ms << "ms.\n";
    }

private:
    std::string m_title;
    std::chrono::steady_clock::time_point m_start_time;
};

/**
 * In-memory message storage
 */
std::vector<std::string> InMemoryMessageStorage;

/**
 * Returns list of messages as a single string. To be used to form http requests reply.
 */
std::string MessageListResponseStr(const std::vector<std::string>& messages)
{
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
struct AppConfig
{
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
AppConfig ParseCmdArgs(int argc, char** argv)
{
    AppConfig config;
    if (argc <= 1) {
        return AppConfig{8000, {}};
    }
    if (argc > 1) {
        config.this_app_port = std::atoi(argv[1]);
    }
    for (int i = 2; i < argc; ++i) {
        config.secondary_apps_ports.push_back(std::atoi(argv[i]));
    }
    return config;
}

std::atomic_bool interrupted = false;
void SignalHandler(int) {
    interrupted = true;
}

int main(int argc, char** argv)
{
    // TODO: weird port if no arguments specified

    AppConfig app_config = ParseCmdArgs(argc, argv);

    // initialize app server
    network::Server server{app_config.this_app_port};

    // form URLs for "Secondary" nodes
    std::vector<std::string> secondary_nodes_urls;
    for (const auto& port : app_config.secondary_apps_ports) {
        secondary_nodes_urls.push_back("http://localhost:" + std::to_string(port));
    }

    auto perform_replication = [](const std::string& url, const std::string& message) -> bool {
        const auto response = network::PostHttpAndWaitReply(url, message);
        // TODO: synchronize log by mutex
        std::cout << "Response from secondary (" << url << "): " << response;
        return true;  // for now - assume no failures
    };

    // POST request handler: receive message
    server.SetRequestHandlerPost([&](const std::string& str) {
        Timer timer{"Post request handling"};
        std::cout << "Post message: " << str << "\n";

        // perform replication if secondary nodes registered
        if (!secondary_nodes_urls.empty()) {
            std::vector<std::future<bool>> replication_results;
            for (const auto& secondary_url : secondary_nodes_urls) {
                replication_results.push_back(std::async([&]() { return perform_replication(secondary_url, str); }));
            }

            bool success = true;
            for (auto& f : replication_results) {
                if (!f.get()) {
                    // assume doesn't happen for v1
                    std::cerr << "Replication failed\n";
                    success = false;
                }
            }

            if (!success) {
                return std::string("Message \"" + str + "\" declined");
            }
        }

        // for testing purposes - random timeout
        if (secondary_nodes_urls.empty()) {
            const auto timeout_ms = RandomTimeoutMs();
            std::cout << "Delay for " << timeout_ms << "ms\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        }

        // add to in-memory storage only after replication performed
        InMemoryMessageStorage.push_back(str);
        return std::string("Message \"" + str + "\" accepted\n");
    });

    // GET request handler: return list of messages from in-memory storage
    server.SetRequestHandlerGet([](const std::string& str) {
        std::cout << "Get message: " << str << "\n";
        return MessageListResponseStr(InMemoryMessageStorage);
    });

    // run server

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGABRT, SignalHandler);
    server.Listen(interrupted);

    return 0;
}
