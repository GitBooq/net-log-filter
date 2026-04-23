#include "details/filter.h"

#include <gtest/gtest.h>

#include "details/ip_v4_address.h"

using net::details::IPv4Address;
using net::details::RangeFilter;
using net::details::SubnetFilter;

class SubnetFilterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    subnet_24_192_168_1 = SubnetFilter::Create("192.168.1.0/24");
    subnet_16_192_168 = SubnetFilter::Create("192.168.0.0/16");
    subnet_8_10 = SubnetFilter::Create("10.0.0.0/8");
    subnet_32_single = SubnetFilter::Create("192.168.1.100/32");
    subnet_0_all = SubnetFilter::Create("0.0.0.0/0");

    ip_192_168_1_1 = IPv4Address::FromString("192.168.1.1").value();
    ip_192_168_1_100 = IPv4Address::FromString("192.168.1.100").value();
    ip_192_168_1_255 = IPv4Address::FromString("192.168.1.255").value();
    ip_192_168_2_1 = IPv4Address::FromString("192.168.2.1").value();
    ip_192_169_1_1 = IPv4Address::FromString("192.169.1.1").value();
    ip_10_0_0_1 = IPv4Address::FromString("10.0.0.1").value();
    ip_10_10_10_10 = IPv4Address::FromString("10.10.10.10").value();
    ip_11_0_0_1 = IPv4Address::FromString("11.0.0.1").value();
  }

  std::optional<SubnetFilter> subnet_24_192_168_1;
  std::optional<SubnetFilter> subnet_16_192_168;
  std::optional<SubnetFilter> subnet_8_10;
  std::optional<SubnetFilter> subnet_32_single;
  std::optional<SubnetFilter> subnet_0_all;

  IPv4Address ip_192_168_1_100;
  IPv4Address ip_192_168_1_255;
  IPv4Address ip_192_168_1_1;
  IPv4Address ip_192_168_2_1;
  IPv4Address ip_192_169_1_1;
  IPv4Address ip_10_0_0_1;
  IPv4Address ip_10_10_10_10;
  IPv4Address ip_11_0_0_1;
};

TEST_F(SubnetFilterTest, CreateValidSubnet) {
  EXPECT_TRUE(subnet_24_192_168_1.has_value());
  EXPECT_TRUE(subnet_16_192_168.has_value());
  EXPECT_TRUE(subnet_8_10.has_value());
  EXPECT_TRUE(subnet_32_single.has_value());
  EXPECT_TRUE(subnet_0_all.has_value());
}

TEST_F(SubnetFilterTest, CreateInvalidSubnet) {
  EXPECT_FALSE(SubnetFilter::Create("192.168.1.0").has_value());
  EXPECT_FALSE(SubnetFilter::Create("192.168.1.0/33").has_value());
  EXPECT_FALSE(SubnetFilter::Create("192.168.1.0/-1").has_value());
  EXPECT_FALSE(SubnetFilter::Create("invalid/24").has_value());
  EXPECT_FALSE(SubnetFilter::Create("192.168.1.256/24").has_value());
  EXPECT_FALSE(SubnetFilter::Create("/24").has_value());
  EXPECT_FALSE(SubnetFilter::Create("192.168.1.0/24abc").has_value());
}

TEST_F(SubnetFilterTest, Subnet24MatchesIPInRange) {
  ASSERT_TRUE(subnet_24_192_168_1.has_value());

  EXPECT_TRUE(subnet_24_192_168_1->Matches(ip_192_168_1_1));
  EXPECT_TRUE(subnet_24_192_168_1->Matches(ip_192_168_1_100));
  EXPECT_TRUE(subnet_24_192_168_1->Matches(ip_192_168_1_255));
}

TEST_F(SubnetFilterTest, Subnet24RejectsIPOutOfRange) {
  ASSERT_TRUE(subnet_24_192_168_1.has_value());

  EXPECT_FALSE(subnet_24_192_168_1->Matches(ip_192_168_2_1));
  EXPECT_FALSE(subnet_24_192_168_1->Matches(ip_192_169_1_1));
  EXPECT_FALSE(subnet_24_192_168_1->Matches(ip_10_0_0_1));
}

