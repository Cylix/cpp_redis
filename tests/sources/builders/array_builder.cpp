#include <gtest/gtest.h>
#include <cpp_redis/builders/simple_string_builder.hpp>
#include <cpp_redis/builders/array_builder.hpp>
#include <cpp_redis/builders/integer_builder.hpp>
#include <cpp_redis/builders/bulk_string_builder.hpp>
#include <cpp_redis/builders/error_builder.hpp>
#include <cpp_redis/redis_error.hpp>

TEST(ArrayBuilder, WithNoData) {
    cpp_redis::builders::array_builder builder;

    EXPECT_EQ(false, builder.reply_ready());
}

TEST(ArrayBuilder, WithNotEnoughData) {
    cpp_redis::builders::array_builder builder;

    std::string buffer = "1\r\n";
    builder << buffer;

    EXPECT_EQ(false, builder.reply_ready());
}

TEST(ArrayBuilder, WithPartOfEndSequence) {
    cpp_redis::builders::array_builder builder;

    std::string buffer = "1\r\n+hello\r";
    builder << buffer;

    EXPECT_EQ(false, builder.reply_ready());
}

TEST(ArrayBuilder, WithAllInOneTime) {
    cpp_redis::builders::array_builder builder;

    std::string buffer = "4\r\n+simple_string\r\n-error\r\n:42\r\n$5\r\nhello\r\n";
    builder << buffer;

    EXPECT_EQ(true, builder.reply_ready());
    EXPECT_EQ("", buffer);

    auto reply = std::dynamic_pointer_cast<cpp_redis::array_reply>(builder.get_reply());
    EXPECT_TRUE(reply != nullptr);
    EXPECT_EQ(4U, reply->size());

    auto row_1 = std::dynamic_pointer_cast<cpp_redis::simple_string_reply>(reply->get(0));
    EXPECT_TRUE(row_1 != nullptr);
    EXPECT_EQ("simple_string", row_1->str());

    auto row_2 = std::dynamic_pointer_cast<cpp_redis::error_reply>(reply->get(1));
    EXPECT_TRUE(row_2 != nullptr);
    EXPECT_EQ("error", row_2->str());

    auto row_3 = std::dynamic_pointer_cast<cpp_redis::integer_reply>(reply->get(2));
    EXPECT_TRUE(row_3 != nullptr);
    EXPECT_EQ(42, row_3->val());

    auto row_4 = std::dynamic_pointer_cast<cpp_redis::bulk_string_reply>(reply->get(3));
    EXPECT_TRUE(row_4 != nullptr);
    EXPECT_EQ("hello", row_4->str());
}

TEST(ArrayBuilder, WithAllInMultipleTimes) {
    cpp_redis::builders::array_builder builder;

    std::string buffer = "4\r\n+simple_string\r";
    builder << buffer;
    buffer += "\n-error\r\n:42\r\n";
    builder << buffer;
    buffer += "$5\r\nhello\r\n";
    builder << buffer;

    EXPECT_EQ(true, builder.reply_ready());
    EXPECT_EQ("", buffer);

    auto reply = std::dynamic_pointer_cast<cpp_redis::array_reply>(builder.get_reply());
    EXPECT_TRUE(reply != nullptr);
    EXPECT_EQ(4U, reply->size());

    auto row_1 = std::dynamic_pointer_cast<cpp_redis::simple_string_reply>(reply->get(0));
    EXPECT_TRUE(row_1 != nullptr);
    EXPECT_EQ("simple_string", row_1->str());

    auto row_2 = std::dynamic_pointer_cast<cpp_redis::error_reply>(reply->get(1));
    EXPECT_TRUE(row_2 != nullptr);
    EXPECT_EQ("error", row_2->str());

    auto row_3 = std::dynamic_pointer_cast<cpp_redis::integer_reply>(reply->get(2));
    EXPECT_TRUE(row_3 != nullptr);
    EXPECT_EQ(42, row_3->val());

    auto row_4 = std::dynamic_pointer_cast<cpp_redis::bulk_string_reply>(reply->get(3));
    EXPECT_TRUE(row_4 != nullptr);
    EXPECT_EQ("hello", row_4->str());
}

TEST(ArrayBuilder, EmptyArray) {
    cpp_redis::builders::array_builder builder;

    std::string buffer = "0\r\n";
    builder << buffer;

    EXPECT_EQ(true, builder.reply_ready());
    EXPECT_EQ("", buffer);

    auto reply = std::dynamic_pointer_cast<cpp_redis::array_reply>(builder.get_reply());
    EXPECT_TRUE(reply != nullptr);
    EXPECT_EQ(0U, reply->size());
}


TEST(ArrayBuilder, InvalidSize) {
    cpp_redis::builders::array_builder builder;

    std::string buffer = "-1\r\n";
    EXPECT_THROW(builder << buffer, cpp_redis::redis_error);
}
