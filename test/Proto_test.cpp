#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "send/FileSendControl.h"

class TestProto : public testing::Test
{
protected:
  TestProto() {
    proto.set_type(Proto::kData);
    proto.set_file_data_len(10);
  }
  Proto proto;
};


TEST_F(TestProto, getset) {
  EXPECT_EQ(Proto::kData, proto.type());
}



int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);
  
  return RUN_ALL_TESTS();
}
