#include <iostream>
#include <vector>

#include <mongoose.h>

std::vector<std::string> messages;

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

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    //std::cout << "ev: " << ev << "\n";
    if (ev == MG_EV_HTTP_MSG) {
        auto& message = *static_cast<mg_http_message*>(ev_data);

        const auto request_type = GetRequestType(message);
        std::cout << "Request type: " << ToString(request_type) << "\n";

        switch (request_type) {
            case RequestType::Get: {
                std::cout << "Messages:\n";
                for (const auto& msg : messages) {
                    std::cout << msg << "\n";
                }

                std::string response_list = "[";
                if (!messages.empty()) {
                    response_list += messages.front();
                    for (auto i = 1u; i < messages.size(); ++i) {
                        response_list += ", ";
                        response_list += messages[i];
                    }
                }
                response_list += "]";
                response_list += "\n"; // append new line, so there is no "partial line"
                mg_http_reply(c, 200, "Content-Type: text/plain\r\n", response_list.c_str());
                break;
            }
            case RequestType::Post: {
                std::cout << "BODY\n" << message.body.ptr << "\nEND BODY\n";
                messages.push_back(std::string{message.body.ptr});

                mg_http_reply(c, 200, "Content-Type: text/plain\r\n", "Message \"%s\" logged\n", messages.back().c_str());
                break;
            }
            default: {
                std::cout << "Unknown request type\n";
                mg_http_reply(c, 400, "Content-Type: text/plain\r\n", "Unknown request type\n");
                break;
            }
        }


    }
}

int main(int argc, char *argv[]) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);                                        // Init manager
    mg_http_listen(&mgr, "http://localhost:8000", fn, &mgr);  // Setup listener
    for (;;) mg_mgr_poll(&mgr, 1000);                         // Event loop
    mg_mgr_free(&mgr);                                        // Cleanup
    return 0;
}
