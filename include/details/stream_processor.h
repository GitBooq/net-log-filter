/**
 * @file stream_processor.h
 * @brief Stream Processor
 */
#pragma once

#include <algorithm>  // for copy
#include <cstddef>    // for size_t
#include <istream>
#include <iterator>  // for ostream_iterator
#include <optional>
#include <ranges>  // for transform_view
#include <string>
#include <string_view>
#include <vector>
#include <span>

#include "filter.h"  // for FilterType
#include "ip_v4_address.h"
#include "net_logger/types.h"

namespace net::details {

/**
 * @brief Parse input and output result after filtering.
 * Batch filtering.
 */
class StreamProcessor {
 public:
  /**
   * @brief Construct a new Stream Processor object.
   * Reserves batch_size bytes in output buffer.
   *
   * @param batch_size
   */
  explicit StreamProcessor(size_t batch_size = DEFAULT_BATCH_SIZE)
      : batch_size_(batch_size) {
    buffer_.reserve(batch_size);
  }

  template <FilterType T, typename Callback>
  void Process(std::istream& input, const T& filter, Callback&& callback);

 private:
  struct ParseResult {
    std::optional<IPv4Address> ip;
    logger::RejectReason reason;
  };

  /**
   * @brief Parse IPAddr from string "IP - message"
   *
   * @param line string to parse
   * @return ParseResult
   */
  ParseResult ParseIPFromLine(const std::string& line) const;

  /**
   * @brief Apply callback to buffer and clear
   *
   */
  template <typename Callback>
  void FlushBuffer(Callback&& callback);

  static constexpr size_t DEFAULT_BATCH_SIZE = 1000U;

  std::vector<logger::LogEntry> buffer_;  ///< buffer for output
  size_t batch_size_;                     ///< max buffer size
};

/////////////////////////////////////////////
//
// Implementation
//
/////////////////////////////////////////////

template <FilterType T, typename Callback>
inline void StreamProcessor::Process(std::istream& input, const T& filter,
                                     Callback&& callback) {
  std::string line;
  size_t line_number = 0;

  while (std::getline(input, line)) {
    ++line_number;

    logger::LogEntry entry;
    entry.raw_line = line;
    entry.line_number = line_number;

    auto parse_result = ParseIPFromLine(line);

    entry.parsed_ip = std::move(parse_result.ip);
    entry.reason = parse_result.reason;

    if (entry.reason == logger::RejectReason::None) {
      if (!filter.Matches(*entry.parsed_ip)) {
        entry.reason = logger::RejectReason::FilterRejected;
      }
    }

    buffer_.push_back(std::move(entry));

    if (buffer_.size() >= batch_size_) {
      FlushBuffer(callback);
    }
  }

  if (!buffer_.empty()) {
    FlushBuffer(callback);
  }
}

inline auto StreamProcessor::ParseIPFromLine(const std::string& line) const
    -> ParseResult {
  using logger::RejectReason;
  size_t end = line.find(' ');
  if (end == std::string::npos) {
    return {std::nullopt, RejectReason::InvalidFormat};
  }

  std::string_view ip_str(line.data(), end);
  auto parsed = IPv4Address::FromString(ip_str);

  if (!parsed) {
    return {std::nullopt, RejectReason::InvalidIPAddress};
  }

  return {std::move(parsed), RejectReason::None};
}

template <typename Callback>
inline void StreamProcessor::FlushBuffer(Callback&& callback) {
  callback(std::span<const logger::LogEntry>(buffer_));

  buffer_.clear();
}
}  // namespace net::details
