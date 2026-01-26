#pragma once

#include <optional>
#include <string>

#include "ICommandNormalizer.h"
#include "MyData.h"

namespace my_control::demo {

class WindSensorCommandNormalizer final : public ICommandNormalizer {
public:
  std::optional<my_data::Task> Normalize(const my_data::RawCommand& cmd,
                                        const my_data::EdgeId& edge_id,
                                        std::string* err) override;
};

} // namespace my_control::demo