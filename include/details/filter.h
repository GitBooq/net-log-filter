/**
 * @file filter.h
 * @brief Filters
 */

#pragma once

#include <algorithm>   // for clamp
#include <concepts>    // for same_as
#include <cstdint>     // for uint32_t
#include <format>      // for format
#include <optional>    // for optional
#include <stdexcept>   // for logic_error
#include <string_view> // for string_view
#include <utility>     // for forward, pair
#include <variant>     // for variant
#include <vector>      // for vector

#include "ip_v4_address.h" // for IPv4Address

namespace net::details {

/**
 * @brief CRTP base for all filters
 */
template <typename Derived> struct Filter {
  /**
   * @brief Check if ip matches the filter
   *
   * @param ip ip address
   * @return true if IP matches filter, false otherwise
   */
  bool Matches(const IPv4Address &ip) const {
    return static_cast<const Derived *>(this)->MatchesImpl(ip);
  }

protected:
  ~Filter() = default;
};

/**
 * @brief Concept to constrain T to filters
 */
template <typename T>
concept FilterType = requires(const T &t, const IPv4Address &ip) {
  { t.Matches(ip) } -> std::same_as<bool>;
};

/**
 * @brief Subnet filter
 *  CIDR filtering
 */
class SubnetFilter : public Filter<SubnetFilter> {
private:
  uint32_t network_; ///< network address
  uint32_t mask_;    ///< network mask

  SubnetFilter(uint32_t network, uint32_t mask) noexcept
      : network_(network), mask_(mask) {}

public:
  /**
   * @brief Creates filter from CIDR string
   *
   * @param cidr CIDR string
   * @return std::optional<SubnetFilter>
   */
  static std::optional<SubnetFilter> Create(std::string_view cidr);

  /**
   * @brief Check if ip matches filter
   *
   * @param ip ip to check
   * @return true if IP matches filter, false otherwise
   */
  bool MatchesImpl(const IPv4Address &ip) const noexcept {
    return (ip.ToUint32() & mask_) == network_;
  }
};

/**
 * @brief IP Range filter
 *
 */
class RangeFilter : public Filter<RangeFilter> {
private:
  std::pair<IPv4Address, IPv4Address> ip_range_;

  RangeFilter(const IPv4Address &first, const IPv4Address &last)
      : ip_range_{first, last} {}

public:
  /**
   * @brief Creates filter from range string
   *
   * @param range range string
   * @return std::optional<RangeFilter>
   */
  static std::optional<RangeFilter> Create(std::string_view range);

  /**
   * @brief Creates filter from two strings
   *
   * @param left lo bound
   * @param right hi bound
   * @return std::optional<RangeFilter>
   */
  static std::optional<RangeFilter> Create(std::string_view left,
                                           std::string_view right);

  /**
   * @brief Check if ip matches filter
   *
   * @param ip ip to check
   * @return true if IP matches filter, false otherwise
   */
  bool MatchesImpl(const IPv4Address &ip) const noexcept {
    const auto &[left, right] = ip_range_;
    return ip == std::clamp(ip, left, right);
  }
};

/**
 * @brief Creates 32bit network mask from prefix
 *
 * @param prefix subnet prefix
 * @return constexpr uint32_t
 */
constexpr uint32_t Create32BitMask(uint32_t prefix);

/**
 * @brief Check if range [left, right] is valid
 *
 * @param left lo
 * @param right hi
 * @return true
 * @return false
 */
constexpr bool IsValidRange(uint32_t left, uint32_t right);

/**
 * @brief Check if prefix is valid
 *
 * @param prefix CIDR prefix to check
 * @return true if prefix is valid (is in range [0, 32]), false otherwise
 */
constexpr bool IsValidCIDRPrefix(int prefix);

/**
 * @brief Class to compose and store all filters
 *
 */
class CompositeFilter {
  using VarFilter = std::variant<SubnetFilter, RangeFilter>; ///< fixed filters

private:
  std::vector<VarFilter> filters_;

public:
  static constexpr std::size_t kMaxFilters = 20;

