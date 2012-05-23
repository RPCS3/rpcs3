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
}
void RunGenEmitterTests(int& passed, int& total)
{
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
}
