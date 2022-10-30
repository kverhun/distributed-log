#include "Server.h"

#include <iostream>
#include <string>

#include <mongoose.h>

namespace network {

enum class RequestType {
    Unknown,
    Get,
    Post,
};

std::string ToString(RequestType rt) {
    switch (rt) {
        case RequestType::Unknown:
            return "Unknown";
        case RequestType::Get:
            return "GET";
        case RequestType::Post:
            return "POST";
        default:
            return std::string("Unknown request type: ") + std::to_string(int(rt));
    }
}

RequestType GetRequestType(const mg_http_message& message) {
    const char* post_str{"POST"};
    const char* get_str{"GET"};

    if (strlen(message.method.ptr) >= strlen(get_str)
        && !strncmp(message.method.ptr, get_str, strlen(get_str))) {
        return RequestType::Get;
    }

    if (strlen(message.method.ptr) >= strlen(post_str)
        && !strncmp(message.method.ptr, post_str, strlen(post_str))) {
        return RequestType::Post;
    }

    return RequestType::Unknown;
}

static void mongoose_handler(mg_connection *c, int ev, void *ev_data, void *fn_data);


void mongoose_handler(mg_connection *c, int ev, void *ev_data, void *fn_data) {
    Server* server = static_cast<Server*>(fn_data);

    if (ev == MG_EV_HTTP_MSG) {
        auto& message = *static_cast<mg_http_message*>(ev_data);

        const auto request_type = GetRequestType(message);
        std::cout << "Request type: " << ToString(request_type) << "\n";

        std::string response;

        switch (request_type) {
            case RequestType::Get:
                response = server->OnGetRequest(message.body.ptr);
                break;
            case RequestType::Post:
                response = server->OnPostRequest(message.body.ptr);
                break;
            default:
                response = "Unknown request type";
                break;
        }

        mg_http_reply(c, 200, "Content-Type: text/plain\r\n", response.c_str());
    }
}


Server::Server(size_t port)
        : m_port(port) {
    mg_mgr_init(&m_mongoose_manager);
    mg_http_listen(&m_mongoose_manager, "http://localhost:8000", mongoose_handler, this);
}

Server::~Server() = default;

void Server::SetRequestHandlerGet(Server::RequestHandler handler) {
    m_handler_get = handler;
}

void Server::SetRequestHandlerPost(Server::RequestHandler handler) {
    m_handler_post = handler;
}

std::string Server::OnGetRequest(const std::string& request_body) {
    if (!m_handler_get) {
        throw std::logic_error("No GET handler set");
    }
    return m_handler_get(request_body);
}

std::string Server::OnPostRequest(const std::string &request_body) {
    if (!m_handler_post) {
        throw std::logic_error("No POST handler set");
    }
    return m_handler_post(request_body);
}

void Server::Listen(const std::atomic_bool& stopped) {
    while (true) {
        if (stopped) {
            break;
        }
        mg_mgr_poll(&m_mongoose_manager, 1000);
    }
}

}
