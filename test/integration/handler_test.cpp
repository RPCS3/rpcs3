#include "specexamples.h"  // IWYU pragma: keep
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/yaml.h"  // IWYU pragma: keep

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::StrictMock;

#define EXPECT_THROW_PARSER_EXCEPTION(statement, message) \
  ASSERT_THROW(statement, ParserException);               \
  try {                                                   \
    statement;                                            \
  }                                                       \
  catch (const ParserException& e) {                      \
    EXPECT_EQ(e.msg, message);                            \
  }

namespace YAML {
namespace {

class MockEventHandler : public EventHandler {
 public:
  MOCK_METHOD1(OnDocumentStart, void(const Mark&));
  MOCK_METHOD0(OnDocumentEnd, void());

  MOCK_METHOD2(OnNull, void(const Mark&, anchor_t));
  MOCK_METHOD2(OnAlias, void(const Mark&, anchor_t));
  MOCK_METHOD4(OnScalar, void(const Mark&, const std::string&, anchor_t,
                              const std::string&));

  MOCK_METHOD3(OnSequenceStart,
               void(const Mark&, const std::string&, anchor_t));
  MOCK_METHOD0(OnSequenceEnd, void());

  MOCK_METHOD3(OnMapStart, void(const Mark&, const std::string&, anchor_t));
  MOCK_METHOD0(OnMapEnd, void());
};

class HandlerTest : public ::testing::Test {
 protected:
  void Parse(const std::string& example) {
    std::stringstream stream(example);
    Parser parser(stream);
    while (parser.HandleNextDocument(handler)) {
    }
  }

  void IgnoreParse(const std::string& example) {
    std::stringstream stream(example);
    Parser parser(stream);
    while (parser.HandleNextDocument(nice_handler)) {
    }
  }

  InSequence sequence;
  StrictMock<MockEventHandler> handler;
  NiceMock<MockEventHandler> nice_handler;
};

TEST_F(HandlerTest, NoEndOfMapFlow) {
  EXPECT_THROW_PARSER_EXCEPTION(Parse("---{header: {id: 1"),
                                ErrorMsg::END_OF_MAP_FLOW);
}

TEST_F(HandlerTest, PlainScalarStartingWithQuestionMark) {
  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "?bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse("foo: ?bar");
}

TEST_F(HandlerTest, NullStringScalar) {
  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnNull(_, 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse("foo: null");
}
}
}
