namespace Emitter {
TEST testb3471d40ef2dadee13be(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo\n";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST testb1f8cfb6083c3fc130aa(YAML::Emitter& out)
{
    out << "foo\n";
    out << YAML::EndDoc;

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST testa2f381bb144cf8886efe(YAML::Emitter& out)
{
    out << YAML::BeginDoc;
    out << "foo\n";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
TEST test29a80ae92b2f00fa1d06(YAML::Emitter& out)
{
    out << "foo\n";

    HANDLE(out.c_str());
    EXPECT_DOC_START();
    EXPECT_SCALAR("!", 0, "foo\n");
    EXPECT_DOC_END();
    DONE();
}
}
void RunGenEmitterTests(int& passed, int& total)
{
    RunGenEmitterTest(&Emitter::testb3471d40ef2dadee13be, "testb3471d40ef2dadee13be", passed, total);
    RunGenEmitterTest(&Emitter::testb1f8cfb6083c3fc130aa, "testb1f8cfb6083c3fc130aa", passed, total);
    RunGenEmitterTest(&Emitter::testa2f381bb144cf8886efe, "testa2f381bb144cf8886efe", passed, total);
    RunGenEmitterTest(&Emitter::test29a80ae92b2f00fa1d06, "test29a80ae92b2f00fa1d06", passed, total);
}