TEST_F(SubnetFilterTest, Subnet16MatchesIPInRange) {
  ASSERT_TRUE(subnet_16_192_168.has_value());

  EXPECT_TRUE(subnet_16_192_168->Matches(ip_192_168_1_1));
  EXPECT_TRUE(subnet_16_192_168->Matches(ip_192_168_2_1));
}

TEST_F(SubnetFilterTest, Subnet16RejectsIPOutOfRange) {
  ASSERT_TRUE(subnet_16_192_168.has_value());

  EXPECT_FALSE(subnet_16_192_168->Matches(ip_192_169_1_1));
  EXPECT_FALSE(subnet_16_192_168->Matches(ip_10_0_0_1));
}

TEST_F(SubnetFilterTest, Subnet8MatchesIPInRange) {
  ASSERT_TRUE(subnet_8_10.has_value());

  EXPECT_TRUE(subnet_8_10->Matches(ip_10_0_0_1));
  EXPECT_TRUE(subnet_8_10->Matches(ip_10_10_10_10));
}

TEST_F(SubnetFilterTest, Subnet8RejectsIPOutOfRange) {
  ASSERT_TRUE(subnet_8_10.has_value());

  EXPECT_FALSE(subnet_8_10->Matches(ip_11_0_0_1));
  EXPECT_FALSE(subnet_8_10->Matches(ip_192_168_1_1));
}

TEST_F(SubnetFilterTest, Subnet32MatchesExactIP) {
  ASSERT_TRUE(subnet_32_single.has_value());

  EXPECT_TRUE(subnet_32_single->Matches(ip_192_168_1_100));
}

TEST_F(SubnetFilterTest, Subnet32RejectsOtherIPs) {
  ASSERT_TRUE(subnet_32_single.has_value());

  EXPECT_FALSE(subnet_32_single->Matches(ip_192_168_1_1));
  EXPECT_FALSE(subnet_32_single->Matches(ip_192_168_1_255));
  EXPECT_FALSE(subnet_32_single->Matches(ip_192_168_2_1));
}

TEST_F(SubnetFilterTest, Subnet0MatchesAllIPs) {
  ASSERT_TRUE(subnet_0_all.has_value());

  EXPECT_TRUE(subnet_0_all->Matches(ip_192_168_1_1));
  EXPECT_TRUE(subnet_0_all->Matches(ip_10_0_0_1));
  EXPECT_TRUE(subnet_0_all->Matches(ip_11_0_0_1));
}

TEST_F(SubnetFilterTest, EdgeCases) {
  auto subnet_zero = SubnetFilter::Create("0.0.0.0/32");
  ASSERT_TRUE(subnet_zero.has_value());

  auto ip_zero = IPv4Address::FromString("0.0.0.0").value();
  auto ip_one = IPv4Address::FromString("0.0.0.1").value();

  EXPECT_TRUE(subnet_zero->Matches(ip_zero));
  EXPECT_FALSE(subnet_zero->Matches(ip_one));

  auto subnet_bcast = SubnetFilter::Create("255.255.255.255/32");
  ASSERT_TRUE(subnet_bcast.has_value());

  auto ip_bcast = IPv4Address::FromString("255.255.255.255").value();
  auto ip_other = IPv4Address::FromString("255.255.255.254").value();

  EXPECT_TRUE(subnet_bcast->Matches(ip_bcast));
  EXPECT_FALSE(subnet_bcast->Matches(ip_other));
}

//
// FILTER TEST
//
TEST(RangeFilterTest, CreateValidRange) {
  auto range = RangeFilter::Create("10.0.0.1", "10.0.0.100");
  EXPECT_TRUE(range.has_value());
}

TEST(RangeFilterTest, CreateValidRangeFromOneString) {
  auto range = RangeFilter::Create("10.0.0.1-10.0.0.100");
  EXPECT_TRUE(range.has_value());
}

