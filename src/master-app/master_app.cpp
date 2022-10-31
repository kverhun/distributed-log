#include <iostream>
#include <string>
#include <vector>

#include <RpcUtils.h>
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

    const size_t MasterPort{8000};
    const size_t Secondary1Port{8005};
    const size_t Secondary2Port{8006};

    network::Server server{MasterPort}; // TODO: configure ports
    const std::string secondary1_url = "http://localhost:" + std::to_string(Secondary1Port);
    const std::string secondary2_url = "http://localhost:" + std::to_string(Secondary2Port);

    server.SetRequestHandlerPost([&](const std::string& str){
        std::cout << "Post message: " << str << "\n";

        const auto response1 = network::PostHttpAndWaitReply(secondary1_url, str);
        std::cout << "Response from secondary1: " << response1;
        const auto response2 = network::PostHttpAndWaitReply(secondary2_url, str);
        std::cout << "Response from secondary2: " << response2;

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
