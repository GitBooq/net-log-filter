/**
 * @file netlogger.hpp
 * @brief NetLogger user interface
 */

#pragma once

#include <cstddef>          // for size_t
#include <exception>        // for exception
#include <initializer_list> // for initializer_list
#include <iosfwd>           // for istream, ostream
#include <optional>         // for optional
#include <string>           // for allocator, operator==, char_traits
#include <utility>          // for move
#include <vector>           // for vector

#include "details/filter.h" // for CompositeFilter, RangeFilter, Subn...
#include "details/stream_processor.h" // for StreamProcessor

namespace net::logger {

using net::details::CompositeFilter;
using net::details::RangeFilter;
using net::details::StreamProcessor;
using net::details::SubnetFilter;

class FilterConfigError final : public std::exception {
public:
  explicit FilterConfigError(std::string msg) noexcept : msg_(std::move(msg)) {}

  const char *what() const noexcept override { return msg_.c_str(); }

private:
  std::string msg_;
};

/**
 * @brief Filter config parser
 *
 */
struct FilterConfig {
  std::string type_;  ///< filter type
  std::string value_; ///< ipv4addr

public:
  static constexpr size_t kFields = 4; ///< number of fields in config line

  /**
   * @brief Construct a new Filter Config object from init list of strings.
   *
   * @throw FilterConfigError if bad format
   * @param il init list
   */
  FilterConfig(std::initializer_list<std::string> il) {
    if (il.size() != kFields) {
      throw FilterConfigError("FilterConfig expects 4 fields");
    }
    const std::string *arr = il.begin();
    auto [key1, val1, key2, val2] = std::tuple{arr[0], arr[1], arr[2], arr[3]};

    if (key1 != "type" || key2 != "value") {
      throw FilterConfigError(
          "Invalid format: expected 'type' and 'value' keys");
    }

    type_ = val1;
    value_ = val2;
  }
};

/**
 * @brief Create a filter object.
 * Compose filters(up to 20) such as ip should match any of stored filters.(OR)
 *
 * @param configs filter string representations
 *
 * @throws std::length_error if exceeded CompositeFilter::kMaxFilters_
 *
 * @throws FilterConfigError if unsupported filter type
 *
 * @note Usage:
 *   auto filter =
 *    CreateFilter({{"type", "subnet", "value", "192.168.88.0/24"},
 *                  {"type", "range", "value", "10.0.0.1-10.0.0.254"}});
 *
 * @note Supported filter types: subnet, range.
 * @return CompositeFilter
 */
inline CompositeFilter CreateFilter(const std::vector<FilterConfig> &configs) {
  CompositeFilter filter;

  for (const auto &cfg : configs) {
    if (cfg.type_ == "subnet") {
      if (auto subnet = SubnetFilter::Create(cfg.value_)) {
        filter.Add(std::move(*subnet)); // can throw
      } else {
        throw FilterConfigError("Bad subnet filter format.");
      }
    } else if (cfg.type_ == "range") {
      if (auto range = RangeFilter::Create(cfg.value_)) {
        filter.Add(std::move(*range)); // can throw
      } else {
        throw FilterConfigError("Bad range filter format.");
      }
    } else {
      throw FilterConfigError("Unknown filter type.");
    }
  }

  return filter;
}

/**
 * @brief Start parse and filtering process.
 * Parse input stream, apply filter and output to output stream.
 * @param input
 * @param output
 * @param filter
 */
inline void ProcessStream(std::istream &input, std::ostream &output,
                          const CompositeFilter &filter) {
  StreamProcessor processor;
  processor.Process(input, output, filter);
}

} // namespace net::logger