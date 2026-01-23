#include "TimeUtil.h"
#include <chrono>

namespace my_data {

TimestampMs NowMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace my_data