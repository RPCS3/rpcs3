#ifdef YAML_GEN_TESTS
namespace Emitter {
TEST test02571eee35ac0cbd3777(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test71b969ca18898d226320(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testd69e4ea95ce6f221c6e7(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testffbfd295ad9bef4deb00(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo";
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test6a459b2fe1f6e961e1a7(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test869ab95640c9933ed4d6(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test433c0771f40ac3ba853e(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST teste181778974c4003bc5a4(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testf8cb7e3f1b11791f53b8(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << "foo\n";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test3c48ed06807100f0a111(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << "foo\n";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST testb13f7b031f425b0e383f(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo\n";
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST testb77284234d3fbe8b24a0(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo\n";
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test9c56fd285b563327a340(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << "foo\n";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test1c08639d56176e64c885(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << "foo\n";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test94c8742f8cab3cec1b4a(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo\n";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test79d1806ceb3ecebfa60b(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo\n";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test360afe50348ec36569d3(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST teste599b3fc1857f4265d3b(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test88adf7adb474ad063424(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test0978ca6f6358ea06e024(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testd915f57fca4b0f6d77b4(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::VerbatimTag("tag");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test1fe1f22496f2a0ffd64e(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test76422a4077d3bdd03579(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test92b168a497cb0c7e3144(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testa93925b3ae311a7f11d4(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test2dd1aaf6a1c1924557d0(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test6ec0585d0f0945ad9dae(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test7e00bca835d55844bbfe(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test70912c7d920a0597bbb2(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::Anchor("anchor");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test16eacbf77bccde360e54(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test556e0c86efb0716d2778(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test71b64326d72fe100e6ad(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test0c7bb03fbd6b52ea3ad6(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testb819efb5742c1176df98(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test1f7b7cd5a13070c723d3(YAML::Emitter& out)
{
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test32126a88cb2b7311e779(YAML::Emitter& out)
{
    out << "foo";
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testd7f952713bde5ce2f9e7(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test5030b4f2d1efb798f320(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testb9015537b9a9e09b8ec8(YAML::Emitter& out)
{
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test03229f6d33fa9007a65d(YAML::Emitter& out)
{
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testf998264dcfd0dba06c0a(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << "foo\n";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test7defadc52eddfbf766aa(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << "foo\n";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test55a7c58211689c7815b2(YAML::Emitter& out)
{
    out << "foo\n";
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST testc3873d954433175e0143(YAML::Emitter& out)
{
    out << "foo\n";
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test81fb6bf1f976e0ad3fba(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << "foo\n";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test29e7ff04645f56a7ea2f(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << "foo\n";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST testce0089a55f926d311ff4(YAML::Emitter& out)
{
    out << "foo\n";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST testd1d301bbc73ec11cd49b(YAML::Emitter& out)
{
    out << "foo\n";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test4640bfb42711b7209ef9(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test1133d19fc3a4ec9fb3e8(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test4a6d083241180899f7ed(YAML::Emitter& out)
{
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testbadb5b228a4db78efac0(YAML::Emitter& out)
{
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test5c6d607ed1ad046568e1(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testac34cde109884bb6876b(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test5c19597d5336d541f990(YAML::Emitter& out)
{
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test39e174ed33d5508a61ce(YAML::Emitter& out)
{
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test43e26cf94441cee4a0c4(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test06afa8e5b516630fc8da(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test4d109db0282a7797cdcb(YAML::Emitter& out)
{
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test0c1c00113c20dfa650a9(YAML::Emitter& out)
{
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testccd7f2183f06483ee5e0(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST test52e25e363a17f37c296f(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testdd81d16d3bdd8636af16(YAML::Emitter& out)
{
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST testd76e8eb5043431c3434e(YAML::Emitter& out)
{
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_DOC_END();
    DONE();
}
TEST teste03be55aff3dc08f07a1(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8ba3e94c45f696c5027b(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test90e45bcf67b89e31fd12(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0b7c4e535bfebfa3c85d(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3a1c08f490683d254cda(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test926cd343d3999525d9ce(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1e5dd6b1e4dfb59f0346(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0f064b30c1187ff2dd4b(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test58fd20c1736964e12b53(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testbddd187d973b632dc188(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5bc168e49ab7503fd2e1(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test712e139d81db4e14196d(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test48130bfdef5d192b888e(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test27552de54635da852895(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6529691d17594d5abeb6(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test46b8a7b9d2461d80e0c0(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testbd1cb845de056e97a301(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testfe1f94c842b37340db76(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste3c7fbf8af4d82e891e3(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd6800d90d6d037d02ace(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testdf987230fa431b7a8f1b(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testcd22b774448a8b15345e(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test591a370a1ce302d23688(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5e56c5800a9f2c4591ff(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test72f6d368cc2f52b488bd(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6786da34791cbab71591(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6d53136f35632180e2e8(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test01020a01f84721d7fb07(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testc3ab223703ef17e47ec7(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test033db6218db214ae5ef9(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test38fe09343ac97f51b38f(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test99eb29693d619703a052(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste9e58998a49132e15fb4(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testc1b4fba9280329b30583(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2f8651438d44de183d22(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4c506bf0bc7a972cb62d(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6a42214b8698a0e87f5f(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste0b5020ccbc0cbc7f699(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3fb453ac1de7a2d37a16(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4d3236ecd88c5faa74e8(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test116230234c38c68eb060(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb7c63d73350b11bf4a56(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test26ae0a3b97fb1c7743bf(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testeabf01d5500c4f5c9de5(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf9195cd2fb4c57783870(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8c7159f70888a6c5548e(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testba96c9cdf5e82c9ebd0f(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6cbb2232cc1e43752958(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2bc126cc8be9e3d0a5bb(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb053b9b6ee7c7eecc798(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testda8339179085c81ac7a9(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf38e7a065a9bda416bf0(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5345180f2a8a65af5b72(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7aee5968853276b78e65(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1c20b15f6680fd1fa304(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3ad355d8aa94a80ed9d0(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test717b89ec9b7b004e5c17(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testc540c8d6d92913953ef8(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test805391f6964c07b1fc51(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testafc978dbd9b5d8005968(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf137897e42e659d45548(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test41c10a5f012922d6d240(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6d46af9b0e1bab6eefd2(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6908c1e71bca5b5a09b6(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test158d6160ee94d7f929c0(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test07186666318de7b13975(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testdcb6011d1dbc47e024b4(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test998e6b11c32e19f91a43(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7b009b420c34f753e2dc(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testba4c5b4eedf23c16ab44(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test022d120061a5c77c6640(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testeb346f4b70732835631f(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testade70114082f144726ee(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf2b68aae6173ab6ad66d(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste9a962c26a72ea4d3f8d(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa9603ff993f8a8d47b5d(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2b70cf1579b37e0fb086(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3790d77103bac72dc302(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testadfa25cb2fd5a9346102(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9764ad298ba1fe9ecfa8(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test22f20fac5e02211edadc(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8b3b22c8ffd679b15623(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test438d1581dec9088389d7(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0b417297c8e11f038c7c(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa3686f0b87652d4640c2(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd6f021791f2162c85174(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3d34018a42371ab9fbcd(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7a9287f053731d912e63(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa9aa047a659d330a4a8b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test74e66203a050c2ce6c17(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7338d0bbe29dd57fab54(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testbd4f6be8cdb35c6f251d(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8f0d1345830b83dfc1b7(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa4e1c63bc2832a9ffb90(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9d5abf8cc60b9bd0d314(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test332175e66c385ed1a97e(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test34238765b14f93c81e57(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test10797ce06190a3866a08(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste92cd495aff9e502a1ca(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf02ffda4a54ad1390ab6(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test252e5030af4f0ab7bf2b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testfe3d96e64a5db1098e2d(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test94c9f1a9c03f970dde84(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd16f010f550e384c3e59(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9b6529d7a517cd7dbc13(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test706fdc6bcd111cd4de81(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7ce41e86a2afa55d59d7(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test14aebe584c51e7c2a682(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb9987fabfcd184f82c65(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testbe1a5e58793366c0c07a(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1eac7fa4e151174d20c5(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5a372a31cdbea0111962(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testcc1a35b80f0b02e1255e(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb04cc0e338c9b30cffa3(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb0a82e7bf3b5bdebdd9c(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0d883c1652c0b59e6643(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa38bc626fc7e3454333b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8bea94aa1202a12d9ae9(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test24a263a0eb80caaaea4b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb86b045d63884140fd1d(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9d261e666ae24a9cfc70(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test331919d746512b1bd2dd(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test58b5cb1c0a14ca820fa0(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testfcc5a2d53d8b78bff00e(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4e18e90b6551c4af46b7(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testfd0cdd7da5ea80def96c(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test31fa2c218bc9f47d31b5(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test486f1defd8f55e9519a9(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testbd67707be3be50792791(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1a5d67d4591ad4c8d1e7(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd1b42a0d7e5156b00706(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test120e16514220d9f1b114(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf3200a3148254d3357d3(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test291bb8d225b135c1f926(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd89446599f31a400dcec(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test10810f50a49dfe065bfa(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb1f754216d575a8cc3af(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb9d82396ef66bed18aed(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5469f77f98702583e6ea(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testea4d055788f9af327d2e(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test08ac3b6e6f8814cdc77a(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9e8e5ac1687da916f607(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6265b47bba1fd6839697(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa78aad03d3d6c0cd9810(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test050fb21ac4e0ec123acc(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test41e74fd70f88555712db(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testc066eeba93b49bfd475e(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testba918d828779830ff775(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9fc69104bdb595977460(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0dd65e43cc41ad032d71(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test324295b9fb6b58411e30(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8aebc8d0e0485dfeb252(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd222de940e9a99d43cdd(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9b7e3c531ced5ced7f08(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testcfc4d3d407725683e731(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test76848f10a77db08e038e(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa4c728e62357ca05c45c(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test666ae3bb483cb7d83170(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6ec557a6e48fd6900cb1(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test18a5d0db57d08d737b99(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd974ded8f39d6b77c0a1(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6c6d47297f6ea03c588b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testfb543650c644777c82ec(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test85cf601a990a9689b6c2(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testae2c05021a270e7e6ce6(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testceebe4a07ec516cb5a7a(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1a2b24646720aa998cbb(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf415efe81b5c2c8112a2(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0f1b297a0cb08c094411(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1b49cc3347751dcb09a9(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testffe72c176661d48910bd(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf3f2d1d0e79f326b3d2f(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testeb66a6725a958bb76923(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0a8fc83bac630f116c86(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6c62ccca61f383967d91(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6ba9e6495bef38087e7f(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test643e77d887ec7390c1c9(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa9725ffe34acd33603c4(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1a65df7ae90ac6ba1f22(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7f982251b9a09ebb1059(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4b2138b3eafc346d6bd3(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa316e26e6b940d585005(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testda2e0d2a6fd2a83cb298(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test54aac276524c0baaecd1(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd1fb3fd39f4a2fda5c6a(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4aaf20817b31a62e905f(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test446f00cbeee81a34d936(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb13663976ee52dac5370(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd245994f89c881efef06(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb6e7c5ee86f3c0559fe3(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testee28e38a088388ee5d9f(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9b40d68cb65d34934b50(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test045fac0048d3cfc15d88(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test85c3ad95991be29b3aff(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2abfed1da9a2ab365c18(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa999e4a9d0199bf463ec(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8452a26a810950ad47b2(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1e2b3a261f435a0f6316(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1423ee4cd0795e6ef646(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf95070e81f8def8ceec9(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf533a12940040f665761(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test51bd09a609a537b79c8a(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testc10b994394aa86a1789b(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testc5de3511ef8fa1e5841e(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0e957613f266e5693f83(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf36b8fb2ec772e02a48c(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test249b4ee9b0e6936bdfcf(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testdae669b09d29d1d05e81(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa3645755a60c69fe8af4(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5500c0af0cbb6a8efc04(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6528fe35c2b993cbd28b(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test072a41bf81e5b4dcd7d2(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3b47e24ba197ca8f686b(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test938c1c274b5ebf36c6b2(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testeea818614d4fbef183a8(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test18533eabe468baceac59(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste239cd01ef68ce26375d(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd3a9b36f8218cd633402(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa899f4512569981104e8(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test230a8fd0d19c5e15963b(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4fa431a3bea1c616f8d0(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testdd05d8f0df4f1ba79b8f(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0cb381d8a384434a2646(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testeadefbe24693d510ac03(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0b43e898410a9da3db1a(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test450ed0f6d19326bab043(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test38b7c3f09ffb6f4e73a5(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testab434195f62ee39997ae(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testdce4c351d6c426d24f14(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2363249f46e86ae9bd64(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6225a910ac0a5ce7304f(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf7898be54854e4587c54(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1ab55f98773e2e58c659(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf116d0bf1446d5e989db(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test90df709c20ab5305b5b0(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test37e041f7726819b98004(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5c13c8d0762eb77abbbe(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3c9ec0009e080492d6a0(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd016b2610a9701c799be(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test12e858bf6ec981811cc8(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0e09fd8b6ac12a309a36(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndDoc;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testca30dc12961feab24a33(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test162ca62af5cdf9d02507(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste3a7658df7a81c1ce8e5(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9dac5e4f0e78f96fcaad(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testaf2e952791b73c6bf78c(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test82b161574e0926b9c854(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test13c3f29be0e6b7bc92b1(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("anchor");
    out << "foo";
    out << YAML::VerbatimTag("tag");
    out << YAML::Anchor("other");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("tag", 1, "foo");
    EXPECT_SCALAR("tag", 2, "bar");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste87c7e9ce16fd2ac5f0e(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testeed7f7f244221932d32f(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test31a5563bfa532571339f(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test502ab92aab5195ff4759(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testdbe2ce5bc02435009b2c(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test06b8d2cc9bbb233d55d5(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << "foo";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9adf48ae5d6dff37e89a(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << "foo";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test398282871dcc1c7f8dbe(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2e2b3c35732210898be1(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test155697ae715940b6d804(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb01c87881d846bb10ecd(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test84a9d3d2ae4eaacc9c98(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb836cd5dc876cf6eb204(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4aa81dc715c5e5c53de1(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testfa8ba9d405de1af93537(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3c07c460dae114d3f278(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf513330011b92283e713(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test88667a8fe856748b4dc6(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7cbc7ae88a6b60d3cb54(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8b126d00ee878273f3e9(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test414dcec883b2fb2668d9(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test41754fe2ab40560f1afe(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test39b7a9ee0ccb5580ef60(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1f4cbfdb2f53d041fb74(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testc20612e8922a8eeba24d(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test693f48133cf726f1e05c(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test12b26bfed7a24736dd8b(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test41d6ec7f045897841e9c(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2bc8d1d6e4ec042ede3e(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test647ff876c844ad7540ff(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test844c1bcb896dde8ea51b(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testbcf04a5174e8505d1891(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testdb6c8ca130035d7a271f(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa0db96f3d93a255f2201(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4fb3eaac714942122715(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test634678f31daa20127d6c(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1dd395a1149e46dcc208(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testebc585cbde90d10a0af1(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd1ecec189e73f8932485(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf8fc72597f0a41b22daa(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd01959c5c228946c8759(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste8236a9672d9244ca486(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test22bd5d24dbd3f0670f97(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0f2c6cac0ce0e624eb08(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2ed2eef7f03696ca3c94(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6fe33177e10d14328a14(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0ea79e93c3439d90bdb8(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2ab970621d7e037153c9(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4229059ec3d639faf4b2(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6df809b4712b73c8577f(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test37a0a34b2bef743d8241(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd6c37c1e50617419a37d(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2fb71689fb176533159c(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa91d7999dd9b43fb5827(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8dfd4533e2891d3861ec(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0a9b0c29dfcf4f071eb9(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3882a796318e573b115d(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1ade39a1572a12eda7b8(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd308c7e19671725523cd(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1ac58dc569ce7b4eebdf(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5fe4c34b163d0efa12d5(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test78433ff7edf3224ce58b(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test089ed50cafddf39653d6(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test809723ec7bdea45cf562(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test953cf821d2acc7898582(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0f1db947e4627596eace(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test52f3b0674f30d955eea7(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf95a488631e07f6ca914(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9b7a916f4c1e2a1ae6ee(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test54ea34948e814ef79607(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testcef62af7508e0a1e3ee3(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testffcfb27f0c904fae7833(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test31ed1460205bbc5a4a68(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7d04c8d923b046159db0(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test31b396851023614cf9fd(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5befd800aa07d83e2df7(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2df4f378f687fd80b98c(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test232f5aa6ea7e85e186c4(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test011421ad7da175099088(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf50217feca0ae03a0b03(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste4b49044063dd3c8a7ff(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testbe9295522ec1f0bc9de5(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7a18055f1e3a49f93d40(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd8eebabad65b8ef02375(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test948f36e3182f3c1aa800(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testde02b69600e5931c39ab(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5fdab21609e7017c3b86(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testfb52965f57b912ec23a4(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa3f220ead85b78154f89(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0ef1b1c26e8a1fa34ccd(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7661db62a921285da885(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndMap;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9382f466be3e19ca395f(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test33c4f45355dc7df2e2a8(YAML::Emitter& out)
{
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_MAP_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testc55c40f32c34c890acce(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb06ba64c5895f218175d(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test14adb5374833871b2d0c(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7ff7826c0f0563ce5a65(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test394e607327447b08e729(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testdf03e1437e901976c2c8(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test77467fcda467dd063050(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5bcea73651331a2357d0(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2956b3f097a16a4cd951(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3170422d0cad24cd602a(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb37f0cd80f138e8f2622(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3e00cce71da4636fa1f7(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testfd184c04759685f21abb(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test14ab4965eff0a569da16(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test271811f2df7210366780(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testfcfe8657dffc21f6cd45(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test609e44eab4ab95f31e33(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7841fc715275a45a2770(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test662c03de87ca40bd943e(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0a9475ec3c946fe11991(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test94d28ebdbee90f430eb1(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd5035afc82e23b67ce03(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testcc9788c342da4454758f(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test140974453293fdb1155d(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testdc0b80a131730e98d735(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1c5225b07d746c2bd331(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa49a0be204cd2b57f17b(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testbe08cc0a08cf2cb5e7ec(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4d2a2e12689655edd77c(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4d4a25a54401f0282ceb(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test91f55feebb012ce89a93(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test1f3d0b19c6a346b087e0(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4e26682c2daf8ded04a6(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6f24e6df03922bba0d8a(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test60849eca7dc178908ff1(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test28b7db2ac68bb806e143(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test8db156db7065942bc260(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste240aced6e2292a9b091(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test468628a845426ce4a106(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa3a2d467766b74acd6fd(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5bf63d8ed606d688d869(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0d35c1487237ba7d8bdc(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb1fddc2897760d60e733(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testbaf845554a46f088bf71(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test6383f28d62ad9ce3c075(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test88a4c1cc11b99a61eccd(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4716a2cf58a70705987b(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test75222084929bd0f9d38f(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test2fb23c79eec625216523(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb1699a6b7c5ded480677(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testd7de744a20ca1dc099db(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test900b2dcf20981b44ea65(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test20cc330b6d1171584aed(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5ea8e3642fab864fb09d(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test42e21cbc65f534972ead(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test14e3b5dca1d7a5a0c957(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9bd4800a58394b172738(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb715a2b66987a872ced8(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste9b56880009cc6899131(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test21f96f767e38471c9d4d(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa8aebba05fc1858c0a6c(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste6e7442377049b17ee9e(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test428b593e283163fee752(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0b6c63323da4bf9798c2(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test0f4c45c39fe39dfc8a1d(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb8043a7ae1de42dd81db(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test4d9b278579ffb76fc56d(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test672fc8b6d281f82b9332(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb406d378fa0df952b051(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test68a227d03f20863f37e4(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testcee8582fd340377bda46(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test06fd48e8c86baf6fc05b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test70b4ccbf71c0716bf8e4(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test449c2b349be8da36682b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9620fa69718e3b4fe391(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test3faaebe701bea6f8ee39(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test763ee61808091c7a354d(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("comment");
    out << YAML::EndSeq;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test81b0d6b575228cde91e5(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testb607ae3c5d560092e37b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testa53c54726737df14a5dd(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SEQ_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test071d73b309a1365e0b07(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf8f45511528fa28cddcb(YAML::Emitter& out)
{
    out << YAML::Comment("comment");
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testabdd2bf3bdf550e3dd60(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test53424b35498a73fbede9(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testf0c6c1a1afced157d6a5(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST teste45dbac33918e0fee74f(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test903c7ab3d09d4323107f(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test5d39d351680dba4be04b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << YAML::Comment("comment");
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testaa1e8d6d4385aab47bcd(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << YAML::Comment("comment");
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test9bd238b748ced1db588b(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::Comment("comment");
    out << YAML::EndMap;
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST testec1cdffaae8842854947(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::Comment("comment");
    out << YAML::EndSeq;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test30727d97de63c1ad395a(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
TEST test7adafdc8be65a5d610bf(YAML::Emitter& out)
{
    out << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::BeginMap;
    out << "foo";
    out << "bar";
    out << YAML::EndMap;
    out << YAML::EndSeq;
    out << YAML::Comment("comment");

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SEQ_START("?", 0);
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_MAP_START("?", 0);
    EXPECT_SCALAR("?", 0, "foo");
    EXPECT_SCALAR("?", 0, "bar");
    EXPECT_MAP_END();
    EXPECT_SEQ_END();
    EXPECT_DOC_END();
    DONE();
}
}
#endif // YAML_GEN_TESTS

void RunGenEmitterTests(int& passed, int& total)
{
#ifdef YAML_GEN_TESTS
    RunGenEmitterTest(&Emitter::test02571eee35ac0cbd3777, "test02571eee35ac0cbd3777", passed, total);
    RunGenEmitterTest(&Emitter::test71b969ca18898d226320, "test71b969ca18898d226320", passed, total);
    RunGenEmitterTest(&Emitter::testd69e4ea95ce6f221c6e7, "testd69e4ea95ce6f221c6e7", passed, total);
    RunGenEmitterTest(&Emitter::testffbfd295ad9bef4deb00, "testffbfd295ad9bef4deb00", passed, total);
    RunGenEmitterTest(&Emitter::test6a459b2fe1f6e961e1a7, "test6a459b2fe1f6e961e1a7", passed, total);
    RunGenEmitterTest(&Emitter::test869ab95640c9933ed4d6, "test869ab95640c9933ed4d6", passed, total);
    RunGenEmitterTest(&Emitter::test433c0771f40ac3ba853e, "test433c0771f40ac3ba853e", passed, total);
    RunGenEmitterTest(&Emitter::teste181778974c4003bc5a4, "teste181778974c4003bc5a4", passed, total);
    RunGenEmitterTest(&Emitter::testf8cb7e3f1b11791f53b8, "testf8cb7e3f1b11791f53b8", passed, total);
    RunGenEmitterTest(&Emitter::test3c48ed06807100f0a111, "test3c48ed06807100f0a111", passed, total);
    RunGenEmitterTest(&Emitter::testb13f7b031f425b0e383f, "testb13f7b031f425b0e383f", passed, total);
    RunGenEmitterTest(&Emitter::testb77284234d3fbe8b24a0, "testb77284234d3fbe8b24a0", passed, total);
    RunGenEmitterTest(&Emitter::test9c56fd285b563327a340, "test9c56fd285b563327a340", passed, total);
    RunGenEmitterTest(&Emitter::test1c08639d56176e64c885, "test1c08639d56176e64c885", passed, total);
    RunGenEmitterTest(&Emitter::test94c8742f8cab3cec1b4a, "test94c8742f8cab3cec1b4a", passed, total);
    RunGenEmitterTest(&Emitter::test79d1806ceb3ecebfa60b, "test79d1806ceb3ecebfa60b", passed, total);
    RunGenEmitterTest(&Emitter::test360afe50348ec36569d3, "test360afe50348ec36569d3", passed, total);
    RunGenEmitterTest(&Emitter::teste599b3fc1857f4265d3b, "teste599b3fc1857f4265d3b", passed, total);
    RunGenEmitterTest(&Emitter::test88adf7adb474ad063424, "test88adf7adb474ad063424", passed, total);
    RunGenEmitterTest(&Emitter::test0978ca6f6358ea06e024, "test0978ca6f6358ea06e024", passed, total);
    RunGenEmitterTest(&Emitter::testd915f57fca4b0f6d77b4, "testd915f57fca4b0f6d77b4", passed, total);
    RunGenEmitterTest(&Emitter::test1fe1f22496f2a0ffd64e, "test1fe1f22496f2a0ffd64e", passed, total);
    RunGenEmitterTest(&Emitter::test76422a4077d3bdd03579, "test76422a4077d3bdd03579", passed, total);
    RunGenEmitterTest(&Emitter::test92b168a497cb0c7e3144, "test92b168a497cb0c7e3144", passed, total);
    RunGenEmitterTest(&Emitter::testa93925b3ae311a7f11d4, "testa93925b3ae311a7f11d4", passed, total);
    RunGenEmitterTest(&Emitter::test2dd1aaf6a1c1924557d0, "test2dd1aaf6a1c1924557d0", passed, total);
    RunGenEmitterTest(&Emitter::test6ec0585d0f0945ad9dae, "test6ec0585d0f0945ad9dae", passed, total);
    RunGenEmitterTest(&Emitter::test7e00bca835d55844bbfe, "test7e00bca835d55844bbfe", passed, total);
    RunGenEmitterTest(&Emitter::test70912c7d920a0597bbb2, "test70912c7d920a0597bbb2", passed, total);
    RunGenEmitterTest(&Emitter::test16eacbf77bccde360e54, "test16eacbf77bccde360e54", passed, total);
    RunGenEmitterTest(&Emitter::test556e0c86efb0716d2778, "test556e0c86efb0716d2778", passed, total);
    RunGenEmitterTest(&Emitter::test71b64326d72fe100e6ad, "test71b64326d72fe100e6ad", passed, total);
    RunGenEmitterTest(&Emitter::test0c7bb03fbd6b52ea3ad6, "test0c7bb03fbd6b52ea3ad6", passed, total);
    RunGenEmitterTest(&Emitter::testb819efb5742c1176df98, "testb819efb5742c1176df98", passed, total);
    RunGenEmitterTest(&Emitter::test1f7b7cd5a13070c723d3, "test1f7b7cd5a13070c723d3", passed, total);
    RunGenEmitterTest(&Emitter::test32126a88cb2b7311e779, "test32126a88cb2b7311e779", passed, total);
    RunGenEmitterTest(&Emitter::testd7f952713bde5ce2f9e7, "testd7f952713bde5ce2f9e7", passed, total);
    RunGenEmitterTest(&Emitter::test5030b4f2d1efb798f320, "test5030b4f2d1efb798f320", passed, total);
    RunGenEmitterTest(&Emitter::testb9015537b9a9e09b8ec8, "testb9015537b9a9e09b8ec8", passed, total);
    RunGenEmitterTest(&Emitter::test03229f6d33fa9007a65d, "test03229f6d33fa9007a65d", passed, total);
    RunGenEmitterTest(&Emitter::testf998264dcfd0dba06c0a, "testf998264dcfd0dba06c0a", passed, total);
    RunGenEmitterTest(&Emitter::test7defadc52eddfbf766aa, "test7defadc52eddfbf766aa", passed, total);
    RunGenEmitterTest(&Emitter::test55a7c58211689c7815b2, "test55a7c58211689c7815b2", passed, total);
    RunGenEmitterTest(&Emitter::testc3873d954433175e0143, "testc3873d954433175e0143", passed, total);
    RunGenEmitterTest(&Emitter::test81fb6bf1f976e0ad3fba, "test81fb6bf1f976e0ad3fba", passed, total);
    RunGenEmitterTest(&Emitter::test29e7ff04645f56a7ea2f, "test29e7ff04645f56a7ea2f", passed, total);
    RunGenEmitterTest(&Emitter::testce0089a55f926d311ff4, "testce0089a55f926d311ff4", passed, total);
    RunGenEmitterTest(&Emitter::testd1d301bbc73ec11cd49b, "testd1d301bbc73ec11cd49b", passed, total);
    RunGenEmitterTest(&Emitter::test4640bfb42711b7209ef9, "test4640bfb42711b7209ef9", passed, total);
    RunGenEmitterTest(&Emitter::test1133d19fc3a4ec9fb3e8, "test1133d19fc3a4ec9fb3e8", passed, total);
    RunGenEmitterTest(&Emitter::test4a6d083241180899f7ed, "test4a6d083241180899f7ed", passed, total);
    RunGenEmitterTest(&Emitter::testbadb5b228a4db78efac0, "testbadb5b228a4db78efac0", passed, total);
    RunGenEmitterTest(&Emitter::test5c6d607ed1ad046568e1, "test5c6d607ed1ad046568e1", passed, total);
    RunGenEmitterTest(&Emitter::testac34cde109884bb6876b, "testac34cde109884bb6876b", passed, total);
    RunGenEmitterTest(&Emitter::test5c19597d5336d541f990, "test5c19597d5336d541f990", passed, total);
    RunGenEmitterTest(&Emitter::test39e174ed33d5508a61ce, "test39e174ed33d5508a61ce", passed, total);
    RunGenEmitterTest(&Emitter::test43e26cf94441cee4a0c4, "test43e26cf94441cee4a0c4", passed, total);
    RunGenEmitterTest(&Emitter::test06afa8e5b516630fc8da, "test06afa8e5b516630fc8da", passed, total);
    RunGenEmitterTest(&Emitter::test4d109db0282a7797cdcb, "test4d109db0282a7797cdcb", passed, total);
    RunGenEmitterTest(&Emitter::test0c1c00113c20dfa650a9, "test0c1c00113c20dfa650a9", passed, total);
    RunGenEmitterTest(&Emitter::testccd7f2183f06483ee5e0, "testccd7f2183f06483ee5e0", passed, total);
    RunGenEmitterTest(&Emitter::test52e25e363a17f37c296f, "test52e25e363a17f37c296f", passed, total);
    RunGenEmitterTest(&Emitter::testdd81d16d3bdd8636af16, "testdd81d16d3bdd8636af16", passed, total);
    RunGenEmitterTest(&Emitter::testd76e8eb5043431c3434e, "testd76e8eb5043431c3434e", passed, total);
    RunGenEmitterTest(&Emitter::teste03be55aff3dc08f07a1, "teste03be55aff3dc08f07a1", passed, total);
    RunGenEmitterTest(&Emitter::test8ba3e94c45f696c5027b, "test8ba3e94c45f696c5027b", passed, total);
    RunGenEmitterTest(&Emitter::test90e45bcf67b89e31fd12, "test90e45bcf67b89e31fd12", passed, total);
    RunGenEmitterTest(&Emitter::test0b7c4e535bfebfa3c85d, "test0b7c4e535bfebfa3c85d", passed, total);
    RunGenEmitterTest(&Emitter::test3a1c08f490683d254cda, "test3a1c08f490683d254cda", passed, total);
    RunGenEmitterTest(&Emitter::test926cd343d3999525d9ce, "test926cd343d3999525d9ce", passed, total);
    RunGenEmitterTest(&Emitter::test1e5dd6b1e4dfb59f0346, "test1e5dd6b1e4dfb59f0346", passed, total);
    RunGenEmitterTest(&Emitter::test0f064b30c1187ff2dd4b, "test0f064b30c1187ff2dd4b", passed, total);
    RunGenEmitterTest(&Emitter::test58fd20c1736964e12b53, "test58fd20c1736964e12b53", passed, total);
    RunGenEmitterTest(&Emitter::testbddd187d973b632dc188, "testbddd187d973b632dc188", passed, total);
    RunGenEmitterTest(&Emitter::test5bc168e49ab7503fd2e1, "test5bc168e49ab7503fd2e1", passed, total);
    RunGenEmitterTest(&Emitter::test712e139d81db4e14196d, "test712e139d81db4e14196d", passed, total);
    RunGenEmitterTest(&Emitter::test48130bfdef5d192b888e, "test48130bfdef5d192b888e", passed, total);
    RunGenEmitterTest(&Emitter::test27552de54635da852895, "test27552de54635da852895", passed, total);
    RunGenEmitterTest(&Emitter::test6529691d17594d5abeb6, "test6529691d17594d5abeb6", passed, total);
    RunGenEmitterTest(&Emitter::test46b8a7b9d2461d80e0c0, "test46b8a7b9d2461d80e0c0", passed, total);
    RunGenEmitterTest(&Emitter::testbd1cb845de056e97a301, "testbd1cb845de056e97a301", passed, total);
    RunGenEmitterTest(&Emitter::testfe1f94c842b37340db76, "testfe1f94c842b37340db76", passed, total);
    RunGenEmitterTest(&Emitter::teste3c7fbf8af4d82e891e3, "teste3c7fbf8af4d82e891e3", passed, total);
    RunGenEmitterTest(&Emitter::testd6800d90d6d037d02ace, "testd6800d90d6d037d02ace", passed, total);
    RunGenEmitterTest(&Emitter::testdf987230fa431b7a8f1b, "testdf987230fa431b7a8f1b", passed, total);
    RunGenEmitterTest(&Emitter::testcd22b774448a8b15345e, "testcd22b774448a8b15345e", passed, total);
    RunGenEmitterTest(&Emitter::test591a370a1ce302d23688, "test591a370a1ce302d23688", passed, total);
    RunGenEmitterTest(&Emitter::test5e56c5800a9f2c4591ff, "test5e56c5800a9f2c4591ff", passed, total);
    RunGenEmitterTest(&Emitter::test72f6d368cc2f52b488bd, "test72f6d368cc2f52b488bd", passed, total);
    RunGenEmitterTest(&Emitter::test6786da34791cbab71591, "test6786da34791cbab71591", passed, total);
    RunGenEmitterTest(&Emitter::test6d53136f35632180e2e8, "test6d53136f35632180e2e8", passed, total);
    RunGenEmitterTest(&Emitter::test01020a01f84721d7fb07, "test01020a01f84721d7fb07", passed, total);
    RunGenEmitterTest(&Emitter::testc3ab223703ef17e47ec7, "testc3ab223703ef17e47ec7", passed, total);
    RunGenEmitterTest(&Emitter::test033db6218db214ae5ef9, "test033db6218db214ae5ef9", passed, total);
    RunGenEmitterTest(&Emitter::test38fe09343ac97f51b38f, "test38fe09343ac97f51b38f", passed, total);
    RunGenEmitterTest(&Emitter::test99eb29693d619703a052, "test99eb29693d619703a052", passed, total);
    RunGenEmitterTest(&Emitter::teste9e58998a49132e15fb4, "teste9e58998a49132e15fb4", passed, total);
    RunGenEmitterTest(&Emitter::testc1b4fba9280329b30583, "testc1b4fba9280329b30583", passed, total);
    RunGenEmitterTest(&Emitter::test2f8651438d44de183d22, "test2f8651438d44de183d22", passed, total);
    RunGenEmitterTest(&Emitter::test4c506bf0bc7a972cb62d, "test4c506bf0bc7a972cb62d", passed, total);
    RunGenEmitterTest(&Emitter::test6a42214b8698a0e87f5f, "test6a42214b8698a0e87f5f", passed, total);
    RunGenEmitterTest(&Emitter::teste0b5020ccbc0cbc7f699, "teste0b5020ccbc0cbc7f699", passed, total);
    RunGenEmitterTest(&Emitter::test3fb453ac1de7a2d37a16, "test3fb453ac1de7a2d37a16", passed, total);
    RunGenEmitterTest(&Emitter::test4d3236ecd88c5faa74e8, "test4d3236ecd88c5faa74e8", passed, total);
    RunGenEmitterTest(&Emitter::test116230234c38c68eb060, "test116230234c38c68eb060", passed, total);
    RunGenEmitterTest(&Emitter::testb7c63d73350b11bf4a56, "testb7c63d73350b11bf4a56", passed, total);
    RunGenEmitterTest(&Emitter::test26ae0a3b97fb1c7743bf, "test26ae0a3b97fb1c7743bf", passed, total);
    RunGenEmitterTest(&Emitter::testeabf01d5500c4f5c9de5, "testeabf01d5500c4f5c9de5", passed, total);
    RunGenEmitterTest(&Emitter::testf9195cd2fb4c57783870, "testf9195cd2fb4c57783870", passed, total);
    RunGenEmitterTest(&Emitter::test8c7159f70888a6c5548e, "test8c7159f70888a6c5548e", passed, total);
    RunGenEmitterTest(&Emitter::testba96c9cdf5e82c9ebd0f, "testba96c9cdf5e82c9ebd0f", passed, total);
    RunGenEmitterTest(&Emitter::test6cbb2232cc1e43752958, "test6cbb2232cc1e43752958", passed, total);
    RunGenEmitterTest(&Emitter::test2bc126cc8be9e3d0a5bb, "test2bc126cc8be9e3d0a5bb", passed, total);
    RunGenEmitterTest(&Emitter::testb053b9b6ee7c7eecc798, "testb053b9b6ee7c7eecc798", passed, total);
    RunGenEmitterTest(&Emitter::testda8339179085c81ac7a9, "testda8339179085c81ac7a9", passed, total);
    RunGenEmitterTest(&Emitter::testf38e7a065a9bda416bf0, "testf38e7a065a9bda416bf0", passed, total);
    RunGenEmitterTest(&Emitter::test5345180f2a8a65af5b72, "test5345180f2a8a65af5b72", passed, total);
    RunGenEmitterTest(&Emitter::test7aee5968853276b78e65, "test7aee5968853276b78e65", passed, total);
    RunGenEmitterTest(&Emitter::test1c20b15f6680fd1fa304, "test1c20b15f6680fd1fa304", passed, total);
    RunGenEmitterTest(&Emitter::test3ad355d8aa94a80ed9d0, "test3ad355d8aa94a80ed9d0", passed, total);
    RunGenEmitterTest(&Emitter::test717b89ec9b7b004e5c17, "test717b89ec9b7b004e5c17", passed, total);
    RunGenEmitterTest(&Emitter::testc540c8d6d92913953ef8, "testc540c8d6d92913953ef8", passed, total);
    RunGenEmitterTest(&Emitter::test805391f6964c07b1fc51, "test805391f6964c07b1fc51", passed, total);
    RunGenEmitterTest(&Emitter::testafc978dbd9b5d8005968, "testafc978dbd9b5d8005968", passed, total);
    RunGenEmitterTest(&Emitter::testf137897e42e659d45548, "testf137897e42e659d45548", passed, total);
    RunGenEmitterTest(&Emitter::test41c10a5f012922d6d240, "test41c10a5f012922d6d240", passed, total);
    RunGenEmitterTest(&Emitter::test6d46af9b0e1bab6eefd2, "test6d46af9b0e1bab6eefd2", passed, total);
    RunGenEmitterTest(&Emitter::test6908c1e71bca5b5a09b6, "test6908c1e71bca5b5a09b6", passed, total);
    RunGenEmitterTest(&Emitter::test158d6160ee94d7f929c0, "test158d6160ee94d7f929c0", passed, total);
    RunGenEmitterTest(&Emitter::test07186666318de7b13975, "test07186666318de7b13975", passed, total);
    RunGenEmitterTest(&Emitter::testdcb6011d1dbc47e024b4, "testdcb6011d1dbc47e024b4", passed, total);
    RunGenEmitterTest(&Emitter::test998e6b11c32e19f91a43, "test998e6b11c32e19f91a43", passed, total);
    RunGenEmitterTest(&Emitter::test7b009b420c34f753e2dc, "test7b009b420c34f753e2dc", passed, total);
    RunGenEmitterTest(&Emitter::testba4c5b4eedf23c16ab44, "testba4c5b4eedf23c16ab44", passed, total);
    RunGenEmitterTest(&Emitter::test022d120061a5c77c6640, "test022d120061a5c77c6640", passed, total);
    RunGenEmitterTest(&Emitter::testeb346f4b70732835631f, "testeb346f4b70732835631f", passed, total);
    RunGenEmitterTest(&Emitter::testade70114082f144726ee, "testade70114082f144726ee", passed, total);
    RunGenEmitterTest(&Emitter::testf2b68aae6173ab6ad66d, "testf2b68aae6173ab6ad66d", passed, total);
    RunGenEmitterTest(&Emitter::teste9a962c26a72ea4d3f8d, "teste9a962c26a72ea4d3f8d", passed, total);
    RunGenEmitterTest(&Emitter::testa9603ff993f8a8d47b5d, "testa9603ff993f8a8d47b5d", passed, total);
    RunGenEmitterTest(&Emitter::test2b70cf1579b37e0fb086, "test2b70cf1579b37e0fb086", passed, total);
    RunGenEmitterTest(&Emitter::test3790d77103bac72dc302, "test3790d77103bac72dc302", passed, total);
    RunGenEmitterTest(&Emitter::testadfa25cb2fd5a9346102, "testadfa25cb2fd5a9346102", passed, total);
    RunGenEmitterTest(&Emitter::test9764ad298ba1fe9ecfa8, "test9764ad298ba1fe9ecfa8", passed, total);
    RunGenEmitterTest(&Emitter::test22f20fac5e02211edadc, "test22f20fac5e02211edadc", passed, total);
    RunGenEmitterTest(&Emitter::test8b3b22c8ffd679b15623, "test8b3b22c8ffd679b15623", passed, total);
    RunGenEmitterTest(&Emitter::test438d1581dec9088389d7, "test438d1581dec9088389d7", passed, total);
    RunGenEmitterTest(&Emitter::test0b417297c8e11f038c7c, "test0b417297c8e11f038c7c", passed, total);
    RunGenEmitterTest(&Emitter::testa3686f0b87652d4640c2, "testa3686f0b87652d4640c2", passed, total);
    RunGenEmitterTest(&Emitter::testd6f021791f2162c85174, "testd6f021791f2162c85174", passed, total);
    RunGenEmitterTest(&Emitter::test3d34018a42371ab9fbcd, "test3d34018a42371ab9fbcd", passed, total);
    RunGenEmitterTest(&Emitter::test7a9287f053731d912e63, "test7a9287f053731d912e63", passed, total);
    RunGenEmitterTest(&Emitter::testa9aa047a659d330a4a8b, "testa9aa047a659d330a4a8b", passed, total);
    RunGenEmitterTest(&Emitter::test74e66203a050c2ce6c17, "test74e66203a050c2ce6c17", passed, total);
    RunGenEmitterTest(&Emitter::test7338d0bbe29dd57fab54, "test7338d0bbe29dd57fab54", passed, total);
    RunGenEmitterTest(&Emitter::testbd4f6be8cdb35c6f251d, "testbd4f6be8cdb35c6f251d", passed, total);
    RunGenEmitterTest(&Emitter::test8f0d1345830b83dfc1b7, "test8f0d1345830b83dfc1b7", passed, total);
    RunGenEmitterTest(&Emitter::testa4e1c63bc2832a9ffb90, "testa4e1c63bc2832a9ffb90", passed, total);
    RunGenEmitterTest(&Emitter::test9d5abf8cc60b9bd0d314, "test9d5abf8cc60b9bd0d314", passed, total);
    RunGenEmitterTest(&Emitter::test332175e66c385ed1a97e, "test332175e66c385ed1a97e", passed, total);
    RunGenEmitterTest(&Emitter::test34238765b14f93c81e57, "test34238765b14f93c81e57", passed, total);
    RunGenEmitterTest(&Emitter::test10797ce06190a3866a08, "test10797ce06190a3866a08", passed, total);
    RunGenEmitterTest(&Emitter::teste92cd495aff9e502a1ca, "teste92cd495aff9e502a1ca", passed, total);
    RunGenEmitterTest(&Emitter::testf02ffda4a54ad1390ab6, "testf02ffda4a54ad1390ab6", passed, total);
    RunGenEmitterTest(&Emitter::test252e5030af4f0ab7bf2b, "test252e5030af4f0ab7bf2b", passed, total);
    RunGenEmitterTest(&Emitter::testfe3d96e64a5db1098e2d, "testfe3d96e64a5db1098e2d", passed, total);
    RunGenEmitterTest(&Emitter::test94c9f1a9c03f970dde84, "test94c9f1a9c03f970dde84", passed, total);
    RunGenEmitterTest(&Emitter::testd16f010f550e384c3e59, "testd16f010f550e384c3e59", passed, total);
    RunGenEmitterTest(&Emitter::test9b6529d7a517cd7dbc13, "test9b6529d7a517cd7dbc13", passed, total);
    RunGenEmitterTest(&Emitter::test706fdc6bcd111cd4de81, "test706fdc6bcd111cd4de81", passed, total);
    RunGenEmitterTest(&Emitter::test7ce41e86a2afa55d59d7, "test7ce41e86a2afa55d59d7", passed, total);
    RunGenEmitterTest(&Emitter::test14aebe584c51e7c2a682, "test14aebe584c51e7c2a682", passed, total);
    RunGenEmitterTest(&Emitter::testb9987fabfcd184f82c65, "testb9987fabfcd184f82c65", passed, total);
    RunGenEmitterTest(&Emitter::testbe1a5e58793366c0c07a, "testbe1a5e58793366c0c07a", passed, total);
    RunGenEmitterTest(&Emitter::test1eac7fa4e151174d20c5, "test1eac7fa4e151174d20c5", passed, total);
    RunGenEmitterTest(&Emitter::test5a372a31cdbea0111962, "test5a372a31cdbea0111962", passed, total);
    RunGenEmitterTest(&Emitter::testcc1a35b80f0b02e1255e, "testcc1a35b80f0b02e1255e", passed, total);
    RunGenEmitterTest(&Emitter::testb04cc0e338c9b30cffa3, "testb04cc0e338c9b30cffa3", passed, total);
    RunGenEmitterTest(&Emitter::testb0a82e7bf3b5bdebdd9c, "testb0a82e7bf3b5bdebdd9c", passed, total);
    RunGenEmitterTest(&Emitter::test0d883c1652c0b59e6643, "test0d883c1652c0b59e6643", passed, total);
    RunGenEmitterTest(&Emitter::testa38bc626fc7e3454333b, "testa38bc626fc7e3454333b", passed, total);
    RunGenEmitterTest(&Emitter::test8bea94aa1202a12d9ae9, "test8bea94aa1202a12d9ae9", passed, total);
    RunGenEmitterTest(&Emitter::test24a263a0eb80caaaea4b, "test24a263a0eb80caaaea4b", passed, total);
    RunGenEmitterTest(&Emitter::testb86b045d63884140fd1d, "testb86b045d63884140fd1d", passed, total);
    RunGenEmitterTest(&Emitter::test9d261e666ae24a9cfc70, "test9d261e666ae24a9cfc70", passed, total);
    RunGenEmitterTest(&Emitter::test331919d746512b1bd2dd, "test331919d746512b1bd2dd", passed, total);
    RunGenEmitterTest(&Emitter::test58b5cb1c0a14ca820fa0, "test58b5cb1c0a14ca820fa0", passed, total);
    RunGenEmitterTest(&Emitter::testfcc5a2d53d8b78bff00e, "testfcc5a2d53d8b78bff00e", passed, total);
    RunGenEmitterTest(&Emitter::test4e18e90b6551c4af46b7, "test4e18e90b6551c4af46b7", passed, total);
    RunGenEmitterTest(&Emitter::testfd0cdd7da5ea80def96c, "testfd0cdd7da5ea80def96c", passed, total);
    RunGenEmitterTest(&Emitter::test31fa2c218bc9f47d31b5, "test31fa2c218bc9f47d31b5", passed, total);
    RunGenEmitterTest(&Emitter::test486f1defd8f55e9519a9, "test486f1defd8f55e9519a9", passed, total);
    RunGenEmitterTest(&Emitter::testbd67707be3be50792791, "testbd67707be3be50792791", passed, total);
    RunGenEmitterTest(&Emitter::test1a5d67d4591ad4c8d1e7, "test1a5d67d4591ad4c8d1e7", passed, total);
    RunGenEmitterTest(&Emitter::testd1b42a0d7e5156b00706, "testd1b42a0d7e5156b00706", passed, total);
    RunGenEmitterTest(&Emitter::test120e16514220d9f1b114, "test120e16514220d9f1b114", passed, total);
    RunGenEmitterTest(&Emitter::testf3200a3148254d3357d3, "testf3200a3148254d3357d3", passed, total);
    RunGenEmitterTest(&Emitter::test291bb8d225b135c1f926, "test291bb8d225b135c1f926", passed, total);
    RunGenEmitterTest(&Emitter::testd89446599f31a400dcec, "testd89446599f31a400dcec", passed, total);
    RunGenEmitterTest(&Emitter::test10810f50a49dfe065bfa, "test10810f50a49dfe065bfa", passed, total);
    RunGenEmitterTest(&Emitter::testb1f754216d575a8cc3af, "testb1f754216d575a8cc3af", passed, total);
    RunGenEmitterTest(&Emitter::testb9d82396ef66bed18aed, "testb9d82396ef66bed18aed", passed, total);
    RunGenEmitterTest(&Emitter::test5469f77f98702583e6ea, "test5469f77f98702583e6ea", passed, total);
    RunGenEmitterTest(&Emitter::testea4d055788f9af327d2e, "testea4d055788f9af327d2e", passed, total);
    RunGenEmitterTest(&Emitter::test08ac3b6e6f8814cdc77a, "test08ac3b6e6f8814cdc77a", passed, total);
    RunGenEmitterTest(&Emitter::test9e8e5ac1687da916f607, "test9e8e5ac1687da916f607", passed, total);
    RunGenEmitterTest(&Emitter::test6265b47bba1fd6839697, "test6265b47bba1fd6839697", passed, total);
    RunGenEmitterTest(&Emitter::testa78aad03d3d6c0cd9810, "testa78aad03d3d6c0cd9810", passed, total);
    RunGenEmitterTest(&Emitter::test050fb21ac4e0ec123acc, "test050fb21ac4e0ec123acc", passed, total);
    RunGenEmitterTest(&Emitter::test41e74fd70f88555712db, "test41e74fd70f88555712db", passed, total);
    RunGenEmitterTest(&Emitter::testc066eeba93b49bfd475e, "testc066eeba93b49bfd475e", passed, total);
    RunGenEmitterTest(&Emitter::testba918d828779830ff775, "testba918d828779830ff775", passed, total);
    RunGenEmitterTest(&Emitter::test9fc69104bdb595977460, "test9fc69104bdb595977460", passed, total);
    RunGenEmitterTest(&Emitter::test0dd65e43cc41ad032d71, "test0dd65e43cc41ad032d71", passed, total);
    RunGenEmitterTest(&Emitter::test324295b9fb6b58411e30, "test324295b9fb6b58411e30", passed, total);
    RunGenEmitterTest(&Emitter::test8aebc8d0e0485dfeb252, "test8aebc8d0e0485dfeb252", passed, total);
    RunGenEmitterTest(&Emitter::testd222de940e9a99d43cdd, "testd222de940e9a99d43cdd", passed, total);
    RunGenEmitterTest(&Emitter::test9b7e3c531ced5ced7f08, "test9b7e3c531ced5ced7f08", passed, total);
    RunGenEmitterTest(&Emitter::testcfc4d3d407725683e731, "testcfc4d3d407725683e731", passed, total);
    RunGenEmitterTest(&Emitter::test76848f10a77db08e038e, "test76848f10a77db08e038e", passed, total);
    RunGenEmitterTest(&Emitter::testa4c728e62357ca05c45c, "testa4c728e62357ca05c45c", passed, total);
    RunGenEmitterTest(&Emitter::test666ae3bb483cb7d83170, "test666ae3bb483cb7d83170", passed, total);
    RunGenEmitterTest(&Emitter::test6ec557a6e48fd6900cb1, "test6ec557a6e48fd6900cb1", passed, total);
    RunGenEmitterTest(&Emitter::test18a5d0db57d08d737b99, "test18a5d0db57d08d737b99", passed, total);
    RunGenEmitterTest(&Emitter::testd974ded8f39d6b77c0a1, "testd974ded8f39d6b77c0a1", passed, total);
    RunGenEmitterTest(&Emitter::test6c6d47297f6ea03c588b, "test6c6d47297f6ea03c588b", passed, total);
    RunGenEmitterTest(&Emitter::testfb543650c644777c82ec, "testfb543650c644777c82ec", passed, total);
    RunGenEmitterTest(&Emitter::test85cf601a990a9689b6c2, "test85cf601a990a9689b6c2", passed, total);
    RunGenEmitterTest(&Emitter::testae2c05021a270e7e6ce6, "testae2c05021a270e7e6ce6", passed, total);
    RunGenEmitterTest(&Emitter::testceebe4a07ec516cb5a7a, "testceebe4a07ec516cb5a7a", passed, total);
    RunGenEmitterTest(&Emitter::test1a2b24646720aa998cbb, "test1a2b24646720aa998cbb", passed, total);
    RunGenEmitterTest(&Emitter::testf415efe81b5c2c8112a2, "testf415efe81b5c2c8112a2", passed, total);
    RunGenEmitterTest(&Emitter::test0f1b297a0cb08c094411, "test0f1b297a0cb08c094411", passed, total);
    RunGenEmitterTest(&Emitter::test1b49cc3347751dcb09a9, "test1b49cc3347751dcb09a9", passed, total);
    RunGenEmitterTest(&Emitter::testffe72c176661d48910bd, "testffe72c176661d48910bd", passed, total);
    RunGenEmitterTest(&Emitter::testf3f2d1d0e79f326b3d2f, "testf3f2d1d0e79f326b3d2f", passed, total);
    RunGenEmitterTest(&Emitter::testeb66a6725a958bb76923, "testeb66a6725a958bb76923", passed, total);
    RunGenEmitterTest(&Emitter::test0a8fc83bac630f116c86, "test0a8fc83bac630f116c86", passed, total);
    RunGenEmitterTest(&Emitter::test6c62ccca61f383967d91, "test6c62ccca61f383967d91", passed, total);
    RunGenEmitterTest(&Emitter::test6ba9e6495bef38087e7f, "test6ba9e6495bef38087e7f", passed, total);
    RunGenEmitterTest(&Emitter::test643e77d887ec7390c1c9, "test643e77d887ec7390c1c9", passed, total);
    RunGenEmitterTest(&Emitter::testa9725ffe34acd33603c4, "testa9725ffe34acd33603c4", passed, total);
    RunGenEmitterTest(&Emitter::test1a65df7ae90ac6ba1f22, "test1a65df7ae90ac6ba1f22", passed, total);
    RunGenEmitterTest(&Emitter::test7f982251b9a09ebb1059, "test7f982251b9a09ebb1059", passed, total);
    RunGenEmitterTest(&Emitter::test4b2138b3eafc346d6bd3, "test4b2138b3eafc346d6bd3", passed, total);
    RunGenEmitterTest(&Emitter::testa316e26e6b940d585005, "testa316e26e6b940d585005", passed, total);
    RunGenEmitterTest(&Emitter::testda2e0d2a6fd2a83cb298, "testda2e0d2a6fd2a83cb298", passed, total);
    RunGenEmitterTest(&Emitter::test54aac276524c0baaecd1, "test54aac276524c0baaecd1", passed, total);
    RunGenEmitterTest(&Emitter::testd1fb3fd39f4a2fda5c6a, "testd1fb3fd39f4a2fda5c6a", passed, total);
    RunGenEmitterTest(&Emitter::test4aaf20817b31a62e905f, "test4aaf20817b31a62e905f", passed, total);
    RunGenEmitterTest(&Emitter::test446f00cbeee81a34d936, "test446f00cbeee81a34d936", passed, total);
    RunGenEmitterTest(&Emitter::testb13663976ee52dac5370, "testb13663976ee52dac5370", passed, total);
    RunGenEmitterTest(&Emitter::testd245994f89c881efef06, "testd245994f89c881efef06", passed, total);
    RunGenEmitterTest(&Emitter::testb6e7c5ee86f3c0559fe3, "testb6e7c5ee86f3c0559fe3", passed, total);
    RunGenEmitterTest(&Emitter::testee28e38a088388ee5d9f, "testee28e38a088388ee5d9f", passed, total);
    RunGenEmitterTest(&Emitter::test9b40d68cb65d34934b50, "test9b40d68cb65d34934b50", passed, total);
    RunGenEmitterTest(&Emitter::test045fac0048d3cfc15d88, "test045fac0048d3cfc15d88", passed, total);
    RunGenEmitterTest(&Emitter::test85c3ad95991be29b3aff, "test85c3ad95991be29b3aff", passed, total);
    RunGenEmitterTest(&Emitter::test2abfed1da9a2ab365c18, "test2abfed1da9a2ab365c18", passed, total);
    RunGenEmitterTest(&Emitter::testa999e4a9d0199bf463ec, "testa999e4a9d0199bf463ec", passed, total);
    RunGenEmitterTest(&Emitter::test8452a26a810950ad47b2, "test8452a26a810950ad47b2", passed, total);
    RunGenEmitterTest(&Emitter::test1e2b3a261f435a0f6316, "test1e2b3a261f435a0f6316", passed, total);
    RunGenEmitterTest(&Emitter::test1423ee4cd0795e6ef646, "test1423ee4cd0795e6ef646", passed, total);
    RunGenEmitterTest(&Emitter::testf95070e81f8def8ceec9, "testf95070e81f8def8ceec9", passed, total);
    RunGenEmitterTest(&Emitter::testf533a12940040f665761, "testf533a12940040f665761", passed, total);
    RunGenEmitterTest(&Emitter::test51bd09a609a537b79c8a, "test51bd09a609a537b79c8a", passed, total);
    RunGenEmitterTest(&Emitter::testc10b994394aa86a1789b, "testc10b994394aa86a1789b", passed, total);
    RunGenEmitterTest(&Emitter::testc5de3511ef8fa1e5841e, "testc5de3511ef8fa1e5841e", passed, total);
    RunGenEmitterTest(&Emitter::test0e957613f266e5693f83, "test0e957613f266e5693f83", passed, total);
    RunGenEmitterTest(&Emitter::testf36b8fb2ec772e02a48c, "testf36b8fb2ec772e02a48c", passed, total);
    RunGenEmitterTest(&Emitter::test249b4ee9b0e6936bdfcf, "test249b4ee9b0e6936bdfcf", passed, total);
    RunGenEmitterTest(&Emitter::testdae669b09d29d1d05e81, "testdae669b09d29d1d05e81", passed, total);
    RunGenEmitterTest(&Emitter::testa3645755a60c69fe8af4, "testa3645755a60c69fe8af4", passed, total);
    RunGenEmitterTest(&Emitter::test5500c0af0cbb6a8efc04, "test5500c0af0cbb6a8efc04", passed, total);
    RunGenEmitterTest(&Emitter::test6528fe35c2b993cbd28b, "test6528fe35c2b993cbd28b", passed, total);
    RunGenEmitterTest(&Emitter::test072a41bf81e5b4dcd7d2, "test072a41bf81e5b4dcd7d2", passed, total);
    RunGenEmitterTest(&Emitter::test3b47e24ba197ca8f686b, "test3b47e24ba197ca8f686b", passed, total);
    RunGenEmitterTest(&Emitter::test938c1c274b5ebf36c6b2, "test938c1c274b5ebf36c6b2", passed, total);
    RunGenEmitterTest(&Emitter::testeea818614d4fbef183a8, "testeea818614d4fbef183a8", passed, total);
    RunGenEmitterTest(&Emitter::test18533eabe468baceac59, "test18533eabe468baceac59", passed, total);
    RunGenEmitterTest(&Emitter::teste239cd01ef68ce26375d, "teste239cd01ef68ce26375d", passed, total);
    RunGenEmitterTest(&Emitter::testd3a9b36f8218cd633402, "testd3a9b36f8218cd633402", passed, total);
    RunGenEmitterTest(&Emitter::testa899f4512569981104e8, "testa899f4512569981104e8", passed, total);
    RunGenEmitterTest(&Emitter::test230a8fd0d19c5e15963b, "test230a8fd0d19c5e15963b", passed, total);
    RunGenEmitterTest(&Emitter::test4fa431a3bea1c616f8d0, "test4fa431a3bea1c616f8d0", passed, total);
    RunGenEmitterTest(&Emitter::testdd05d8f0df4f1ba79b8f, "testdd05d8f0df4f1ba79b8f", passed, total);
    RunGenEmitterTest(&Emitter::test0cb381d8a384434a2646, "test0cb381d8a384434a2646", passed, total);
    RunGenEmitterTest(&Emitter::testeadefbe24693d510ac03, "testeadefbe24693d510ac03", passed, total);
    RunGenEmitterTest(&Emitter::test0b43e898410a9da3db1a, "test0b43e898410a9da3db1a", passed, total);
    RunGenEmitterTest(&Emitter::test450ed0f6d19326bab043, "test450ed0f6d19326bab043", passed, total);
    RunGenEmitterTest(&Emitter::test38b7c3f09ffb6f4e73a5, "test38b7c3f09ffb6f4e73a5", passed, total);
    RunGenEmitterTest(&Emitter::testab434195f62ee39997ae, "testab434195f62ee39997ae", passed, total);
    RunGenEmitterTest(&Emitter::testdce4c351d6c426d24f14, "testdce4c351d6c426d24f14", passed, total);
    RunGenEmitterTest(&Emitter::test2363249f46e86ae9bd64, "test2363249f46e86ae9bd64", passed, total);
    RunGenEmitterTest(&Emitter::test6225a910ac0a5ce7304f, "test6225a910ac0a5ce7304f", passed, total);
    RunGenEmitterTest(&Emitter::testf7898be54854e4587c54, "testf7898be54854e4587c54", passed, total);
    RunGenEmitterTest(&Emitter::test1ab55f98773e2e58c659, "test1ab55f98773e2e58c659", passed, total);
    RunGenEmitterTest(&Emitter::testf116d0bf1446d5e989db, "testf116d0bf1446d5e989db", passed, total);
    RunGenEmitterTest(&Emitter::test90df709c20ab5305b5b0, "test90df709c20ab5305b5b0", passed, total);
    RunGenEmitterTest(&Emitter::test37e041f7726819b98004, "test37e041f7726819b98004", passed, total);
    RunGenEmitterTest(&Emitter::test5c13c8d0762eb77abbbe, "test5c13c8d0762eb77abbbe", passed, total);
    RunGenEmitterTest(&Emitter::test3c9ec0009e080492d6a0, "test3c9ec0009e080492d6a0", passed, total);
    RunGenEmitterTest(&Emitter::testd016b2610a9701c799be, "testd016b2610a9701c799be", passed, total);
    RunGenEmitterTest(&Emitter::test12e858bf6ec981811cc8, "test12e858bf6ec981811cc8", passed, total);
    RunGenEmitterTest(&Emitter::test0e09fd8b6ac12a309a36, "test0e09fd8b6ac12a309a36", passed, total);
    RunGenEmitterTest(&Emitter::testca30dc12961feab24a33, "testca30dc12961feab24a33", passed, total);
    RunGenEmitterTest(&Emitter::test162ca62af5cdf9d02507, "test162ca62af5cdf9d02507", passed, total);
    RunGenEmitterTest(&Emitter::teste3a7658df7a81c1ce8e5, "teste3a7658df7a81c1ce8e5", passed, total);
    RunGenEmitterTest(&Emitter::test9dac5e4f0e78f96fcaad, "test9dac5e4f0e78f96fcaad", passed, total);
    RunGenEmitterTest(&Emitter::testaf2e952791b73c6bf78c, "testaf2e952791b73c6bf78c", passed, total);
    RunGenEmitterTest(&Emitter::test82b161574e0926b9c854, "test82b161574e0926b9c854", passed, total);
    RunGenEmitterTest(&Emitter::test13c3f29be0e6b7bc92b1, "test13c3f29be0e6b7bc92b1", passed, total);
    RunGenEmitterTest(&Emitter::teste87c7e9ce16fd2ac5f0e, "teste87c7e9ce16fd2ac5f0e", passed, total);
    RunGenEmitterTest(&Emitter::testeed7f7f244221932d32f, "testeed7f7f244221932d32f", passed, total);
    RunGenEmitterTest(&Emitter::test31a5563bfa532571339f, "test31a5563bfa532571339f", passed, total);
    RunGenEmitterTest(&Emitter::test502ab92aab5195ff4759, "test502ab92aab5195ff4759", passed, total);
    RunGenEmitterTest(&Emitter::testdbe2ce5bc02435009b2c, "testdbe2ce5bc02435009b2c", passed, total);
    RunGenEmitterTest(&Emitter::test06b8d2cc9bbb233d55d5, "test06b8d2cc9bbb233d55d5", passed, total);
    RunGenEmitterTest(&Emitter::test9adf48ae5d6dff37e89a, "test9adf48ae5d6dff37e89a", passed, total);
    RunGenEmitterTest(&Emitter::test398282871dcc1c7f8dbe, "test398282871dcc1c7f8dbe", passed, total);
    RunGenEmitterTest(&Emitter::test2e2b3c35732210898be1, "test2e2b3c35732210898be1", passed, total);
    RunGenEmitterTest(&Emitter::test155697ae715940b6d804, "test155697ae715940b6d804", passed, total);
    RunGenEmitterTest(&Emitter::testb01c87881d846bb10ecd, "testb01c87881d846bb10ecd", passed, total);
    RunGenEmitterTest(&Emitter::test84a9d3d2ae4eaacc9c98, "test84a9d3d2ae4eaacc9c98", passed, total);
    RunGenEmitterTest(&Emitter::testb836cd5dc876cf6eb204, "testb836cd5dc876cf6eb204", passed, total);
    RunGenEmitterTest(&Emitter::test4aa81dc715c5e5c53de1, "test4aa81dc715c5e5c53de1", passed, total);
    RunGenEmitterTest(&Emitter::testfa8ba9d405de1af93537, "testfa8ba9d405de1af93537", passed, total);
    RunGenEmitterTest(&Emitter::test3c07c460dae114d3f278, "test3c07c460dae114d3f278", passed, total);
    RunGenEmitterTest(&Emitter::testf513330011b92283e713, "testf513330011b92283e713", passed, total);
    RunGenEmitterTest(&Emitter::test88667a8fe856748b4dc6, "test88667a8fe856748b4dc6", passed, total);
    RunGenEmitterTest(&Emitter::test7cbc7ae88a6b60d3cb54, "test7cbc7ae88a6b60d3cb54", passed, total);
    RunGenEmitterTest(&Emitter::test8b126d00ee878273f3e9, "test8b126d00ee878273f3e9", passed, total);
    RunGenEmitterTest(&Emitter::test414dcec883b2fb2668d9, "test414dcec883b2fb2668d9", passed, total);
    RunGenEmitterTest(&Emitter::test41754fe2ab40560f1afe, "test41754fe2ab40560f1afe", passed, total);
    RunGenEmitterTest(&Emitter::test39b7a9ee0ccb5580ef60, "test39b7a9ee0ccb5580ef60", passed, total);
    RunGenEmitterTest(&Emitter::test1f4cbfdb2f53d041fb74, "test1f4cbfdb2f53d041fb74", passed, total);
    RunGenEmitterTest(&Emitter::testc20612e8922a8eeba24d, "testc20612e8922a8eeba24d", passed, total);
    RunGenEmitterTest(&Emitter::test693f48133cf726f1e05c, "test693f48133cf726f1e05c", passed, total);
    RunGenEmitterTest(&Emitter::test12b26bfed7a24736dd8b, "test12b26bfed7a24736dd8b", passed, total);
    RunGenEmitterTest(&Emitter::test41d6ec7f045897841e9c, "test41d6ec7f045897841e9c", passed, total);
    RunGenEmitterTest(&Emitter::test2bc8d1d6e4ec042ede3e, "test2bc8d1d6e4ec042ede3e", passed, total);
    RunGenEmitterTest(&Emitter::test647ff876c844ad7540ff, "test647ff876c844ad7540ff", passed, total);
    RunGenEmitterTest(&Emitter::test844c1bcb896dde8ea51b, "test844c1bcb896dde8ea51b", passed, total);
    RunGenEmitterTest(&Emitter::testbcf04a5174e8505d1891, "testbcf04a5174e8505d1891", passed, total);
    RunGenEmitterTest(&Emitter::testdb6c8ca130035d7a271f, "testdb6c8ca130035d7a271f", passed, total);
    RunGenEmitterTest(&Emitter::testa0db96f3d93a255f2201, "testa0db96f3d93a255f2201", passed, total);
    RunGenEmitterTest(&Emitter::test4fb3eaac714942122715, "test4fb3eaac714942122715", passed, total);
    RunGenEmitterTest(&Emitter::test634678f31daa20127d6c, "test634678f31daa20127d6c", passed, total);
    RunGenEmitterTest(&Emitter::test1dd395a1149e46dcc208, "test1dd395a1149e46dcc208", passed, total);
    RunGenEmitterTest(&Emitter::testebc585cbde90d10a0af1, "testebc585cbde90d10a0af1", passed, total);
    RunGenEmitterTest(&Emitter::testd1ecec189e73f8932485, "testd1ecec189e73f8932485", passed, total);
    RunGenEmitterTest(&Emitter::testf8fc72597f0a41b22daa, "testf8fc72597f0a41b22daa", passed, total);
    RunGenEmitterTest(&Emitter::testd01959c5c228946c8759, "testd01959c5c228946c8759", passed, total);
    RunGenEmitterTest(&Emitter::teste8236a9672d9244ca486, "teste8236a9672d9244ca486", passed, total);
    RunGenEmitterTest(&Emitter::test22bd5d24dbd3f0670f97, "test22bd5d24dbd3f0670f97", passed, total);
    RunGenEmitterTest(&Emitter::test0f2c6cac0ce0e624eb08, "test0f2c6cac0ce0e624eb08", passed, total);
    RunGenEmitterTest(&Emitter::test2ed2eef7f03696ca3c94, "test2ed2eef7f03696ca3c94", passed, total);
    RunGenEmitterTest(&Emitter::test6fe33177e10d14328a14, "test6fe33177e10d14328a14", passed, total);
    RunGenEmitterTest(&Emitter::test0ea79e93c3439d90bdb8, "test0ea79e93c3439d90bdb8", passed, total);
    RunGenEmitterTest(&Emitter::test2ab970621d7e037153c9, "test2ab970621d7e037153c9", passed, total);
    RunGenEmitterTest(&Emitter::test4229059ec3d639faf4b2, "test4229059ec3d639faf4b2", passed, total);
    RunGenEmitterTest(&Emitter::test6df809b4712b73c8577f, "test6df809b4712b73c8577f", passed, total);
    RunGenEmitterTest(&Emitter::test37a0a34b2bef743d8241, "test37a0a34b2bef743d8241", passed, total);
    RunGenEmitterTest(&Emitter::testd6c37c1e50617419a37d, "testd6c37c1e50617419a37d", passed, total);
    RunGenEmitterTest(&Emitter::test2fb71689fb176533159c, "test2fb71689fb176533159c", passed, total);
    RunGenEmitterTest(&Emitter::testa91d7999dd9b43fb5827, "testa91d7999dd9b43fb5827", passed, total);
    RunGenEmitterTest(&Emitter::test8dfd4533e2891d3861ec, "test8dfd4533e2891d3861ec", passed, total);
    RunGenEmitterTest(&Emitter::test0a9b0c29dfcf4f071eb9, "test0a9b0c29dfcf4f071eb9", passed, total);
    RunGenEmitterTest(&Emitter::test3882a796318e573b115d, "test3882a796318e573b115d", passed, total);
    RunGenEmitterTest(&Emitter::test1ade39a1572a12eda7b8, "test1ade39a1572a12eda7b8", passed, total);
    RunGenEmitterTest(&Emitter::testd308c7e19671725523cd, "testd308c7e19671725523cd", passed, total);
    RunGenEmitterTest(&Emitter::test1ac58dc569ce7b4eebdf, "test1ac58dc569ce7b4eebdf", passed, total);
    RunGenEmitterTest(&Emitter::test5fe4c34b163d0efa12d5, "test5fe4c34b163d0efa12d5", passed, total);
    RunGenEmitterTest(&Emitter::test78433ff7edf3224ce58b, "test78433ff7edf3224ce58b", passed, total);
    RunGenEmitterTest(&Emitter::test089ed50cafddf39653d6, "test089ed50cafddf39653d6", passed, total);
    RunGenEmitterTest(&Emitter::test809723ec7bdea45cf562, "test809723ec7bdea45cf562", passed, total);
    RunGenEmitterTest(&Emitter::test953cf821d2acc7898582, "test953cf821d2acc7898582", passed, total);
    RunGenEmitterTest(&Emitter::test0f1db947e4627596eace, "test0f1db947e4627596eace", passed, total);
    RunGenEmitterTest(&Emitter::test52f3b0674f30d955eea7, "test52f3b0674f30d955eea7", passed, total);
    RunGenEmitterTest(&Emitter::testf95a488631e07f6ca914, "testf95a488631e07f6ca914", passed, total);
    RunGenEmitterTest(&Emitter::test9b7a916f4c1e2a1ae6ee, "test9b7a916f4c1e2a1ae6ee", passed, total);
    RunGenEmitterTest(&Emitter::test54ea34948e814ef79607, "test54ea34948e814ef79607", passed, total);
    RunGenEmitterTest(&Emitter::testcef62af7508e0a1e3ee3, "testcef62af7508e0a1e3ee3", passed, total);
    RunGenEmitterTest(&Emitter::testffcfb27f0c904fae7833, "testffcfb27f0c904fae7833", passed, total);
    RunGenEmitterTest(&Emitter::test31ed1460205bbc5a4a68, "test31ed1460205bbc5a4a68", passed, total);
    RunGenEmitterTest(&Emitter::test7d04c8d923b046159db0, "test7d04c8d923b046159db0", passed, total);
    RunGenEmitterTest(&Emitter::test31b396851023614cf9fd, "test31b396851023614cf9fd", passed, total);
    RunGenEmitterTest(&Emitter::test5befd800aa07d83e2df7, "test5befd800aa07d83e2df7", passed, total);
    RunGenEmitterTest(&Emitter::test2df4f378f687fd80b98c, "test2df4f378f687fd80b98c", passed, total);
    RunGenEmitterTest(&Emitter::test232f5aa6ea7e85e186c4, "test232f5aa6ea7e85e186c4", passed, total);
    RunGenEmitterTest(&Emitter::test011421ad7da175099088, "test011421ad7da175099088", passed, total);
    RunGenEmitterTest(&Emitter::testf50217feca0ae03a0b03, "testf50217feca0ae03a0b03", passed, total);
    RunGenEmitterTest(&Emitter::teste4b49044063dd3c8a7ff, "teste4b49044063dd3c8a7ff", passed, total);
    RunGenEmitterTest(&Emitter::testbe9295522ec1f0bc9de5, "testbe9295522ec1f0bc9de5", passed, total);
    RunGenEmitterTest(&Emitter::test7a18055f1e3a49f93d40, "test7a18055f1e3a49f93d40", passed, total);
    RunGenEmitterTest(&Emitter::testd8eebabad65b8ef02375, "testd8eebabad65b8ef02375", passed, total);
    RunGenEmitterTest(&Emitter::test948f36e3182f3c1aa800, "test948f36e3182f3c1aa800", passed, total);
    RunGenEmitterTest(&Emitter::testde02b69600e5931c39ab, "testde02b69600e5931c39ab", passed, total);
    RunGenEmitterTest(&Emitter::test5fdab21609e7017c3b86, "test5fdab21609e7017c3b86", passed, total);
    RunGenEmitterTest(&Emitter::testfb52965f57b912ec23a4, "testfb52965f57b912ec23a4", passed, total);
    RunGenEmitterTest(&Emitter::testa3f220ead85b78154f89, "testa3f220ead85b78154f89", passed, total);
    RunGenEmitterTest(&Emitter::test0ef1b1c26e8a1fa34ccd, "test0ef1b1c26e8a1fa34ccd", passed, total);
    RunGenEmitterTest(&Emitter::test7661db62a921285da885, "test7661db62a921285da885", passed, total);
    RunGenEmitterTest(&Emitter::test9382f466be3e19ca395f, "test9382f466be3e19ca395f", passed, total);
    RunGenEmitterTest(&Emitter::test33c4f45355dc7df2e2a8, "test33c4f45355dc7df2e2a8", passed, total);
    RunGenEmitterTest(&Emitter::testc55c40f32c34c890acce, "testc55c40f32c34c890acce", passed, total);
    RunGenEmitterTest(&Emitter::testb06ba64c5895f218175d, "testb06ba64c5895f218175d", passed, total);
    RunGenEmitterTest(&Emitter::test14adb5374833871b2d0c, "test14adb5374833871b2d0c", passed, total);
    RunGenEmitterTest(&Emitter::test7ff7826c0f0563ce5a65, "test7ff7826c0f0563ce5a65", passed, total);
    RunGenEmitterTest(&Emitter::test394e607327447b08e729, "test394e607327447b08e729", passed, total);
    RunGenEmitterTest(&Emitter::testdf03e1437e901976c2c8, "testdf03e1437e901976c2c8", passed, total);
    RunGenEmitterTest(&Emitter::test77467fcda467dd063050, "test77467fcda467dd063050", passed, total);
    RunGenEmitterTest(&Emitter::test5bcea73651331a2357d0, "test5bcea73651331a2357d0", passed, total);
    RunGenEmitterTest(&Emitter::test2956b3f097a16a4cd951, "test2956b3f097a16a4cd951", passed, total);
    RunGenEmitterTest(&Emitter::test3170422d0cad24cd602a, "test3170422d0cad24cd602a", passed, total);
    RunGenEmitterTest(&Emitter::testb37f0cd80f138e8f2622, "testb37f0cd80f138e8f2622", passed, total);
    RunGenEmitterTest(&Emitter::test3e00cce71da4636fa1f7, "test3e00cce71da4636fa1f7", passed, total);
    RunGenEmitterTest(&Emitter::testfd184c04759685f21abb, "testfd184c04759685f21abb", passed, total);
    RunGenEmitterTest(&Emitter::test14ab4965eff0a569da16, "test14ab4965eff0a569da16", passed, total);
    RunGenEmitterTest(&Emitter::test271811f2df7210366780, "test271811f2df7210366780", passed, total);
    RunGenEmitterTest(&Emitter::testfcfe8657dffc21f6cd45, "testfcfe8657dffc21f6cd45", passed, total);
    RunGenEmitterTest(&Emitter::test609e44eab4ab95f31e33, "test609e44eab4ab95f31e33", passed, total);
    RunGenEmitterTest(&Emitter::test7841fc715275a45a2770, "test7841fc715275a45a2770", passed, total);
    RunGenEmitterTest(&Emitter::test662c03de87ca40bd943e, "test662c03de87ca40bd943e", passed, total);
    RunGenEmitterTest(&Emitter::test0a9475ec3c946fe11991, "test0a9475ec3c946fe11991", passed, total);
    RunGenEmitterTest(&Emitter::test94d28ebdbee90f430eb1, "test94d28ebdbee90f430eb1", passed, total);
    RunGenEmitterTest(&Emitter::testd5035afc82e23b67ce03, "testd5035afc82e23b67ce03", passed, total);
    RunGenEmitterTest(&Emitter::testcc9788c342da4454758f, "testcc9788c342da4454758f", passed, total);
    RunGenEmitterTest(&Emitter::test140974453293fdb1155d, "test140974453293fdb1155d", passed, total);
    RunGenEmitterTest(&Emitter::testdc0b80a131730e98d735, "testdc0b80a131730e98d735", passed, total);
    RunGenEmitterTest(&Emitter::test1c5225b07d746c2bd331, "test1c5225b07d746c2bd331", passed, total);
    RunGenEmitterTest(&Emitter::testa49a0be204cd2b57f17b, "testa49a0be204cd2b57f17b", passed, total);
    RunGenEmitterTest(&Emitter::testbe08cc0a08cf2cb5e7ec, "testbe08cc0a08cf2cb5e7ec", passed, total);
    RunGenEmitterTest(&Emitter::test4d2a2e12689655edd77c, "test4d2a2e12689655edd77c", passed, total);
    RunGenEmitterTest(&Emitter::test4d4a25a54401f0282ceb, "test4d4a25a54401f0282ceb", passed, total);
    RunGenEmitterTest(&Emitter::test91f55feebb012ce89a93, "test91f55feebb012ce89a93", passed, total);
    RunGenEmitterTest(&Emitter::test1f3d0b19c6a346b087e0, "test1f3d0b19c6a346b087e0", passed, total);
    RunGenEmitterTest(&Emitter::test4e26682c2daf8ded04a6, "test4e26682c2daf8ded04a6", passed, total);
    RunGenEmitterTest(&Emitter::test6f24e6df03922bba0d8a, "test6f24e6df03922bba0d8a", passed, total);
    RunGenEmitterTest(&Emitter::test60849eca7dc178908ff1, "test60849eca7dc178908ff1", passed, total);
    RunGenEmitterTest(&Emitter::test28b7db2ac68bb806e143, "test28b7db2ac68bb806e143", passed, total);
    RunGenEmitterTest(&Emitter::test8db156db7065942bc260, "test8db156db7065942bc260", passed, total);
    RunGenEmitterTest(&Emitter::teste240aced6e2292a9b091, "teste240aced6e2292a9b091", passed, total);
    RunGenEmitterTest(&Emitter::test468628a845426ce4a106, "test468628a845426ce4a106", passed, total);
    RunGenEmitterTest(&Emitter::testa3a2d467766b74acd6fd, "testa3a2d467766b74acd6fd", passed, total);
    RunGenEmitterTest(&Emitter::test5bf63d8ed606d688d869, "test5bf63d8ed606d688d869", passed, total);
    RunGenEmitterTest(&Emitter::test0d35c1487237ba7d8bdc, "test0d35c1487237ba7d8bdc", passed, total);
    RunGenEmitterTest(&Emitter::testb1fddc2897760d60e733, "testb1fddc2897760d60e733", passed, total);
    RunGenEmitterTest(&Emitter::testbaf845554a46f088bf71, "testbaf845554a46f088bf71", passed, total);
    RunGenEmitterTest(&Emitter::test6383f28d62ad9ce3c075, "test6383f28d62ad9ce3c075", passed, total);
    RunGenEmitterTest(&Emitter::test88a4c1cc11b99a61eccd, "test88a4c1cc11b99a61eccd", passed, total);
    RunGenEmitterTest(&Emitter::test4716a2cf58a70705987b, "test4716a2cf58a70705987b", passed, total);
    RunGenEmitterTest(&Emitter::test75222084929bd0f9d38f, "test75222084929bd0f9d38f", passed, total);
    RunGenEmitterTest(&Emitter::test2fb23c79eec625216523, "test2fb23c79eec625216523", passed, total);
    RunGenEmitterTest(&Emitter::testb1699a6b7c5ded480677, "testb1699a6b7c5ded480677", passed, total);
    RunGenEmitterTest(&Emitter::testd7de744a20ca1dc099db, "testd7de744a20ca1dc099db", passed, total);
    RunGenEmitterTest(&Emitter::test900b2dcf20981b44ea65, "test900b2dcf20981b44ea65", passed, total);
    RunGenEmitterTest(&Emitter::test20cc330b6d1171584aed, "test20cc330b6d1171584aed", passed, total);
    RunGenEmitterTest(&Emitter::test5ea8e3642fab864fb09d, "test5ea8e3642fab864fb09d", passed, total);
    RunGenEmitterTest(&Emitter::test42e21cbc65f534972ead, "test42e21cbc65f534972ead", passed, total);
    RunGenEmitterTest(&Emitter::test14e3b5dca1d7a5a0c957, "test14e3b5dca1d7a5a0c957", passed, total);
    RunGenEmitterTest(&Emitter::test9bd4800a58394b172738, "test9bd4800a58394b172738", passed, total);
    RunGenEmitterTest(&Emitter::testb715a2b66987a872ced8, "testb715a2b66987a872ced8", passed, total);
    RunGenEmitterTest(&Emitter::teste9b56880009cc6899131, "teste9b56880009cc6899131", passed, total);
    RunGenEmitterTest(&Emitter::test21f96f767e38471c9d4d, "test21f96f767e38471c9d4d", passed, total);
    RunGenEmitterTest(&Emitter::testa8aebba05fc1858c0a6c, "testa8aebba05fc1858c0a6c", passed, total);
    RunGenEmitterTest(&Emitter::teste6e7442377049b17ee9e, "teste6e7442377049b17ee9e", passed, total);
    RunGenEmitterTest(&Emitter::test428b593e283163fee752, "test428b593e283163fee752", passed, total);
    RunGenEmitterTest(&Emitter::test0b6c63323da4bf9798c2, "test0b6c63323da4bf9798c2", passed, total);
    RunGenEmitterTest(&Emitter::test0f4c45c39fe39dfc8a1d, "test0f4c45c39fe39dfc8a1d", passed, total);
    RunGenEmitterTest(&Emitter::testb8043a7ae1de42dd81db, "testb8043a7ae1de42dd81db", passed, total);
    RunGenEmitterTest(&Emitter::test4d9b278579ffb76fc56d, "test4d9b278579ffb76fc56d", passed, total);
    RunGenEmitterTest(&Emitter::test672fc8b6d281f82b9332, "test672fc8b6d281f82b9332", passed, total);
    RunGenEmitterTest(&Emitter::testb406d378fa0df952b051, "testb406d378fa0df952b051", passed, total);
    RunGenEmitterTest(&Emitter::test68a227d03f20863f37e4, "test68a227d03f20863f37e4", passed, total);
    RunGenEmitterTest(&Emitter::testcee8582fd340377bda46, "testcee8582fd340377bda46", passed, total);
    RunGenEmitterTest(&Emitter::test06fd48e8c86baf6fc05b, "test06fd48e8c86baf6fc05b", passed, total);
    RunGenEmitterTest(&Emitter::test70b4ccbf71c0716bf8e4, "test70b4ccbf71c0716bf8e4", passed, total);
    RunGenEmitterTest(&Emitter::test449c2b349be8da36682b, "test449c2b349be8da36682b", passed, total);
    RunGenEmitterTest(&Emitter::test9620fa69718e3b4fe391, "test9620fa69718e3b4fe391", passed, total);
    RunGenEmitterTest(&Emitter::test3faaebe701bea6f8ee39, "test3faaebe701bea6f8ee39", passed, total);
    RunGenEmitterTest(&Emitter::test763ee61808091c7a354d, "test763ee61808091c7a354d", passed, total);
    RunGenEmitterTest(&Emitter::test81b0d6b575228cde91e5, "test81b0d6b575228cde91e5", passed, total);
    RunGenEmitterTest(&Emitter::testb607ae3c5d560092e37b, "testb607ae3c5d560092e37b", passed, total);
    RunGenEmitterTest(&Emitter::testa53c54726737df14a5dd, "testa53c54726737df14a5dd", passed, total);
    RunGenEmitterTest(&Emitter::test071d73b309a1365e0b07, "test071d73b309a1365e0b07", passed, total);
    RunGenEmitterTest(&Emitter::testf8f45511528fa28cddcb, "testf8f45511528fa28cddcb", passed, total);
    RunGenEmitterTest(&Emitter::testabdd2bf3bdf550e3dd60, "testabdd2bf3bdf550e3dd60", passed, total);
    RunGenEmitterTest(&Emitter::test53424b35498a73fbede9, "test53424b35498a73fbede9", passed, total);
    RunGenEmitterTest(&Emitter::testf0c6c1a1afced157d6a5, "testf0c6c1a1afced157d6a5", passed, total);
    RunGenEmitterTest(&Emitter::teste45dbac33918e0fee74f, "teste45dbac33918e0fee74f", passed, total);
    RunGenEmitterTest(&Emitter::test903c7ab3d09d4323107f, "test903c7ab3d09d4323107f", passed, total);
    RunGenEmitterTest(&Emitter::test5d39d351680dba4be04b, "test5d39d351680dba4be04b", passed, total);
    RunGenEmitterTest(&Emitter::testaa1e8d6d4385aab47bcd, "testaa1e8d6d4385aab47bcd", passed, total);
    RunGenEmitterTest(&Emitter::test9bd238b748ced1db588b, "test9bd238b748ced1db588b", passed, total);
    RunGenEmitterTest(&Emitter::testec1cdffaae8842854947, "testec1cdffaae8842854947", passed, total);
    RunGenEmitterTest(&Emitter::test30727d97de63c1ad395a, "test30727d97de63c1ad395a", passed, total);
    RunGenEmitterTest(&Emitter::test7adafdc8be65a5d610bf, "test7adafdc8be65a5d610bf", passed, total);
#else // YAML_GEN_TESTS
   (void)passed; (void)total;
#endif // YAML_GEN_TESTS
}
