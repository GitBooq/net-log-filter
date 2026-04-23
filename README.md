# IPv4 Log Filter

A streaming log processing system with IPv4 address filtering capabilities.

## Features
- **IPv4 parsing** from logs in `IP - message` format
- **Filter types:**
  - CIDR subnets (e.g., `192.168.0.0/24`)
  - IP ranges (e.g., `10.0.0.1-10.0.0.255`)
  - Combined filters(up to 20) using logical `OR`
- **Streaming processing** with buffering up to 1000 records

## Usage Example

```cpp
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "netlogger/netlogger.hpp"

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
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception caught" << std::endl;
  return 1;
}
```

## Test input
```
# Valid IPv4
192.168.1.25 - GET /index.html
10.0.0.100 - POST /login

# Edge cases
0.0.0.0 - DEFAULT_ROUTE
255.255.255.255 - BROADCAST
192.168.1.0 - NETWORK_ADDR
192.168.1.255 - BROADCAST_ADDR

# Invalid data
192.168.1.256 - INVALID_OCTET
192.168.1 - INCOMPLETE
invalid.ip - CORRUPTED
 - EMPTY_LINE

# Filter boundary checks
10.0.0.1 - LOWER_BOUND
10.0.0.100 - UPPER_BOUND
10.0.0.101 - OUT_OF_RANGE
```

## Test Output
```
192.168.1.25 - GET /index.html
10.0.0.100 - POST /login
192.168.1.0 - NETWORK_ADDR
192.168.1.255 - BROADCAST_ADDR
10.0.0.1 - LOWER_BOUND
10.0.0.100 - UPPER_BOUND
```

## Build & Run

```bash
clone repo
run ./buildAndRun.sh