#include <gtest/gtest.h>

//! For debugging purpose, uncomment
// #include <memory>
// #include <cpp_redis/cpp_redis>

int
main(int argc, char** argv) {
  //! For debugging purpose, uncomment
  // cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger(cpp_redis::logger::log_level::debug));

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
