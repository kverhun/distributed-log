#pragma once

#include <string>

namespace network {

std::string PostHttpAndWaitReply(const std::string& url, const std::string& message);

}
