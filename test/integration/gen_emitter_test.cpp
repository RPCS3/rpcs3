#include "handler_test.h"
#include "yaml-cpp/yaml.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;

namespace YAML {
namespace {

typedef HandlerTest GenEmitterTest;

TEST_F(GenEmitterTest, testbf4e63edf2258c91fb88) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8c2aa26989357a4c8d2d) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf8818f97591e2c51179c) {
  Emitter out;
  out << BeginDoc;
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2b9d697f1ec84bdc484f) {
  Emitter out;
  out << BeginDoc;
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test969d8cf1535db02242b4) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4d16d2c638f0b1131d42) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3bdad9a4ffa67cc4201b) {
  Emitter out;
  out << BeginDoc;
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa57103d877a04b0da3c9) {
  Emitter out;
  out << BeginDoc;
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf838cbd6db90346652d6) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << "foo\n";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste65456c6070d7ed9b292) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << "foo\n";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test365273601c89ebaeec61) {
  Emitter out;
  out << BeginDoc;
  out << "foo\n";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test92d67b382f78c6a58c2a) {
  Emitter out;
  out << BeginDoc;
  out << "foo\n";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test49e0bb235c344722e0df) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << "foo\n";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3010c495cd1c61d1ccf2) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << "foo\n";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test22e48c3bc91b32853688) {
  Emitter out;
  out << BeginDoc;
  out << "foo\n";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test03e42bee2a2c6ffc1dd8) {
  Emitter out;
  out << BeginDoc;
  out << "foo\n";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9662984f64ea0b79b267) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf3867ffaec6663c515ff) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfd8783233e21636f7f12) {
  Emitter out;
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3fc20508ecea0f4cb165) {
  Emitter out;
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste120c09230c813be6c30) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test835d37d226cbacaa4b2d) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7a26848396e9291bf1f1) {
  Emitter out;
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test34a821220a5e1441f553) {
  Emitter out;
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test53e5179db889a79c3ea2) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb8450c68977e0df66c5b) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste0277d1ed537e53294b4) {
  Emitter out;
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd6ebe62492bf8757ddde) {
  Emitter out;
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test56c67a81a5989623dad7) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testea4c45819b88c22d02b6) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfa05ed7573dd54074344) {
  Emitter out;
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test52431165a20aa2a085dc) {
  Emitter out;
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2e1bf781941755fc5944) {
  Emitter out;
  out << Comment("comment");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5405b9f863e524bb3e81) {
  Emitter out;
  out << Comment("comment");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0a7d85109d068170e547) {
  Emitter out;
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testba8dc6889d6983fb0f05) {
  Emitter out;
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd8743fc1225fef185b69) {
  Emitter out;
  out << Comment("comment");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc2f808fe5fb8b2970b89) {
  Emitter out;
  out << Comment("comment");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test984d0572a31be4451efc) {
  Emitter out;
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa3883cf6b7f84c32ba99) {
  Emitter out;
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1fe1f2d242b3a00c5f83) {
  Emitter out;
  out << Comment("comment");
  out << "foo\n";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test80e82792ed68bb0cadbc) {
  Emitter out;
  out << Comment("comment");
  out << "foo\n";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6756b87f08499449fd53) {
  Emitter out;
  out << "foo\n";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7d768a7e214b2e791928) {
  Emitter out;
  out << "foo\n";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test73470e304962e94c82ee) {
  Emitter out;
  out << Comment("comment");
  out << "foo\n";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test220fcaca9f58ed63ab66) {
  Emitter out;
  out << Comment("comment");
  out << "foo\n";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7e4c037d370d52aa4da4) {
  Emitter out;
  out << "foo\n";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test79a2ffc6c8161726f1ed) {
  Emitter out;
  out << "foo\n";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2a634546fd8c4b92ad18) {
  Emitter out;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test84a311c6ca4fe200eff5) {
  Emitter out;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testef05b48cc1f9318b612f) {
  Emitter out;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa77250518abd6e019ab8) {
  Emitter out;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3e9c6f05218917c77c62) {
  Emitter out;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7392dd9d6829b8569e16) {
  Emitter out;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8b3e535afd61211d988f) {
  Emitter out;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa88d36caa958ac21e487) {
  Emitter out;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0afc4387fea5a0ad574d) {
  Emitter out;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6e02b45ba1f87d0b17fa) {
  Emitter out;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1ecee6697402f1ced486) {
  Emitter out;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf778d3e7e1fd4bc81ac8) {
  Emitter out;
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testce2ddd97c4f7b7cad993) {
  Emitter out;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9801aff946ce11347b78) {
  Emitter out;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test02ae081b4d9719668378) {
  Emitter out;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1c75e643ba55491e9d58) {
  Emitter out;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1388c6b2e9ed23c46e83) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0d92d3471da737a6632d) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test65a86b4d0f234874b5d7) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa9576676ee9dbef13c65) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test65040071850bfa32b9bb) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testef8dbacc95c3dff2ddfb) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7e21a99d0a50a0c87aaa) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test782795d5876c20ea9560) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test79db87f2940dae93d293) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb591251ebb8bb8b92258) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0c34d5a0721e22357981) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfc28a7e273d9856577e7) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbafbc41a4c0c940ee33a) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6f75b0fefb0f648e60dd) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test46bb97a4ba469982a901) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test490ae5b7a9d137e370f2) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd6235109a3be4cdc7848) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb76432dc7369179a6a43) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testaecc7e5d083d040bcc53) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc57095da3bc936594c0a) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test91199f826c7cf3acac29) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test555cb7ab09c7cef0935b) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7f7cc35b00a3709ab727) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testba758c2f91b6eadd6d03) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1a00c84b1c11e51a94db) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testeb2e5389f63c6e815e3c) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test349a326aa4e8afb218bb) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3eea4f22fc0d36cc0d98) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test14f4244498cca9dc40c3) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste1f90f88f842640d459d) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test702d257adfc8ecf33cd0) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste4b0fc1c9cc9fc7f5ace) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbd571e48d96a29594458) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2f3118b8a385b882ba82) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0a94d23d00e619349f8f) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9cf01d11d8c136fd56a7) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test92b15a6c3db86f6679df) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4544465792a8e637b1a4) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8e41e2a7fe27a766fb48) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2bdf003438908d3702b2) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf9b093be4bd2943872db) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test248191adffc94e17ebfa) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7714a2e9abd0581a0bd0) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdbed9b5242af91e93a49) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd05375283b255ad9b193) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test25672169587d28236247) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf6ab8d0952ac8ed5c70b) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8fca31b84d85277abf69) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc9d01018d1f62e39c123) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb6deabb10a45ddc007b4) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd1b1ae28527d505d42d1) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfcd8f743d719a2a14b95) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test752b27902e4ac50dcffe) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test090ff016a7c40492ec56) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc1bb724073020b11d8cf) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9bfb359c967a13c58bb1) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb99473af08e43d8f318b) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9e0e3a4a056479cdf5b2) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testae8e3805462a6201d1b3) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3f49e839e34b1fc62edd) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf62b452f38efa0d8fda9) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testac563141c91715286936) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbe27048505799389b8e2) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test054d89fbd98eee64ec9a) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testba157731bf174c6c2724) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test00c04983145e858e0410) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test61632267d3708bd0d91a) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5d4043bea55d986865a4) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb402a88610a0ab1a2034) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb9bf03751fa3e4102569) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test059687500c7600e9c66b) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8b157ca7ebd6986c8742) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testac6d6c91f2bef452fe7f) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb1a6c74b6fd59875a995) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testabc8e0c151b055815e62) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb8ee84933deeda83e8dd) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9e845641cf55a4ea7aa4) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9aa6fef396957e49fec7) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6ff511b9e4beb8c7aab8) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testad01e404e04d9c511e2c) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test17dee50addd90f03043e) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test31ea32f235e6c23ec194) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5a2fcadc2d7a5b2865c0) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1291832147125e008a57) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test755e49db22ed175e295e) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test533f4a4db1643c737140) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1dd22ff508c30755be6d) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7050780cfb3f1580ce6f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc3aaa4ee24316655f918) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7c00f6d0c51e3a19331a) {
  Emitter out;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb1da2c1c827ab9d13a87) {
  Emitter out;
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test405617086a61392c11b1) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test749f6d187854181ccaf7) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test44d22d3cbdffd570e6c2) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa0f0c67d0fcc1a74cba7) {
  Emitter out;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa10d0468c5fd6dd54d45) {
  Emitter out;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test44b719160a85c946dbc3) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcba2dda5bbc13494145d) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3790fc28d67c5d9e1b43) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test55991ca477463fd588a3) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa89a0c4f338cd64c5b7b) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8b08b7f2456818355765) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbe45f53229761d1358dc) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf3647e65db7aaa57e4c9) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test258756f063c9b4007ec3) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test12b870b421bc720a0843) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2b479a5420a812f41324) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test45161f7b1f8965a5c770) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test341a392f5fd01408b6ff) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test652fc4c8a843452a2338) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testaee3d9d53287a2200657) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbd0ec1bddba9abe76b10) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test38120b0421f0057272c9) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test974e0ea82099e2f8937c) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test908b38265f2f57856e1d) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3083893fff705d49c013) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test96a6c135a24157354bb9) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6d8f0ff03921a5b3b9f2) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb0f6de3f6038f6853cc1) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfbd34bd50cd03ed39dcf) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test231a8fa27b2674173db1) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9ca5970eb21e99ea37db) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdf7b1481c534136d9b06) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf3de15177d2aaa7f230a) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0f3d2bd45bf4a7f4c67b) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd4a2e6e1c8642958c24a) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7421c2b3ed152054e934) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0871fddad017ba60c9c0) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test139eda3a09e31c851c9e) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa3d2512774f92f4df087) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test333b5a1357622401602c) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test84bb5b58a848a234996e) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test462fbeed108fc6796aa8) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test604f0fd657a536f57d5f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test414c3ac71381964d0864) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa6f58087b9ffcd19fed0) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test17328cbd9f86b4832607) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5eef6c8c9fe8dd575341) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test782fa8630250003779ac) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test59a32d15e9937062b85f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test22799c884410a4270dd2) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4c2cfc1a0c9855fbaea0) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0f81ed80de482457cfee) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testca42eaff1cc9bced6434) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5aef4463ea4673ccd05b) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7dad3f52b3b26610c7af) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3ce2ff995e77cc9ba600) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc6537cded30f5bd190a9) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test47e22348a94cf3f87746) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf2fd387cfe6c7b9a9e1d) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test64855486556216400238) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa63343df16ba399c2388) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb3aa83206fa80c469cfe) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0ed1c6165c70a766c537) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7c88ccfe3cef5e463913) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test226fd5b2a4320ef39d5d) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdb0f5c07db2e6dee481a) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9cfc0c19d01d1b819aa1) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test337da951ee48100f3d8f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd058933aee922e78dfe8) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcb4874993891eddbab00) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0378531d8172f6518512) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test94160a7ce84f9cf0a7d5) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test534a4487e6174fdcaac5) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc00247c63b3bfe18c812) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7f95256142a04b96f28f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9a6d4b75ee3036ece870) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0002ad9101a070ddf075) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test197e848680c81c553903) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testaff214cf5bb82018af40) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test50571f18c1c33f39de2f) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test42d173eb53ac949bb861) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa20657b13d154530ecb9) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf9a22d2110b473ab80af) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testefe13915505275269311) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test71d94ecf19640983cf1b) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfffc4985596873566ea8) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1ae4cf1e0d5d59ad7b41) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1a6995dafb0016446725) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testddc252fbed9724302791) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3c3ea7e37733ad857c78) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test011aeaeafe274b32cc05) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test21f19241e5664b5906e5) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa22e1d75d7ff7f7597af) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7913865b19109fd8ff2e) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa32d8694b91f4cb68964) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test20bec09b8a4ebe9b82a6) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test89b6c77694f437f024ef) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test014b3b395bb52d6570ac) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4cbb61c9466c0f38617a) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test14e839f1cb4575c45177) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbd9c064b4f7e0c6ce694) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5901d177c4074cea6d9b) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0723bdb909902bb29f50) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test087c28c86195301ec009) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7ddf7ab201c5b305f80f) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test73445b0773a971263d89) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testeebc7fd34ba182ea9273) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test41ca8f20940734df23e3) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test20e9205a36290de1b198) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test63edeee948dde2dd9ba1) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test68882a5e1cdb1616032d) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test058d6612598f319499b9) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4101f386851b7ffd6eff) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd76795158db2c713c986) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa82eaac99c6367e7c3df) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf32e32f85b171c885c9a) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test09038dae2c9a44090c0e) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste001d05e377e097922dd) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9bf490ab4615624e3ff5) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd2d34be15e181fa05e42) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test91f016f21845218780cd) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6a9eb1ccae866e321278) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test69ea8c37d5de2eba3061) {
  Emitter out;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test22d346c518958ed0a29a) {
  Emitter out;
  out << BeginMap;
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test520f93c7e63ea59f0d71) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdccf42e98d8562f48cd6) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test62a3a162830f497a90b9) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1db0d7b0c4bf3847c314) {
  Emitter out;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf7c6c2383d90d1ef2d16) {
  Emitter out;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste68538246feafad55bd1) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1eb459c7870ad4b6af1d) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test69fef2a4c8befa21d938) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6ca29a629d9ac3c1d429) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste5b9c085fbd8d19424eb) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd514def6019ec1590873) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test312a85739c1f38726feb) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test68224aa149dedecd616a) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2acbc29e623d0d9738ff) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste83e77ed14fddf76e32c) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5a304023afbcdacd9af8) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf5feead651be88058e69) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test799fc8df4a2e359e7b3c) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb79b98fc8a3bad292a74) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0b3b3a115ca8e5458d79) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9d8c156c756e00a1baaa) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test22df3676be3423b00cce) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6b827dbc448d858a2507) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test20cdbf3bbe7ce47687a7) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc6e5c8f6a76d302c30d0) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2f4853b3fde26ce53291) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0f4a7a9427fd9d228329) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbda74fd491ce9c1a417d) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test850815a7fe9fbf84dcda) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test962a1cbd02405d0fbf5c) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test80e6eeb2db9ea8a29ad1) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf961c58f156c0d10821a) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testba8440cb470b66c37bd2) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbffeb1e52a131b2e3508) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0c4e000433a577f9a526) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test91282cd432be1ce942e1) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa50f78d08f6482283841) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3c2b2d644ae5467c04aa) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbfb73120a92a3c64bbad) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7e23c1ca886c84b778ab) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7ad0518f9806b1d723ca) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf716871ee0dab143fbc5) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb122634768db80f9b9d8) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1dbff921053d94a4b9cb) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testccfdcbf942a0da49c625) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testabb3a9f113a4ebf6347e) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8344e9d0a0d4f1594b07) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc9997291841b4ea82bf6) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9d326a920c254469b813) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5e33b8376d0a3d453978) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1137327a599e536d4b78) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test747b3661df5656db5955) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9c85ac2c1b43334f86ea) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test38377dd35c453655b27f) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc6410eea08971f4a53a1) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa9e111bc3cb37afa64f1) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5b28739a8a003fc4d712) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4a08ec448ec40323fedb) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test304b0aa68b3caecbe8ea) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2613f9bddf3460d2d580) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test14dc01dfaf34ae324af0) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfc00f5da9940c82704b7) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testeb2bf19f2053bd67b4bd) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test40aaa2f91df1e390a9c5) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5faee7b690291bcaedfb) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test36d90583e16f64f5416d) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc163d005ba76d202e60c) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd47c01370591023c86d3) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test90f2506a73bd181d4a84) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test705fae7986640be9ebc5) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfa7f68a4ad0fdfd9ccc2) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test938b5359e11b5237f248) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test76398b4e5b861c69ebbb) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7cd2e0d05cb04ec47539) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4b9b77f310a4588113d1) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test274595fce326fd796656) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test073251446eba0e22da3c) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test49016a5ab8b3a0bb0877) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste5a16ae97d5bc41be66a) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1a0f7a3b9c21558ec6cc) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5c423c12ce0cbad0d994) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0e77007746f8284bf96e) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testee86aaef5ce0e25239cd) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test72a934d8ca89413d837e) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test24bf3393cf5de8010f23) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test83b5a3c8048b96cc1b2c) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf478f9a36dd3ffe5a09b) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa84e5292c74875c5a5b2) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test151d960aeab73bef0f8f) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test96637d371edf8e0331c3) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8ddda6479de49ddc21dc) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test79d2ad4a2ef977101904) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test29102010b150435fa0b5) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9d484df95a4a18093f26) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4f0d5a440b454d02ca5e) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste829fb10d0562956a78c) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test499a932b32ee03f7f798) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8943c507b29eeac72d21) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste1778f6ee55f14e784fb) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test657f3e6c808e4fdd27d5) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf69955026db899442c06) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcc393a3f138b664269b3) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testab7c512ac51cc93465e8) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb9ab8d54773a44286fb5) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test66c34aad666e269893a0) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test50ee9d1d4e9aa514728f) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa587b5a3f18d6dd21f15) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3a3d0562363d7803e6c9) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb94a56cc3c5fa88ae31a) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testae08e0157227234c7c1d) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb8af095dec19d2b7eba8) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8a31a82a9e569684e96a) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test03620d515ce953c16510) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5f2112c33e87bf2d20f2) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfe099be6bd04691b8c94) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test46cf13e6ae9fe4a76ab7) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa82c59e1c3525732c2e1) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3e1f40e243f8b26e4fd5) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7890610150235df252f2) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test29d80fb3f1cdfd5abf10) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste6b4fdd5e7e3ee51dec5) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc04ea134ed646e19ea11) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test08cbb44e3f0062df827d) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb4b508df5857a4fd0ec0) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4c849a0ccb52f3965fb3) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa986347438ae551cfbbb) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test84aaa650012405b9705f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc755c4903a517819fc24) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1d82c377e71999c6415e) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test670efbf25f862e35d969) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf3372e609891208dbd26) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test74e7b734b89ba72d0e97) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test84217fc182966092b104) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb9068c92ad6100abdc72) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9bfffb392a9fdd206523) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfc7f0207790c23361cb4) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testddff02d8eee993041355) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test51dfb937f1dbad9b3616) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test50fd3ab7159626f0ee3a) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1c523ae47e99f5e6f917) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test853576a927891dfadde8) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5f5cffcc07d4e419fd83) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test49f37e02fdf786148f58) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test77aec79c11d77d3999fd) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2f8f1d320eb09567c52d) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test161b89230d67e4bd30bc) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test62b2ed6542a97e0b0cc4) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3ff52242e1a3e0ec1c09) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0a4893e5a14ffff98b17) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf9acc9f113335ca40d54) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcb351ce91b012793194f) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9b35c085c3689a6f880c) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb3171005f7df07c59728) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd1123327075e1a24f2f6) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5c5834ef11daf0579c5a) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test99c7fc5914fa416060e5) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste72905e6bfdb60e4fcd0) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test72319a3635f29c53d265) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6e3f686e25a76f66dbb5) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9e614e875da8230262bc) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3e0d72fa669c82c554b3) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste8f4fd7fad0b68a588f8) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test268018dc9c53472df765) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1d8111d3ca2885b8df98) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3c396b312379ee8e6fec) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6d434ed19d74e948101a) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1c394f04e8b70f2fc4bf) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5e81a07fa400b500b282) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc26d94533661b104d11e) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5b31f044eceae59309ca) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc4362afe04cd9003ca2e) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf4c8b0374bf3a9d6a04d) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test024a940092fc779c3d34) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd7325119d9a206dae44f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7075805dde951b54b673) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbbd5b9aa15dd533c26ff) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test22fad936665887d43ff7) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testeb8989899dcc86b54ea8) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testba45473b01a5781f09e1) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test27c4890334049fab5e11) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testca5e85e2413efdb0079c) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test723865c5c5e9119078b3) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc3f8ed87aca4598e3687) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3722c73474afce54e8ff) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test09e39e0ed889eb88438f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test316759e4b046d96e777f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfdd08ff673f4641686b4) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8fcd6af968be17ba5c2e) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf1249e2d68d1ad3254c9) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9f942ee23206dd6b1c10) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2074ea1cf3e6cc46b873) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test265abc9077357c8856e1) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test69b53b8afa9898148e35) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test65f02a13b5c2cd8a5937) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test006123ad6adbc209d0f9) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc00388f82f797b224bf3) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5be9ca608909e56a5019) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test70a410628c94b8c6b867) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1c8841fa392f227c4356) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test84d12b33d6e4ac16309d) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test20fd961ce7bfc3a008fc) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste104b6dfca74d6e13383) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4b5645aaf03bf8ea8e75) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test51125b72703a23899993) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test53308c1e6a138d09b71f) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test15468293cb07b4f27c01) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test78b5500f59b119dd1822) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdf605e22429ef3b09958) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc35717ed7567daa8a957) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf3178a4cc7928f4dc98e) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test43491189b8ec80c4bb33) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcb44b281e0479ee43187) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test678d266f1779eb2c784d) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test15a1494eea17258857eb) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa37dc56777b71d029e3a) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4b896f9a5461502ed1f6) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1e209a78672bc1f67c53) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdf635543851a224c92c4) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test19e33bce2d816b447759) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}
}
}
