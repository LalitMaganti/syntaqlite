// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include "gtest/gtest.h"

namespace syntaqlite {
namespace {

TEST(SmokeTest, BasicAssertions) {
  EXPECT_EQ(1 + 1, 2);
  EXPECT_TRUE(true);
  EXPECT_FALSE(false);
}

TEST(SmokeTest, StringComparison) {
  std::string hello = "Hello";
  EXPECT_EQ(hello, "Hello");
  EXPECT_NE(hello, "World");
}

}  // namespace
}  // namespace syntaqlite
