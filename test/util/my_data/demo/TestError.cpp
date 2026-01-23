#include <gtest/gtest.h>
#include "MyData.h"

using namespace my_data;

TEST(MyData_Error, ErrorToStringNotEmpty) {
  EXPECT_FALSE(ToString(ErrorCode::Ok).empty());
  EXPECT_FALSE(ToString(ErrorCode::InternalError).empty());
}