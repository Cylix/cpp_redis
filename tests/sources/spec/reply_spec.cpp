#include <cpp_redis/redis_error.hpp>
#include <cpp_redis/reply.hpp>
#include <gtest/gtest.h>

TEST(Reply, NullReply) {
  cpp_redis::reply r;

  EXPECT_EQ(r.is_array(), false);
  EXPECT_EQ(r.is_string(), false);
  EXPECT_EQ(r.is_simple_string(), false);
  EXPECT_EQ(r.is_bulk_string(), false);
  EXPECT_EQ(r.is_error(), false);
  EXPECT_EQ(r.is_integer(), false);
  EXPECT_EQ(r.is_null(), true);
  EXPECT_EQ(r.ok(), true);
  EXPECT_EQ(r.ko(), false);
  EXPECT_THROW(r.error(), cpp_redis::redis_error);
  EXPECT_EQ((bool) r, false);
  EXPECT_THROW(r.as_array(), cpp_redis::redis_error);
  EXPECT_THROW(r.as_string(), cpp_redis::redis_error);
  EXPECT_THROW(r.as_integer(), cpp_redis::redis_error);
  EXPECT_EQ(r.get_type(), cpp_redis::reply::type::null);
}

TEST(Reply, Error) {
  cpp_redis::reply r("some error", cpp_redis::reply::string_type::error);

  EXPECT_EQ(r.is_array(), false);
  EXPECT_EQ(r.is_string(), true);
  EXPECT_EQ(r.is_simple_string(), false);
  EXPECT_EQ(r.is_bulk_string(), false);
  EXPECT_EQ(r.is_error(), true);
  EXPECT_EQ(r.is_integer(), false);
  EXPECT_EQ(r.is_null(), false);
  EXPECT_EQ(r.ok(), false);
  EXPECT_EQ(r.ko(), true);
  EXPECT_EQ(r.error(), "some error");
  EXPECT_EQ((bool) r, false);
  EXPECT_THROW(r.as_array(), cpp_redis::redis_error);
  EXPECT_EQ(r.as_string(), "some error");
  EXPECT_THROW(r.as_integer(), cpp_redis::redis_error);
  EXPECT_EQ(r.get_type(), cpp_redis::reply::type::error);
}

TEST(Reply, BulkString) {
  cpp_redis::reply r("str", cpp_redis::reply::string_type::bulk_string);

  EXPECT_EQ(r.is_array(), false);
  EXPECT_EQ(r.is_string(), true);
  EXPECT_EQ(r.is_simple_string(), false);
  EXPECT_EQ(r.is_bulk_string(), true);
  EXPECT_EQ(r.is_error(), false);
  EXPECT_EQ(r.is_integer(), false);
  EXPECT_EQ(r.is_null(), false);
  EXPECT_EQ(r.ok(), true);
  EXPECT_EQ(r.ko(), false);
  EXPECT_THROW(r.error(), cpp_redis::redis_error);
  EXPECT_EQ((bool) r, true);
  EXPECT_THROW(r.as_array(), cpp_redis::redis_error);
  EXPECT_EQ(r.as_string(), "str");
  EXPECT_THROW(r.as_integer(), cpp_redis::redis_error);
  EXPECT_EQ(r.get_type(), cpp_redis::reply::type::bulk_string);
}

TEST(Reply, SimpleString) {
  cpp_redis::reply r("str", cpp_redis::reply::string_type::simple_string);

  EXPECT_EQ(r.is_array(), false);
  EXPECT_EQ(r.is_string(), true);
  EXPECT_EQ(r.is_simple_string(), true);
  EXPECT_EQ(r.is_bulk_string(), false);
  EXPECT_EQ(r.is_error(), false);
  EXPECT_EQ(r.is_integer(), false);
  EXPECT_EQ(r.is_null(), false);
  EXPECT_EQ(r.ok(), true);
  EXPECT_EQ(r.ko(), false);
  EXPECT_THROW(r.error(), cpp_redis::redis_error);
  EXPECT_EQ((bool) r, true);
  EXPECT_THROW(r.as_array(), cpp_redis::redis_error);
  EXPECT_EQ(r.as_string(), "str");
  EXPECT_THROW(r.as_integer(), cpp_redis::redis_error);
  EXPECT_EQ(r.get_type(), cpp_redis::reply::type::simple_string);
}

TEST(Reply, Integer) {
  cpp_redis::reply r(42);

  EXPECT_EQ(r.is_array(), false);
  EXPECT_EQ(r.is_string(), false);
  EXPECT_EQ(r.is_simple_string(), false);
  EXPECT_EQ(r.is_bulk_string(), false);
  EXPECT_EQ(r.is_error(), false);
  EXPECT_EQ(r.is_integer(), true);
  EXPECT_EQ(r.is_null(), false);
  EXPECT_EQ(r.ok(), true);
  EXPECT_EQ(r.ko(), false);
  EXPECT_THROW(r.error(), cpp_redis::redis_error);
  EXPECT_EQ((bool) r, true);
  EXPECT_THROW(r.as_array(), cpp_redis::redis_error);
  EXPECT_THROW(r.as_string(), cpp_redis::redis_error);
  EXPECT_EQ(r.as_integer(), 42);
  EXPECT_EQ(r.get_type(), cpp_redis::reply::type::integer);
}

TEST(Reply, Array) {
  cpp_redis::reply r_arr_1(42);
  cpp_redis::reply r_arr_2("str", cpp_redis::reply::string_type::simple_string);
  cpp_redis::reply r({r_arr_1, r_arr_2});

  EXPECT_EQ(r.is_array(), true);
  EXPECT_EQ(r.is_string(), false);
  EXPECT_EQ(r.is_simple_string(), false);
  EXPECT_EQ(r.is_bulk_string(), false);
  EXPECT_EQ(r.is_error(), false);
  EXPECT_EQ(r.is_integer(), false);
  EXPECT_EQ(r.is_null(), false);
  EXPECT_EQ(r.ok(), true);
  EXPECT_EQ(r.ko(), false);
  EXPECT_THROW(r.error(), cpp_redis::redis_error);
  EXPECT_EQ((bool) r, true);
  auto arr = r.as_array();
  EXPECT_EQ(arr.size(), 2);
  EXPECT_EQ(arr[0].is_integer(), true);
  EXPECT_EQ(arr[0].as_integer(), 42);
  EXPECT_EQ(arr[1].is_simple_string(), true);
  EXPECT_EQ(arr[1].as_string(), "str");
  EXPECT_THROW(r.as_string(), cpp_redis::redis_error);
  EXPECT_THROW(r.as_integer(), cpp_redis::redis_error);
  EXPECT_EQ(r.get_type(), cpp_redis::reply::type::array);
}
