#include "gtest/gtest.h"

#include <vector>

#include "MyBEHelper.h"
#include "MyLEHelper.h"

TEST(MyLEHelperTest, WriteAndReadPrimitiveTypes) {
    std::vector<uint8_t> buf;

    MyLEHelper::write_uint8(buf, 0x12);
    MyLEHelper::write_uint16(buf, 0x3456);
    MyLEHelper::write_int16(buf, static_cast<int16_t>(-2));
    MyLEHelper::write_uint32(buf, 0x789ABCDEu);
    MyLEHelper::write_uint64(buf, 0x0123456789ABCDEFULL);

    size_t offset = 0;
    EXPECT_EQ(MyLEHelper::read_uint8(buf, offset), 0x12);
    EXPECT_EQ(MyLEHelper::read_uint16(buf, offset), 0x3456);
    EXPECT_EQ(MyLEHelper::read_int16(buf, offset), static_cast<int16_t>(-2));
    EXPECT_EQ(MyLEHelper::read_uint32(buf, offset), 0x789ABCDEu);
    EXPECT_EQ(MyLEHelper::read_uint64(buf, offset), 0x0123456789ABCDEFULL);
    EXPECT_EQ(offset, buf.size());
}

TEST(MyBEHelperTest, WriteAndReadPrimitiveTypes) {
    std::vector<uint8_t> buf;

    MyBEHelper::write_uint8(buf, 0x12);
    MyBEHelper::write_uint16(buf, 0x3456);
    MyBEHelper::write_int16(buf, static_cast<int16_t>(-2));
    MyBEHelper::write_uint32(buf, 0x789ABCDEu);
    MyBEHelper::write_uint64(buf, 0x0123456789ABCDEFULL);

    size_t offset = 0;
    EXPECT_EQ(MyBEHelper::read_uint8(buf, offset), 0x12);
    EXPECT_EQ(MyBEHelper::read_uint16(buf, offset), 0x3456);
    EXPECT_EQ(MyBEHelper::read_int16(buf, offset), static_cast<int16_t>(-2));
    EXPECT_EQ(MyBEHelper::read_uint32(buf, offset), 0x789ABCDEu);
    EXPECT_EQ(MyBEHelper::read_uint64(buf, offset), 0x0123456789ABCDEFULL);
    EXPECT_EQ(offset, buf.size());
}

TEST(MyLEHelperTest, BitsRoundTripWithNonByteAlignment) {
    std::vector<uint8_t> buf;
    MyLEHelper::write_bits(buf, 0xABC, 12);

    ASSERT_EQ(buf.size(), 2u);
    EXPECT_EQ(buf[0], 0xBC);
    EXPECT_EQ(buf[1], 0x0A);
    EXPECT_EQ(MyLEHelper::read_bits(buf, 0, 12), 0xABCULL);
}

TEST(MyBEHelperTest, BitsRoundTripWithNonByteAlignment) {
    std::vector<uint8_t> buf;
    MyBEHelper::write_bits(buf, 0xABC, 12);

    ASSERT_EQ(buf.size(), 2u);
    EXPECT_EQ(buf[0], 0x0A);
    EXPECT_EQ(buf[1], 0xBC);
    EXPECT_EQ(MyBEHelper::read_bits(buf, 0, 12), 0xABCULL);
}

TEST(MyEndianHelperTest, ThrowWhenBufferTooShort) {
    std::vector<uint8_t> buf = {0x01};
    size_t offset = 0;

    EXPECT_THROW(MyLEHelper::read_uint16(buf, offset), std::out_of_range);
    EXPECT_THROW(MyBEHelper::read_uint32(buf, offset), std::out_of_range);
}

TEST(MyEndianHelperTest, ThrowWhenBitLengthInvalid) {
    std::vector<uint8_t> buf;

    EXPECT_THROW(MyLEHelper::write_bits(buf, 0x1, 0), std::invalid_argument);
    EXPECT_THROW(MyBEHelper::write_bits(buf, 0x1, 65), std::invalid_argument);
    EXPECT_THROW(MyLEHelper::read_bits(std::vector<uint8_t>{0x01}, 0, 0), std::invalid_argument);
    EXPECT_THROW(MyBEHelper::read_bits(std::vector<uint8_t>{0x01}, 0, 65), std::invalid_argument);
}