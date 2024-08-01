#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <print>

#include "proto/hello.pb.h"

using namespace testing;

int main() {
  InitGoogleTest();
  InitGoogleMock();
  return RUN_ALL_TESTS();
}

class protobuf_tests : public Test {
 public:
  void SetUp() override {
    person.set_name(expected_name);
    person.set_id(expected_id);
    person.set_email(expected_email);
  }

  static constexpr std::string_view expected_name { "Igor`" };
  static constexpr int expected_id { 32 };
  static constexpr std::string_view expected_email { "igor@gmail.com" };

  study::Person person;
};

TEST_F(protobuf_tests, simple_usage) {
  ASSERT_EQ(expected_name, person.name());
  ASSERT_EQ(expected_id, person.id());
  ASSERT_EQ(expected_email, person.email());
}

TEST_F(protobuf_tests, simple_serialize) {
  auto f { person.SerializeAsString() };
  study::Person person2;
  person2.MergeFromString(f);

  ASSERT_EQ(expected_name, person2.name());
  ASSERT_EQ(expected_id, person2.id());
  ASSERT_EQ(expected_email, person2.email());
}