  /**
   * @brief Adds filter
   *
   * @tparam T Filter
   * @param filter filter to add
   * @throws std::length_error if exceeded kMaxFilters_
   */
  template <FilterType T> void Add(T &&filter);

  /**
   * @brief Check if ip matches any of stored filters
   *
   * @param ip ip to check
   * @return true if IP matches any filter, false otherwise
   */
  bool Matches(const IPv4Address &ip) const;

  /**
   * @brief Get filters count
   *
   * @return constexpr std::size_t
   */
  constexpr std::size_t filter_count() const noexcept {
    return filters_.size();
  }
};

/////////////////////////////////////////////
//
// Implementation
//
/////////////////////////////////////////////

// Forward dcls
class RangeFilter;
class SubnetFilter;

constexpr uint32_t Create32BitMask(uint32_t prefix) {
  return (prefix == 0) ? 0 : (0xFFFFFFFF << (32 - prefix));
}

constexpr bool IsValidRange(uint32_t left, uint32_t right) {
  return left <= right;
}

constexpr bool IsValidCIDRPrefix(int prefix) {
  return prefix >= 0 && prefix <= 32;
}

inline std::optional<SubnetFilter> SubnetFilter::Create(std::string_view cidr) {
  auto slash_pos = cidr.find('/');
  if (slash_pos == std::string_view::npos) {
    return std::nullopt;
  }

  auto ip_str = cidr.substr(0, slash_pos);
  auto ip = IPv4Address::FromString(ip_str);
  if (!ip) {
    return std::nullopt;
  }

  auto prefix_str = cidr.substr(slash_pos + 1);
  int prefix;
  auto [ptr, ec] = std::from_chars(
      prefix_str.data(), prefix_str.data() + prefix_str.size(), prefix);

  // prefix_str consist of only valid integer prefix
  if (ec != std::errc() || ptr != prefix_str.cend() ||
      !IsValidCIDRPrefix(prefix)) {
    return std::nullopt;
  }

  uint32_t mask = Create32BitMask(prefix);
  uint32_t network = ip->ToUint32() & mask;

  return SubnetFilter(network, mask);
}

inline std::optional<RangeFilter> RangeFilter::Create(std::string_view range) {
  char sep = '-';
  auto dash_pos = range.find(sep);
  if (dash_pos == std::string_view::npos) {
    return std::nullopt;
  }

  auto left_str = range.substr(0, dash_pos);
  auto right_str = range.substr(dash_pos + 1);

  auto left = IPv4Address::FromString(left_str);
  auto right = IPv4Address::FromString(right_str);

  if (!left || !right) {
    return std::nullopt;
  }
  if (!IsValidRange(left->ToUint32(), right->ToUint32())) {
    return std::nullopt;
  }

  return RangeFilter{left.value(), right.value()};
}

inline std::optional<RangeFilter> RangeFilter::Create(std::string_view left,
                                                      std::string_view right) {
  auto ip_left = IPv4Address::FromString(left);
  auto ip_right = IPv4Address::FromString(right);

  if (!ip_left || !ip_right) {
    return std::nullopt;
  }
  if (!IsValidRange(ip_left->ToUint32(), ip_right->ToUint32())) {
    return std::nullopt;
  }

  return RangeFilter{ip_left.value(), ip_right.value()};
}

template <FilterType T> inline void CompositeFilter::Add(T &&filter) {
  if (filters_.size() < kMaxFilters) {
    filters_.push_back(std::forward<T>(filter));
  } else {
    throw std::length_error(
        std::format("Maximum number of rules {} exceeded.", kMaxFilters));
  }
}

inline bool CompositeFilter::Matches(const IPv4Address &ip) const {
  for (const auto &f : filters_) {
    if (std::visit([&ip](const auto &filter) { return filter.Matches(ip); },
                   f)) {
      return true;
    }
  }
  return false;
}
} // namespace net::details