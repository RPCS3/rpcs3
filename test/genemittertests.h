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
    RunGenEmitterTest(&Emitter::test0c7bb03fbd6b52ea3ad6, "test0c7bb03fbd6b52ea3ad6", passed, total);
    RunGenEmitterTest(&Emitter::testb819efb5742c1176df98, "testb819efb5742c1176df98", passed, total);
    RunGenEmitterTest(&Emitter::test1f7b7cd5a13070c723d3, "test1f7b7cd5a13070c723d3", passed, total);
    RunGenEmitterTest(&Emitter::test32126a88cb2b7311e779, "test32126a88cb2b7311e779", passed, total);
    RunGenEmitterTest(&Emitter::testd7f952713bde5ce2f9e7, "testd7f952713bde5ce2f9e7", passed, total);
    RunGenEmitterTest(&Emitter::test5030b4f2d1efb798f320, "test5030b4f2d1efb798f320", passed, total);
    RunGenEmitterTest(&Emitter::testb9015537b9a9e09b8ec8, "testb9015537b9a9e09b8ec8", passed, total);
    RunGenEmitterTest(&Emitter::test03229f6d33fa9007a65d, "test03229f6d33fa9007a65d", passed, total);
}
