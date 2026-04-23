/**
 * @file stream_processor.h
 * @brief Stream Processor
 */
#pragma once

#include <algorithm>   // for copy
#include <cstddef>     // for size_t
#include <istream>     // for ostream, istream
#include <iterator>    // for ostream_iterator
#include <optional>    // for optional
#include <ranges>      // for transform_view
#include <string>      // for string
#include <string_view> // for string_view
#include <vector>      // for vector

#include "filter.h"        // for FilterType
#include "ip_v4_address.h" // for IPv4Address

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

  /**
   * @brief Parse, filter, output.
   * Output when buffer is full, or input ends
   * @tparam T filter type
   * @param input input stream (logs)
   * @param output output stream (result)
   * @param filter filter to apply to input stream
   */
  template <FilterType T>
  void Process(std::istream &input, std::ostream &output, const T &filter);

private:
  /**
   * @brief Input file line structure
   *
   */
  struct LogEntry {
    IPv4Address ip;
    std::string line;
    size_t line_number;
  };

  /**
   * @brief Parse IPAddr from string "IP - message"
   *
   * @param line string to parse
   * @return std::optional<IPv4Address>
   */
  std::optional<IPv4Address> ParseIPFromLine(const std::string &line) const;

  /**
   * @brief Output buffer and clear
   *
   * @param output output stream
   */
  void FlushBuffer(std::ostream &output);

  static constexpr size_t DEFAULT_BATCH_SIZE = 1000U;

  std::vector<LogEntry> buffer_; ///< buffer for output
  size_t batch_size_;            ///< max buffer size
};

/////////////////////////////////////////////
//
// Implementation
//
/////////////////////////////////////////////

template <FilterType T>
void StreamProcessor::Process(std::istream &input, std::ostream &output,
                              const T &filter) {
  std::string line;
  size_t line_number = 0;

  while (std::getline(input, line)) {
    ++line_number;

    auto ip = ParseIPFromLine(line);

    if (ip && filter.Matches(*ip)) {
      buffer_.push_back({*ip, line, line_number});
    }

    if (buffer_.size() >= batch_size_) {
      FlushBuffer(output);
    }
  }

  if (!buffer_.empty()) {
    FlushBuffer(output);
  }
}

inline std::optional<IPv4Address>
StreamProcessor::ParseIPFromLine(const std::string &line) const {
  size_t end = line.find(' ');
  if (end == std::string::npos) {
    return std::nullopt;
  }

  std::string_view ip_str(line.data(), end);
  return IPv4Address::FromString(ip_str);
}

// Output buffer to ostream and clear
inline void StreamProcessor::FlushBuffer(std::ostream &output) {
  if (buffer_.empty()) {
    return;
  }

  std::ranges::copy(buffer_ | std::views::transform(&LogEntry::line),
                    std::ostream_iterator<std::string>(output, "\n"));

  buffer_.clear();
  output.flush();
}
} // namespace net::details
