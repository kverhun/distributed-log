#include "RpcUtils.h"

#include <atomic>
#include <array>
#include <iostream>

namespace network {

// borrowed from: https://stackoverflow.com/a/478960/2466572
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string PostHttpAndWaitReply(const std::string &url, const std::string &message) {
    std::string cmd = "curl -d \"";
    cmd += message;
    cmd += "\" -X POST \"";
    cmd += url;
    cmd += "\"";

    std::cout << "Executing curl cmd \"" << cmd << "\"..." << "\n";
    const auto reply = exec(cmd.c_str());
    std::cout << "Reply: \"" << reply << "\"\n";

    return reply;
}

}