TEST(RangeFilterTest, CreateInvalidRangeFromOneString) {
  auto range = RangeFilter::Create("10.0.0.1- 10.0.0.100");
  auto r2 = RangeFilter::Create("10.0.0.1 -10.0.0.100");
  auto r3 = RangeFilter::Create("10.0.0.1--10.0.0.100");
  auto r4 = RangeFilter::Create("10.0.0.110.0.0.100");
  auto r5 = RangeFilter::Create("10.0.0.1 10.0.0.100");
  auto r6 = RangeFilter::Create("10.0.0.1-");
  auto r7 = RangeFilter::Create("-10.0.0.100");

  EXPECT_FALSE(range.has_value());
  EXPECT_FALSE(r2.has_value());
  EXPECT_FALSE(r3.has_value());
  EXPECT_FALSE(r4.has_value());
  EXPECT_FALSE(r5.has_value());
  EXPECT_FALSE(r6.has_value());
  EXPECT_FALSE(r7.has_value());
}

TEST(RangeFilterTest, CreateValidRangeWithBoundaries) {
  auto range1 = RangeFilter::Create("0.0.0.0", "255.255.255.255");
  EXPECT_TRUE(range1.has_value());

  auto range2 = RangeFilter::Create("192.168.1.1", "192.168.1.1");
  EXPECT_TRUE(range2.has_value());
}

TEST(RangeFilterTest, CreateInvalidRangeStartGreaterThanEnd) {
  auto range = RangeFilter::Create("10.0.0.100", "10.0.0.1");
  EXPECT_FALSE(range.has_value());
}

TEST(RangeFilterTest, CreateInvalidRangeInvalidStart) {
  auto range = RangeFilter::Create("invalid", "10.0.0.100");
  EXPECT_FALSE(range.has_value());
}

TEST(RangeFilterTest, CreateInvalidRangeInvalidEnd) {
  auto range = RangeFilter::Create("10.0.0.1", "invalid");
  EXPECT_FALSE(range.has_value());
}

TEST(RangeFilterTest, MatchesIPInRange) {
  auto range = RangeFilter::Create("10.0.0.1", "10.0.0.100");
  ASSERT_TRUE(range.has_value());

  auto ip_lower = IPv4Address::FromString("10.0.0.1").value();
  auto ip_middle = IPv4Address::FromString("10.0.0.50").value();
  auto ip_upper = IPv4Address::FromString("10.0.0.100").value();

  EXPECT_TRUE(range->Matches(ip_lower));
  EXPECT_TRUE(range->Matches(ip_middle));
  EXPECT_TRUE(range->Matches(ip_upper));
}

TEST(RangeFilterTest, RejectsIPBelowRange) {
  auto range = RangeFilter::Create("10.0.0.10", "10.0.0.20");
  ASSERT_TRUE(range.has_value());

  auto ip_below = IPv4Address::FromString("10.0.0.5").value();
  auto ip_just_below = IPv4Address::FromString("10.0.0.9").value();

  EXPECT_FALSE(range->Matches(ip_below));
  EXPECT_FALSE(range->Matches(ip_just_below));
}

TEST(RangeFilterTest, RejectsIPAboveRange) {
  auto range = RangeFilter::Create("10.0.0.10", "10.0.0.20");
  ASSERT_TRUE(range.has_value());

  auto ip_above = IPv4Address::FromString("10.0.0.25").value();
  auto ip_just_above = IPv4Address::FromString("10.0.0.21").value();

  EXPECT_FALSE(range->Matches(ip_above));
  EXPECT_FALSE(range->Matches(ip_just_above));
}

TEST(RangeFilterTest, RejectsIPFromDifferentSubnet) {
  auto range = RangeFilter::Create("192.168.1.1", "192.168.1.100");
  ASSERT_TRUE(range.has_value());

  auto ip_different = IPv4Address::FromString("10.0.0.50").value();
  auto ip_other_subnet = IPv4Address::FromString("192.168.2.50").value();

  EXPECT_FALSE(range->Matches(ip_different));
  EXPECT_FALSE(range->Matches(ip_other_subnet));
}

