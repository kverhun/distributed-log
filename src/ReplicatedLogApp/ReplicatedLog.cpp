#include <RpcUtils.h>
#include <Server.h>

#include <chrono>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

int RandomTimeoutMs()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(5000, 10000);
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
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        std::cout << "Timer " << m_title << ": " << ms << "ms.\n";
    }

private:
    std::string m_title;
    std::chrono::steady_clock::time_point m_start_time;
};

struct Message {
    std::string message;
    size_t id;
};

/**
 * In-memory message storage
 */
std::vector<Message> InMemoryMessageStorage;

/**
 * Returns list of messages as a single string. To be used to form http requests reply.
 */
std::string MessageListResponseStr(const std::vector<Message>& messages)
{
    std::string res = "[";
    if (!messages.empty()) {
        res += messages.front().message;
    }
    for (auto i = 1u; i < messages.size(); ++i) {
        res += ", ";
        res += messages[i].message;
    }
    res += "]\n";
    return res;
}

/**
 * Compute message hash. For logging and acknowledgement.
 */
std::string MessageHash(const std::string& message) {
    return std::to_string(std::hash<std::string>()(message));
}

/**
 * Construct acknowledgement response string
 */
std::string ConstructAck(const std::string& message) {
    return std::string("Message received: " + MessageHash(message));
}

/**
 * Post message content
 */
struct PostContent {
    size_t write_concern{1u};
    std::string message;
};

/**
 * Parse string in format "[number]message" into <number, message>
 */
std::pair<size_t, std::string> ParseMessage(const std::string& post_message) {
    if (post_message.empty()) {
        return std::make_pair(0u, "");
    }

    // no write concern specified
    if (post_message[0] != '[') {
        return std::make_pair(0u, post_message);
    }

    const auto close_bracket_idx = post_message.find_first_of(']');
    if (close_bracket_idx != std::string::npos) {
        std::string content_between_brackets;
        content_between_brackets.assign(post_message.begin() + 1, post_message.begin() + close_bracket_idx);
        try {
            const size_t number_in_brackets = std::atoi(content_between_brackets.c_str());
            std::string message;
            message.assign(post_message.begin() + close_bracket_idx + 1, post_message.end());
            return std::make_pair(
                number_in_brackets,
                message
            );
        }
        catch (...) {
            std::cout << "Fail to parse post message: " << post_message << "\n";
        }
    }

    // parsing write concern unsuccessful - pass message as is
    return std::make_pair(
        1u,
        post_message
    );
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

    auto perform_replication = [](const std::string& url, const Message& message) -> bool {
        std::string message_with_id = "[" + std::to_string(message.id) + "]" + message.message;
        std::cout << "Sending to secondary: " << message_with_id << "\n";
        const auto response = network::PostHttpAndWaitReply(url, message_with_id);
        // TODO: synchronize log by mutex
        std::cout << "Response from secondary (" << url << "): " << response << "\n";

        const std::string success_response_content = ConstructAck(message.message);
        if (response.find(success_response_content) == std::string::npos) {
            // ACK not found
            return false;
        }

        return true;
    };

    size_t current_message_id = 0;

    // store pending replications
    std::vector<std::future<bool>> pending_replications;

    // POST request handler: receive message
    server.SetRequestHandlerPost([&](const std::string& str) {
        Timer timer{"Post request handling"};
        std::cout << "Post message: " << str << "\n";

        const auto parse_result = ParseMessage(str);

        Message current_message = [&]() {
            if (!secondary_nodes_urls.empty()) {
                return Message{
                    parse_result.second,
                    current_message_id++
                };
            }
            else {
                return Message{
                    parse_result.second,
                    parse_result.first
                };
            }
        }();

        std::cout << "Message: " << current_message.message << " id: " << current_message.id << "\n";

        std::atomic_size_t replication_counter = 0;

        // for testing purposes - random timeout
        if (secondary_nodes_urls.empty()) {
            const auto timeout_ms = RandomTimeoutMs();
            std::cout << "Delay for " << timeout_ms << "ms\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        }

        // store message in current node
        InMemoryMessageStorage.push_back(current_message);
        std::sort(InMemoryMessageStorage.begin(), InMemoryMessageStorage.end(), [](const Message& l, const Message& r) {
            return l.id < r.id;
        });
        ++replication_counter;

        // perform replication if secondary nodes registered
        if (!secondary_nodes_urls.empty()) {
            PostContent post_message_data{parse_result.first, current_message.message};

            const size_t write_concern = [&]() -> size_t {
                if (post_message_data.write_concern < 1) {
                    std::cerr << "Write concern < 1 does not make sense. Setting write concern to 1" << "\n";
                    return 1u;
                }
                if (post_message_data.write_concern > secondary_nodes_urls.size() + 1) {
                    std::cerr << "Write concern is bigger than number of secondary nodes. Setting write concern to "
                              << secondary_nodes_urls.size() << "\n";
                    return secondary_nodes_urls.size() + 1;
                }
                return post_message_data.write_concern;
            }();

            std::cout << "Message: " << post_message_data.message << " (" << MessageHash(post_message_data.message)
                      << "): w = " << write_concern << "\n";

            std::vector<std::future<bool>> replication_results;
            for (const auto& secondary_url : secondary_nodes_urls) {
                replication_results.push_back(std::async([secondary_url, current_message, perform_replication,
                                                          &replication_counter]() {
                    const bool replication_result = perform_replication(secondary_url, current_message);
                    if (replication_result) {
                        ++replication_counter;
                    }
                    return replication_result;
                }));
            }

            std::future<void> replication_future = std::async([&replication_counter, write_concern](){
                size_t log_counter = 0;
                while (replication_counter < write_concern) {
                    if (++log_counter % 10 == 0) {
                        std::cout << "Waiting for replication: " << replication_counter << " of " << write_concern
                                  << "\n";
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds{50});
                }
            });
            replication_future.get();

            // move pending replications to "backlog" to be completed later
            pending_replications.insert(
                pending_replications.end(), std::make_move_iterator(replication_results.begin()),
                std::make_move_iterator(replication_results.end())
            );

            std::cout << "Replication with write concern " << write_concern << " finished" << "\n";
        }

        return ConstructAck(current_message.message);
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
