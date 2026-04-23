#include "details/stream_processor.h"

#include <sstream>
#include <string>

#include "gtest/gtest.h"

using namespace net::details;

constexpr auto kNpos = std::string_view::npos;

std::string MakeLogLine(const std::string &ip, const std::string &message) {
  return ip + " - " + message;
}

TEST(StreamProcessorTest, EmptyInput) {
  std::stringstream input;
  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.1.0/24").value();
  StreamProcessor processor;

  processor.Process(input, output, filter);

  EXPECT_TRUE(output.str().empty());
}

TEST(StreamProcessorTest, SingleLineMatch) {
  std::stringstream input;
  input << MakeLogLine("192.168.1.1", "GET /index.html") << "\n";

  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.1.0/24").value();
  StreamProcessor processor;

  processor.Process(input, output, filter);

  EXPECT_EQ(output.str(), "192.168.1.1 - GET /index.html\n");
}

TEST(StreamProcessorTest, SingleLineNoMatch) {
  std::stringstream input;
  input << MakeLogLine("10.0.0.1", "GET /index.html") << "\n";

  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.1.0/24").value();
  StreamProcessor processor;

  processor.Process(input, output, filter);

  EXPECT_TRUE(output.str().empty());
}

TEST(StreamProcessorTest, MultipleLinesMixed) {
  std::stringstream input;
  input << MakeLogLine("192.168.1.1", "GET /page1") << "\n";
  input << MakeLogLine("10.0.0.1", "POST /login") << "\n";
  input << MakeLogLine("192.168.1.100", "GET /page2") << "\n";
  input << MakeLogLine("invalid.ip", "SKIP") << "\n";
  input << MakeLogLine("192.168.1.50", "GET /page3") << "\n";

  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.1.0/24").value();
  StreamProcessor processor;

  processor.Process(input, output, filter);

  auto result = output.view();
  EXPECT_NE(result.find("192.168.1.1 - GET /page1"), kNpos);
  EXPECT_EQ(result.find("10.0.0.1 - POST /login"), kNpos);
  EXPECT_NE(result.find("192.168.1.100 - GET /page2"), kNpos);
  EXPECT_EQ(result.find("invalid.ip - SKIP"), kNpos);
  EXPECT_NE(result.find("192.168.1.50 - GET /page3"), kNpos);
}

TEST(StreamProcessorTest, BatchOutputExact) {
  std::stringstream input;
  const int kBatchSize = 5;

  for (int i = 0; i < kBatchSize; ++i) {
    input << MakeLogLine("192.168.1.1", "MESSAGE " + std::to_string(i)) << "\n";
  }

  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.1.0/24").value();
  StreamProcessor processor(kBatchSize);

  processor.Process(input, output, filter);

  auto view = output.view();
  int lines = std::count(view.begin(), view.end(), '\n');

  EXPECT_EQ(lines, kBatchSize);
}

TEST(StreamProcessorTest, BatchOutputPartial) {
  std::stringstream input;
  const int kBatchSize = 1000;
  const int kPartial = 342;

  for (int i = 0; i < kPartial; ++i) {
    input << MakeLogLine("192.168.1.1", "MESSAGE " + std::to_string(i)) << "\n";
  }

  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.1.0/24").value();
  StreamProcessor processor(kBatchSize);

  processor.Process(input, output, filter);

  auto view = output.view();
  int lines = std::count(view.begin(), view.end(), '\n');
  EXPECT_EQ(lines, kPartial);
}

TEST(StreamProcessorTest, BatchOutputMultipleBatches) {
  std::stringstream input;
  const int kTotal = 2500;
  const int kBatchSize = 1000;

  for (int i = 0; i < kTotal; ++i) {
    input << MakeLogLine("192.168.1.1", "MESSAGE " + std::to_string(i)) << "\n";
  }

  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.1.0/24").value();
  StreamProcessor processor(kBatchSize);

  processor.Process(input, output, filter);

  auto view = output.view();
  int lines = std::count(view.begin(), view.end(), '\n');
  EXPECT_EQ(lines, kTotal);
}

TEST(StreamProcessorTest, WithRangeFilter) {
  std::stringstream input;
  input << MakeLogLine("10.0.0.1", "LOWER_BOUND") << "\n";
  input << MakeLogLine("10.0.0.50", "MIDDLE") << "\n";
  input << MakeLogLine("10.0.0.100", "UPPER_BOUND") << "\n";
  input << MakeLogLine("10.0.0.101", "OUT_OF_RANGE") << "\n";
  input << MakeLogLine("192.168.1.1", "DIFFERENT_SUBNET") << "\n";

  std::stringstream output;

  auto filter = RangeFilter::Create("10.0.0.1", "10.0.0.100").value();
  StreamProcessor processor;

  processor.Process(input, output, filter);

  auto result = output.view();
  EXPECT_NE(result.find("10.0.0.1 - LOWER_BOUND"), kNpos);
  EXPECT_NE(result.find("10.0.0.50 - MIDDLE"), kNpos);
  EXPECT_NE(result.find("10.0.0.100 - UPPER_BOUND"), kNpos);
  EXPECT_EQ(result.find("10.0.0.101 - OUT_OF_RANGE"), kNpos);
  EXPECT_EQ(result.find("192.168.1.1 - DIFFERENT_SUBNET"), kNpos);
}

