#include "IdUtil.h"
#include "TimeUtil.h"
#include <atomic>
#include <sstream>

namespace my_data {

std::string GenerateId(const char* prefix) {
  static std::atomic<std::uint64_t> seq{0};
  std::ostringstream oss;
  oss << (prefix ? prefix : "id") << "-" << NowMs() << "-" << seq.fetch_add(1);
  return oss.str();
}

} // namespace my_data