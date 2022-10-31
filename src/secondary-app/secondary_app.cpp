#include <iostream>

#include <string>
#include <vector>
#include <thread>

#include <Server.h>

constexpr size_t DefaultPort{8005};

std::vector<std::string> InMemoryMessageStorage;

// TODO: get rid of code duplication
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

int main(int argc, char** argv) {
    const size_t port = [&]() {
        if (argc <= 1) {
            return DefaultPort;
        }
        return size_t(std::atoi(argv[1]));
    }();

    network::Server server{port};

    server.SetRequestHandlerPost([](const std::string& str){
        std::this_thread::sleep_for(std::chrono::seconds{2});
        std::cout << "Secondary: message " << str << " received from POST request\n";
        InMemoryMessageStorage.push_back(str);
        return std::string("Message \"" + str + "\" accepted\n");
    });
    server.SetRequestHandlerGet([](const std::string& msg){
        std::cout << "Secondary: GET request received. Msg: " << msg << "\n";
        return MessageListResponseStr(InMemoryMessageStorage);
    });

    std::atomic_bool stopped = false;
    server.Listen(stopped);

    return 0;
}
