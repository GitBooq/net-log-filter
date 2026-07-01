/**
 * @file types.h
 */
#pragma once

#include <optional>
#include <string>

#include "details/ip_v4_address.h"

namespace net::logger {

enum class RejectReason : uint8_t {
  None,
  InvalidFormat,
  InvalidIPAddress,
  FilterRejected
};

struct LogEntry {
  std::string raw_line;
  std::optional<details::IPv4Address> parsed_ip;
  RejectReason reason = RejectReason::None;
  std::size_t line_number = 0;
};

}  // namespace net::logger