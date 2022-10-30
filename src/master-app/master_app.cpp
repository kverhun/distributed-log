#include <iostream>

#include <Server.h>

int main() {
    std::atomic_bool stopped = false;
    network::Server server{8000};

    server.SetRequestHandlerPost([](const std::string& str){
        std::cout << "Post message: " << str << "\n";
        return "POST accepted";
    });
    server.SetRequestHandlerGet([](const std::string& str){
        std::cout << "Get message: " << str << "\n";
        return "GET accepted";
    });

    server.Listen(stopped);
    return 0;
}
