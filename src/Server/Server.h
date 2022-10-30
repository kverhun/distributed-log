#pragma once

#include <atomic>
#include <cstddef>
#include <functional>

#include <mongoose.h>

namespace network {

/**
 * @brief Server class provides a simple abstraction for a web-server
 * Current implementation is based on Mongoose server
 */
class Server {
public:
    Server(size_t port);
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = default;
    Server& operator=(Server&&) = default;
    ~Server();

    // input parameter: BODY of request
    // return value: response
    using RequestHandler = std::function<std::string(const std::string&)>;

    void SetRequestHandlerGet(RequestHandler handler);
    void SetRequestHandlerPost(RequestHandler handler);

    std::string OnGetRequest(const std::string&);
    std::string OnPostRequest(const std::string& request_body);

    void Listen(const std::atomic_bool& stopped);

private:
    size_t m_port;
    Server::RequestHandler m_handler_get;
    Server::RequestHandler m_handler_post;

    // mongoose infrastructure
    mg_mgr m_mongoose_manager;
};

}