TEST(StreamProcessorTest, WithCompositeFilter) {
  std::stringstream input;
  input << MakeLogLine("192.168.1.15", "MATCH") << "\n";
  input << MakeLogLine("192.168.1.5", "NOT_MATCH") << "\n";
  input << MakeLogLine("10.0.0.50", "MATCH") << "\n";
  input << MakeLogLine("192.168.2.1", "NOT_MATCH") << "\n";

  std::stringstream output;

  auto subnet = SubnetFilter::Create("10.0.0.0/24").value();
  auto range = RangeFilter::Create("192.168.1.10", "192.168.1.20").value();

  CompositeFilter composite;
  composite.Add(std::move(subnet));
  composite.Add(std::move(range));

  StreamProcessor processor;
  processor.Process(input, output, composite);

  auto result = output.view();
  EXPECT_NE(result.find("192.168.1.15 - MATCH"), kNpos);
  EXPECT_EQ(result.find("192.168.1.5 - NOT_MATCH"), kNpos);
  EXPECT_NE(result.find("10.0.0.50 - MATCH"), kNpos);
  EXPECT_EQ(result.find("192.168.2.1 - NOT_MATCH"), kNpos);
}

TEST(StreamProcessorTest, InvalidIpLinesIgnored) {
  std::stringstream input;
  input << MakeLogLine("192.168.1.1", "VALID") << "\n";
  input << "invalid.ip - CORRUPTED\n";
  input << "192.168.1.256 - INVALID_OCTET\n";
  input << "192.168.1 - INCOMPLETE\n";
  input << " - EMPTY_LINE\n";
  input << MakeLogLine("192.168.1.100", "VALID2") << "\n";

  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.1.0/24").value();
  StreamProcessor processor;

  processor.Process(input, output, filter);

  auto result = output.view();
  EXPECT_NE(result.find("192.168.1.1 - VALID"), kNpos);
  EXPECT_NE(result.find("192.168.1.100 - VALID2"), kNpos);
  EXPECT_EQ(result.find("invalid.ip - CORRUPTED"), kNpos);
  EXPECT_EQ(result.find("192.168.1.256 - INVALID_OCTET"), kNpos);
  EXPECT_EQ(result.find("192.168.1 - INCOMPLETE"), kNpos);
  EXPECT_EQ(result.find(" - EMPTY_LINE"), kNpos);
}

TEST(StreamProcessorTest, EdgeCaseZeroIP) {
  std::stringstream input;
  input << MakeLogLine("0.0.0.0", "DEFAULT_ROUTE") << "\n";
  input << MakeLogLine("255.255.255.255", "BROADCAST") << "\n";

  std::stringstream output;

  auto filter = RangeFilter::Create("0.0.0.0", "255.255.255.255").value();
  StreamProcessor processor;

  processor.Process(input, output, filter);

  auto result = output.view();
  EXPECT_NE(result.find("0.0.0.0 - DEFAULT_ROUTE"), kNpos);
  EXPECT_NE(result.find("255.255.255.255 - BROADCAST"), kNpos);
}

TEST(StreamProcessorTest, EdgeCaseNetworkAndBroadcast) {
  std::stringstream input;
  input << MakeLogLine("192.168.0.0", "NETWORK_ADDR") << "\n";
  input << MakeLogLine("192.168.0.255", "BROADCAST_ADDR") << "\n";

  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.0.0/24").value();
  StreamProcessor processor;

  processor.Process(input, output, filter);

  auto result = output.view();
  EXPECT_NE(result.find("192.168.0.0 - NETWORK_ADDR"), kNpos);
  EXPECT_NE(result.find("192.168.0.255 - BROADCAST_ADDR"), kNpos);
}

TEST(StreamProcessorTest, LargeInputNoMemoryLeak) {
  std::stringstream input;
  const int kLargeCount = 100000;

  for (int i = 0; i < kLargeCount; ++i) {
    input << MakeLogLine("192.168.1.1", "MESSAGE") << "\n";
  }

  std::stringstream output;

  auto filter = SubnetFilter::Create("192.168.1.0/24").value();
  StreamProcessor processor(1000);

  processor.Process(input, output, filter);

  auto view = output.view();
  int lines = std::count(view.begin(), view.end(), '\n');
  EXPECT_EQ(lines, kLargeCount);
}