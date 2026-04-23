#include "details/ip_v4_address.h"

#include <gtest/gtest.h>

using net::details::IPv4Address;

TEST(IPv4AddressTest, CreateFromValidString) {
  std::string a{"192.168.88.1"};
  std::string b{"127.0.0.1"};
  std::string c{a};
  auto from_a = IPv4Address::FromString(a);
  auto from_b = IPv4Address::FromString(b);
  auto from_c = IPv4Address::FromString(c);

  EXPECT_NE(from_a, from_b);
  EXPECT_EQ(from_a, from_c);
}

TEST(IPv4AddressTest, CreateFromInvalidString) {
  std::string a{"192.168.88."};
  std::string b{"x192.168.88.1"};
  std::string c{"192.16x8.88.1"};
  std::string d{"192.168.88.1x"};
  std::string e{"192.168.88.256"};
  std::string f{"192.168.-88.1"};
  std::string g{"-192.168.88.1"};
  std::string h{"192.1 68.88.1"};
  std::string leadZero{"192.068.88.1"};
  std::vector<std::string_view> strs{a, b, c, d, e, f, g, h, leadZero};

  for (auto sv : strs) {
    auto value = IPv4Address::FromString(sv);
    EXPECT_EQ(value, std::nullopt);
  }
}

TEST(IPv4AddressTest, CreateFromBytes) {
  std::array<uint32_t, 4> a{192, 168, 88, 1};
  std::array<uint32_t, 4> b{127, 0, 0, 1};
  auto c{a};
  auto from_a = IPv4Address::FromBytes(a);
  auto from_b = IPv4Address::FromBytes(b);
  auto from_c = IPv4Address::FromBytes(c);

  EXPECT_NE(from_a, from_b);
  EXPECT_EQ(from_a, from_c);
}

TEST(IPv4AddressTest, ComparisonOperators) {
  auto ip1 = IPv4Address::FromString("192.168.1.1").value();
  auto ip2 = IPv4Address::FromString("192.168.1.1").value();
  auto ip3 = IPv4Address::FromString("192.168.1.2").value();
  auto ip4 = IPv4Address::FromString("10.0.0.1").value();

  EXPECT_TRUE(ip1 == ip2);
  EXPECT_FALSE(ip1 == ip3);

  EXPECT_TRUE(ip1 != ip3);
  EXPECT_FALSE(ip1 != ip2);

  EXPECT_TRUE(ip4 < ip1);
  EXPECT_TRUE(ip1 < ip3);
  EXPECT_FALSE(ip3 < ip1);

  EXPECT_TRUE(ip3 > ip1);
  EXPECT_TRUE(ip1 > ip4);
  EXPECT_FALSE(ip1 > ip3);

  EXPECT_TRUE(ip1 <= ip2);
  EXPECT_TRUE(ip1 <= ip3);
  EXPECT_FALSE(ip3 <= ip1);

  EXPECT_TRUE(ip3 >= ip1);
  EXPECT_TRUE(ip1 >= ip4);
  EXPECT_FALSE(ip1 >= ip3);
}