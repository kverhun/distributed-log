#pragma once

#include <string>

namespace network {

/**
 * @brief Send POST http request to `url` with `message` as data
 * For now, no timeouts and error handling: assume perfect connection.
 * @return http request reply
 */
std::string PostHttpAndWaitReply(const std::string& url, const std::string& message);

}  // namespace network
