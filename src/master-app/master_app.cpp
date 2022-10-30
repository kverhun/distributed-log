#include <iostream>
#include <string>
#include <vector>

#include <Server.h>

std::vector<std::string> InMemoryMessageStorage;

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

int main() {
    std::atomic_bool stopped = false;
    network::Server server{8000};

    server.SetRequestHandlerPost([](const std::string& str){
        std::cout << "Post message: " << str << "\n";

        InMemoryMessageStorage.push_back(str);

        return std::string("Message \"" + str + "\" accepted\n");
    });
    server.SetRequestHandlerGet([](const std::string& str){
        std::cout << "Get message: " << str << "\n";
        return MessageListResponseStr(InMemoryMessageStorage);
    });

    server.Listen(stopped);
    return 0;
}
