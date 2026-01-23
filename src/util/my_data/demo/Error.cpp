#include "Error.h"

namespace my_data {

std::string ToString(ErrorCode code) {
  switch (code) {
    case ErrorCode::Ok:               return "Ok";
    case ErrorCode::InvalidCommand:   return "InvalidCommand";
    case ErrorCode::UnknownDevice:    return "UnknownDevice";
    case ErrorCode::DeviceOffline:    return "DeviceOffline";
    case ErrorCode::Timeout:          return "Timeout";
    case ErrorCode::DriverError:      return "DriverError";
    case ErrorCode::EStop:            return "EStop";
    case ErrorCode::InternalError:    return "InternalError";
    default:                          return "UnknownErrorCode";
  }
}

} // namespace my_data