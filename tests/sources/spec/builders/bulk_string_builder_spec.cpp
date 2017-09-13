// The MIT License (MIT)
//
// Copyright (c) 2015-2017 Simon Ninon <simon.ninon@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cpp_redis/builders/bulk_string_builder.hpp>
#include <cpp_redis/misc/error.hpp>
#include <gtest/gtest.h>

TEST(BulkStringBuilder, WithNoData) {
  cpp_redis::builders::bulk_string_builder builder;

  EXPECT_EQ(false, builder.reply_ready());
}

TEST(BulkStringBuilder, WithNotEnoughData) {
  cpp_redis::builders::bulk_string_builder builder;

  std::string buffer = "5\r\nhell";
  builder << buffer;

  EXPECT_EQ(false, builder.reply_ready());
}

TEST(BulkStringBuilder, WithPartOfEndSequence) {
  cpp_redis::builders::bulk_string_builder builder;

  std::string buffer = "5\r\nhello\r";
  builder << buffer;

  EXPECT_EQ(false, builder.reply_ready());
}

TEST(BulkStringBuilder, Null) {
  cpp_redis::builders::bulk_string_builder builder;

  std::string buffer = "-1\r\n";
  builder << buffer;

  EXPECT_EQ(true, builder.reply_ready());
  EXPECT_EQ("", buffer);

  auto reply = builder.get_reply();
  EXPECT_TRUE(reply.is_null());
}

TEST(BulkStringBuilder, WithAllInOneTime) {
  cpp_redis::builders::bulk_string_builder builder;

  std::string buffer = "5\r\nhello\r\n";
  builder << buffer;

  EXPECT_EQ(true, builder.reply_ready());
  EXPECT_EQ("", buffer);

  auto reply = builder.get_reply();
  EXPECT_TRUE(reply.is_bulk_string());
  EXPECT_EQ("hello", reply.as_string());
}

TEST(BulkStringBuilder, WithAllInMultipleTimes) {
  cpp_redis::builders::bulk_string_builder builder;

  std::string buffer = "5\r\nhell";
  builder << buffer;
  buffer += "o\r\n";
  builder << buffer;

  EXPECT_EQ(true, builder.reply_ready());
  EXPECT_EQ("", buffer);

  auto reply = builder.get_reply();
  EXPECT_TRUE(reply.is_bulk_string());
  EXPECT_EQ("hello", reply.as_string());
}

TEST(BulkStringBuilder, WithAllInMultipleTimes2) {
  cpp_redis::builders::bulk_string_builder builder;

  std::string buffer = "5\r\nhello";
  builder << buffer;
  buffer += "\r\n";
  builder << buffer;

  EXPECT_EQ(true, builder.reply_ready());
  EXPECT_EQ("", buffer);

  auto reply = builder.get_reply();
  EXPECT_TRUE(reply.is_bulk_string());
  EXPECT_EQ("hello", reply.as_string());
}

TEST(BulkStringBuilder, WithAllInMultipleTimes3) {
  cpp_redis::builders::bulk_string_builder builder;

  std::string buffer = "5\r\nhello\r";
  builder << buffer;
  buffer += "\n";
  builder << buffer;

  EXPECT_EQ(true, builder.reply_ready());
  EXPECT_EQ("", buffer);

  auto reply = builder.get_reply();
  EXPECT_TRUE(reply.is_bulk_string());
  EXPECT_EQ("hello", reply.as_string());
}

TEST(BulkStringBuilder, InvalidEndSequence) {
  cpp_redis::builders::bulk_string_builder builder;

  std::string buffer = "5\r\nhello\ra";
  EXPECT_THROW(builder << buffer, cpp_redis::redis_error);
}
