#include <gtest/gtest.h>
#include <cpp_redis/builders/simple_string_builder.hpp>
#include <cpp_redis/builders/reply_builder.hpp>
#include <cpp_redis/builders/array_builder.hpp>
#include <cpp_redis/builders/integer_builder.hpp>
#include <cpp_redis/builders/bulk_string_builder.hpp>
#include <cpp_redis/builders/error_builder.hpp>
#include <cpp_redis/redis_error.hpp>

TEST(ReplyBuilder, WithNoData) {
    cpp_redis::builders::reply_builder builder;

    EXPECT_EQ(false, builder.reply_available());
}

TEST(ReplyBuilder, WithNotEnoughData) {
    cpp_redis::builders::reply_builder builder;

    builder << "*1\r\n";

    EXPECT_EQ(false, builder.reply_available());
}

TEST(ReplyBuilder, WithPartOfEndSequence) {
    cpp_redis::builders::reply_builder builder;

    builder << "*1\r\n+hello\r";

    EXPECT_EQ(false, builder.reply_available());
}

TEST(ReplyBuilder, WithAllInOneTime) {
    cpp_redis::builders::reply_builder builder;

    builder << "*4\r\n+simple_string\r\n-error\r\n:42\r\n$5\r\nhello\r\n";

    EXPECT_EQ(true, builder.reply_available());

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

TEST(ReplyBuilder, WithAllInMultipleTimes) {
    cpp_redis::builders::reply_builder builder;

    builder << "*4\r\n+simple_string\r";
    builder << "\n-error\r\n:42\r\n";;
    builder << "$5\r\nhello\r\n";

    EXPECT_EQ(true, builder.reply_available());

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