TEST(RangeFilterTest, SingleIPRange) {
  auto range = RangeFilter::Create("192.168.1.1", "192.168.1.1");
  ASSERT_TRUE(range.has_value());

  auto ip_match = IPv4Address::FromString("192.168.1.1").value();
  auto ip_mismatch = IPv4Address::FromString("192.168.1.2").value();

  EXPECT_TRUE(range->Matches(ip_match));
  EXPECT_FALSE(range->Matches(ip_mismatch));
}

TEST(RangeFilterTest, RangeFromZeroToBroadcast) {
  auto range = RangeFilter::Create("0.0.0.0", "255.255.255.255");
  ASSERT_TRUE(range.has_value());

  auto ip1 = IPv4Address::FromString("0.0.0.0").value();
  auto ip2 = IPv4Address::FromString("127.0.0.1").value();
  auto ip3 = IPv4Address::FromString("192.168.1.1").value();
  auto ip4 = IPv4Address::FromString("255.255.255.255").value();

  EXPECT_TRUE(range->Matches(ip1));
  EXPECT_TRUE(range->Matches(ip2));
  EXPECT_TRUE(range->Matches(ip3));
  EXPECT_TRUE(range->Matches(ip4));
}

TEST(RangeFilterTest, RangeFromZeroToSpecific) {
  auto range = RangeFilter::Create("0.0.0.0", "10.0.0.100");
  ASSERT_TRUE(range.has_value());

  auto ip_zero = IPv4Address::FromString("0.0.0.0").value();
  auto ip_small = IPv4Address::FromString("1.1.1.1").value();
  auto ip_boundary = IPv4Address::FromString("10.0.0.100").value();
  auto ip_out = IPv4Address::FromString("10.0.0.101").value();

  EXPECT_TRUE(range->Matches(ip_zero));
  EXPECT_TRUE(range->Matches(ip_small));
  EXPECT_TRUE(range->Matches(ip_boundary));
  EXPECT_FALSE(range->Matches(ip_out));
}

TEST(RangeFilterTest, RangeFromSpecificToBroadcast) {
  auto range = RangeFilter::Create("240.0.0.0", "255.255.255.255");
  ASSERT_TRUE(range.has_value());

  auto ip_lower = IPv4Address::FromString("240.0.0.0").value();
  auto ip_middle = IPv4Address::FromString("250.0.0.1").value();
  auto ip_broadcast = IPv4Address::FromString("255.255.255.255").value();
  auto ip_out = IPv4Address::FromString("239.255.255.255").value();

  EXPECT_TRUE(range->Matches(ip_lower));
  EXPECT_TRUE(range->Matches(ip_middle));
  EXPECT_TRUE(range->Matches(ip_broadcast));
  EXPECT_FALSE(range->Matches(ip_out));
}

TEST(RangeFilterTest, PrivateNetworkRange) {
  auto range = RangeFilter::Create("192.168.0.0", "192.168.255.255");
  ASSERT_TRUE(range.has_value());

  auto ip_start = IPv4Address::FromString("192.168.0.0").value();
  auto ip_middle = IPv4Address::FromString("192.168.1.100").value();
  auto ip_end = IPv4Address::FromString("192.168.255.255").value();
  auto ip_outside = IPv4Address::FromString("192.169.0.1").value();

  EXPECT_TRUE(range->Matches(ip_start));
  EXPECT_TRUE(range->Matches(ip_middle));
  EXPECT_TRUE(range->Matches(ip_end));
  EXPECT_FALSE(range->Matches(ip_outside));
}

TEST(RangeFilterTest, LoopbackRange) {
  auto range = RangeFilter::Create("127.0.0.0", "127.255.255.255");
  ASSERT_TRUE(range.has_value());

  auto ip_localhost = IPv4Address::FromString("127.0.0.1").value();
  auto ip_loopback = IPv4Address::FromString("127.255.255.255").value();
  auto ip_outside = IPv4Address::FromString("128.0.0.1").value();

  EXPECT_TRUE(range->Matches(ip_localhost));
  EXPECT_TRUE(range->Matches(ip_loopback));
  EXPECT_FALSE(range->Matches(ip_outside));
}
