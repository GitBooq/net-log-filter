#include "net_logger/net_logger.h"

#include <cassert>
#include <string>

#include "gtest/gtest.h"

using namespace net::logger;

namespace {
constexpr auto MaxFilterRules = net::details::CompositeFilter::kMaxFilters;
}

TEST(NetLoggerTest, CreateValidFilter) {
  EXPECT_NO_THROW(
      CreateFilter({{"type", "subnet", "value", "192.168.1.0/24"}}));
  EXPECT_NO_THROW(
      CreateFilter({{"type", "range", "value", "10.0.0.1-10.0.1.255"}}));
}

TEST(NetLoggerTest, CreateInvalidFilterBadFormat) {
  EXPECT_THROW(CreateFilter({{"typeo", "subnet", "value", "192.168.1.0/24"}}),
               FilterConfigError);
  EXPECT_THROW(CreateFilter({{"type", "subnet", "valuee", "192.168.1.0/24"}}),
               FilterConfigError);
  EXPECT_THROW(
      CreateFilter({{"type", "subnet", "value", "192.168.1.0/24", "id"}}),
      FilterConfigError);
  EXPECT_THROW(CreateFilter({{"type", "!subnet", "value", "192.168.1.0/24"}}),
               FilterConfigError);
  EXPECT_THROW(
      CreateFilter({{"type", "range!", "value", "10.0.0.1-10.0.1.255"}}),
      FilterConfigError);

  // range type but subnet given
  EXPECT_THROW(CreateFilter({{"type", "range", "value", "192.168.1.0/24"}}),
               FilterConfigError);
  // subnet type but range given
  EXPECT_THROW(
      CreateFilter({{"type", "subnet", "value", "10.0.0.1-10.0.1.255"}}),
      FilterConfigError);
}

TEST(NetLoggerTest, CreateValidFilterMaxRules) {
  std::vector<FilterConfig> configs;

  for (auto i = 0UZ; i < MaxFilterRules; ++i) {
    assert(i <= 255);
    configs.push_back(
        {"type", "subnet", "value", std::format("192.168.{}.0/24", i)});
  }

  EXPECT_NO_THROW(CreateFilter(configs));
}

TEST(NetLoggerTest, CreateInvalidFilterMaxRules) {
  std::vector<FilterConfig> configs;

  for (auto i = 0UZ; i < MaxFilterRules + 1; ++i) {
    assert(i <= 255);
    configs.push_back(
        {"type", "subnet", "value", std::format("192.168.{}.0/24", i)});
  }

  EXPECT_THROW(CreateFilter(configs), std::length_error);
}