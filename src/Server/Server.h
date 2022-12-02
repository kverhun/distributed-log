#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <vector>

#include <mongoose.h>

namespace network {

/**
 * @brief Server class provides a simple abstraction for a web-server
 * Current implementation is based on Mongoose server
 */
class Server
{
public:
    Server(size_t port);
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;
    ~Server();

    /**
     * @brief HTTP request handler
     * input parameter: body of request
     * return value: response to be sent
     */
    using RequestHandler = std::function<std::string(const std::string&)>;

    /**
     * @brief setup handler which will be executed on GET request to a server
     */
    void SetRequestHandlerGet(RequestHandler handler);

    /**
     * @brief setup handler which will be executed on POST request to a server
     */
    void SetRequestHandlerPost(RequestHandler handler);

    /**
     * @brief starts listening the port specified in constructor.
     * Even loop breaks when `stopped` is set to `true` externally
     * @param stopped
     */
    void Listen(const std::atomic_bool& stopped);

private:
    friend void mongoose_handler(mg_connection* c, int ev, void* ev_data, void* fn_data);

    void OnGetRequest(std::string request, mg_connection* connection);
    void OnPostRequest(std::string request_body, mg_connection* connection);

private:
    size_t m_port;
    Server::RequestHandler m_handler_get;
    Server::RequestHandler m_handler_post;

    struct Response {
        std::string message;
        mg_connection* c;
    };
    mutable std::mutex m_pending_requests_mutex;
    std::vector<std::future<Response>> m_pending_requests;

    mg_mgr m_mongoose_manager;
};

}  // namespace network
