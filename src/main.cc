#include <exception> // for exception
#include <fstream>   // for basic_ostream, char_traits, opera...
#include <iostream>  // for cerr, cout
#include <sstream>   // for stringstream
#include <vector>    // for vector

#include "net_logger/net_logger.h" // for FilterConfig, CreateFilter

using namespace net::logger;

int main() try {
  std::ifstream input("test.log");

  if (!input.is_open()) {
    std::cerr << "Can't open file\n";
    return 1;
  }

  std::stringstream buffer;
  buffer << input.rdbuf();

  auto filter =
      CreateFilter({{"type", "subnet", "value", "192.168.1.0/24"},
                    {"type", "range", "value", "10.0.0.1-10.0.0.100"}});

  ProcessStream(buffer, std::cout, filter);

  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception caught" << std::endl;
  return 1;
}