#include "tests.h"
#include "yaml.h"

namespace Test
{
	namespace Emitter {
		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// correct emitting

		void SimpleScalar(YAML::Emitter& out, std::string& desiredOutput) {
			out << "Hello, World!";
			desiredOutput = "--- Hello, World!";
		}
		
		void SimpleSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginSeq;
			out << "eggs";
			out << "bread";
			out << "milk";
			out << YAML::EndSeq;

			desiredOutput = "---\n- eggs\n- bread\n- milk";
		}
		
		void SimpleFlowSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::Flow;
			out << YAML::BeginSeq;
			out << "Larry";
			out << "Curly";
			out << "Moe";
			out << YAML::EndSeq;
			
			desiredOutput = "--- [Larry, Curly, Moe]";
		}
		
		void EmptyFlowSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::Flow;
			out << YAML::BeginSeq;
			out << YAML::EndSeq;
			
			desiredOutput = "--- []";
		}
		
		void NestedBlockSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginSeq;
			out << "item 1";
			out << YAML::BeginSeq << "subitem 1" << "subitem 2" << YAML::EndSeq;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- item 1\n-\n  - subitem 1\n  - subitem 2";
		}
		
		void NestedFlowSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginSeq;
			out << "one";
			out << YAML::Flow << YAML::BeginSeq << "two" << "three" << YAML::EndSeq;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- one\n- [two, three]";
		}

		void SimpleMap(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginMap;
			out << YAML::Key << "name";
			out << YAML::Value << "Ryan Braun";
			out << YAML::Key << "position";
			out << YAML::Value << "3B";
			out << YAML::EndMap;

			desiredOutput = "---\nname: Ryan Braun\nposition: 3B";
		}
		
		void SimpleFlowMap(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::Flow;
			out << YAML::BeginMap;
			out << YAML::Key << "shape";
			out << YAML::Value << "square";
			out << YAML::Key << "color";
			out << YAML::Value << "blue";
			out << YAML::EndMap;
			
			desiredOutput = "--- {shape: square, color: blue}";
		}
		
		void MapAndList(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginMap;
			out << YAML::Key << "name";
			out << YAML::Value << "Barack Obama";
			out << YAML::Key << "children";
			out << YAML::Value << YAML::BeginSeq << "Sasha" << "Malia" << YAML::EndSeq;
			out << YAML::EndMap;

			desiredOutput = "---\nname: Barack Obama\nchildren:\n  - Sasha\n  - Malia";
		}
		
		void ListAndMap(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginSeq;
			out << "item 1";
			out << YAML::BeginMap;
			out << YAML::Key << "pens" << YAML::Value << 8;
			out << YAML::Key << "pencils" << YAML::Value << 14;
			out << YAML::EndMap;
			out << "item 2";
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- item 1\n-\n  pens: 8\n  pencils: 14\n- item 2";
		}

		void NestedBlockMap(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginMap;
			out << YAML::Key << "name";
			out << YAML::Value << "Fred";
			out << YAML::Key << "grades";
			out << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "algebra" << YAML::Value << "A";
			out << YAML::Key << "physics" << YAML::Value << "C+";
			out << YAML::Key << "literature" << YAML::Value << "B";
			out << YAML::EndMap;
			out << YAML::EndMap;
			
			desiredOutput = "---\nname: Fred\ngrades:\n  algebra: A\n  physics: C+\n  literature: B";
		}

		void NestedFlowMap(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::Flow;
			out << YAML::BeginMap;
			out << YAML::Key << "name";
			out << YAML::Value << "Fred";
			out << YAML::Key << "grades";
			out << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "algebra" << YAML::Value << "A";
			out << YAML::Key << "physics" << YAML::Value << "C+";
			out << YAML::Key << "literature" << YAML::Value << "B";
			out << YAML::EndMap;
			out << YAML::EndMap;
			
			desiredOutput = "--- {name: Fred, grades: {algebra: A, physics: C+, literature: B}}";
		}
		
		void MapListMix(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginMap;
			out << YAML::Key << "name";
			out << YAML::Value << "Bob";
			out << YAML::Key << "position";
			out << YAML::Value;
			out << YAML::Flow << YAML::BeginSeq << 2 << 4 << YAML::EndSeq;
			out << YAML::Key << "invincible" << YAML::Value << YAML::OnOffBool << false;
			out << YAML::EndMap;
			
			desiredOutput = "---\nname: Bob\nposition: [2, 4]\ninvincible: off";
		}

		void SimpleLongKey(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::LongKey;
			out << YAML::BeginMap;
			out << YAML::Key << "height";
			out << YAML::Value << "5'9\"";
			out << YAML::Key << "weight";
			out << YAML::Value << 145;
			out << YAML::EndMap;
			
			desiredOutput = "---\n? height\n: 5'9\"\n? weight\n: 145";
		}
		
		void SingleLongKey(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "age";
			out << YAML::Value << "24";
			out << YAML::LongKey << YAML::Key << "height";
			out << YAML::Value << "5'9\"";
			out << YAML::Key << "weight";
			out << YAML::Value << 145;
			out << YAML::EndMap;
			
			desiredOutput = "---\nage: 24\n? height\n: 5'9\"\nweight: 145";
		}
		
		void ComplexLongKey(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::LongKey;
			out << YAML::BeginMap;
			out << YAML::Key << YAML::BeginSeq << 1 << 3 << YAML::EndSeq;
			out << YAML::Value << "monster";
			out << YAML::Key << YAML::Flow << YAML::BeginSeq << 2 << 0 << YAML::EndSeq;
			out << YAML::Value << "demon";
			out << YAML::EndMap;
			
			desiredOutput = "---\n?\n  - 1\n  - 3\n: monster\n? [2, 0]\n: demon";
		}

		void AutoLongKey(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << YAML::BeginSeq << 1 << 3 << YAML::EndSeq;
			out << YAML::Value << "monster";
			out << YAML::Key << YAML::Flow << YAML::BeginSeq << 2 << 0 << YAML::EndSeq;
			out << YAML::Value << "demon";
			out << YAML::Key << "the origin";
			out << YAML::Value << "angel";
			out << YAML::EndMap;
			
			desiredOutput = "---\n?\n  - 1\n  - 3\n: monster\n? [2, 0]\n: demon\nthe origin: angel";
		}
		
		void ScalarFormat(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << "simple scalar";
			out << YAML::SingleQuoted << "explicit single-quoted scalar";
			out << YAML::DoubleQuoted << "explicit double-quoted scalar";
			out << "auto-detected\ndouble-quoted scalar";
			out << "a non-\"auto-detected\" double-quoted scalar";
			out << YAML::Literal << "literal scalar\nthat may span\nmany, many\nlines and have \"whatever\" crazy\tsymbols that we like";
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- simple scalar\n- 'explicit single-quoted scalar'\n- \"explicit double-quoted scalar\"\n- \"auto-detected\\x0adouble-quoted scalar\"\n- a non-\"auto-detected\" double-quoted scalar\n- |\n  literal scalar\n  that may span\n  many, many\n  lines and have \"whatever\" crazy\tsymbols that we like";
		}

		void AutoLongKeyScalar(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << YAML::Literal << "multi-line\nscalar";
			out << YAML::Value << "and its value";
			out << YAML::EndMap;
			
			desiredOutput = "---\n? |\n  multi-line\n  scalar\n: and its value";
		}
		
		void LongKeyFlowMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow;
			out << YAML::BeginMap;
			out << YAML::Key << "simple key";
			out << YAML::Value << "and value";
			out << YAML::LongKey << YAML::Key << "long key";
			out << YAML::Value << "and its value";
			out << YAML::EndMap;
			
			desiredOutput = "--- {simple key: and value, ? long key: and its value}";
		}

		void BlockMapAsKey(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key;
			out << YAML::BeginMap;
			out << YAML::Key << "key" << YAML::Value << "value";
			out << YAML::Key << "next key" << YAML::Value << "next value";
			out << YAML::EndMap;
			out << YAML::Value;
			out << "total value";
			out << YAML::EndMap;
			
			desiredOutput = "---\n?\n  key: value\n  next key: next value\n: total value";
		}
		
		void AliasAndAnchor(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::Anchor("fred");
			out << YAML::BeginMap;
			out << YAML::Key << "name" << YAML::Value << "Fred";
			out << YAML::Key << "age" << YAML::Value << 42;
			out << YAML::EndMap;
			out << YAML::Alias("fred");
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- &fred\n  name: Fred\n  age: 42\n- *fred";
		}

		void AliasAndAnchorWithNull(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::Anchor("fred") << YAML::Null;
			out << YAML::Alias("fred");
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- &fred ~\n- *fred";
		}
		
		void SimpleVerbatimTag(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::VerbatimTag("!foo") << "bar";
			
			desiredOutput = "--- !<!foo> bar";
		}

		void VerbatimTagInBlockSeq(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::VerbatimTag("!foo") << "bar";
			out << "baz";
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- !<!foo> bar\n- baz";
		}

		void VerbatimTagInFlowSeq(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginSeq;
			out << YAML::VerbatimTag("!foo") << "bar";
			out << "baz";
			out << YAML::EndSeq;
			
			desiredOutput = "--- [!<!foo> bar, baz]";
		}

		void VerbatimTagInFlowSeqWithNull(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginSeq;
			out << YAML::VerbatimTag("!foo") << YAML::Null;
			out << "baz";
			out << YAML::EndSeq;
			
			desiredOutput = "--- [!<!foo> ~, baz]";
		}

		void VerbatimTagInBlockMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << YAML::VerbatimTag("!foo") << "bar";
			out << YAML::Value << YAML::VerbatimTag("!waz") << "baz";
			out << YAML::EndMap;
			
			desiredOutput = "---\n!<!foo> bar: !<!waz> baz";
		}

		void VerbatimTagInFlowMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginMap;
			out << YAML::Key << YAML::VerbatimTag("!foo") << "bar";
			out << YAML::Value << "baz";
			out << YAML::EndMap;
			
			desiredOutput = "--- {!<!foo> bar: baz}";
		}

		void VerbatimTagInFlowMapWithNull(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginMap;
			out << YAML::Key << YAML::VerbatimTag("!foo") << YAML::Null;
			out << YAML::Value << "baz";
			out << YAML::EndMap;
			
			desiredOutput = "--- {!<!foo> ~: baz}";
		}
		
		void VerbatimTagWithEmptySeq(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::VerbatimTag("!foo") << YAML::BeginSeq << YAML::EndSeq;
			
			desiredOutput = "--- !<!foo>\n[]";
		}

		void VerbatimTagWithEmptyMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::VerbatimTag("!bar") << YAML::BeginMap << YAML::EndMap;
			
			desiredOutput = "--- !<!bar>\n{}";
		}

		void VerbatimTagWithEmptySeqAndMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::VerbatimTag("!foo") << YAML::BeginSeq << YAML::EndSeq;
			out << YAML::VerbatimTag("!bar") << YAML::BeginMap << YAML::EndMap;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- !<!foo>\n  []\n- !<!bar>\n  {}";
		}

		void ComplexDoc(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "receipt";
			out << YAML::Value << "Oz-Ware Purchase Invoice";
			out << YAML::Key << "date";
			out << YAML::Value << "2007-08-06";
			out << YAML::Key << "customer";
			out << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "given";
			out << YAML::Value << "Dorothy";
			out << YAML::Key << "family";
			out << YAML::Value << "Gale";
			out << YAML::EndMap;
			out << YAML::Key << "items";
			out << YAML::Value;
			out << YAML::BeginSeq;
			out << YAML::BeginMap;
			out << YAML::Key << "part_no";
			out << YAML::Value << "A4786";
			out << YAML::Key << "descrip";
			out << YAML::Value << "Water Bucket (Filled)";
			out << YAML::Key << "price";
			out << YAML::Value << 1.47;
			out << YAML::Key << "quantity";
			out << YAML::Value << 4;
			out << YAML::EndMap;
			out << YAML::BeginMap;
			out << YAML::Key << "part_no";
			out << YAML::Value << "E1628";
			out << YAML::Key << "descrip";
			out << YAML::Value << "High Heeled \"Ruby\" Slippers";
			out << YAML::Key << "price";
			out << YAML::Value << 100.27;
			out << YAML::Key << "quantity";
			out << YAML::Value << 1;
			out << YAML::EndMap;
			out << YAML::EndSeq;
			out << YAML::Key << "bill-to";
			out << YAML::Value << YAML::Anchor("id001");
			out << YAML::BeginMap;
			out << YAML::Key << "street";
			out << YAML::Value << YAML::Literal << "123 Tornado Alley\nSuite 16";
			out << YAML::Key << "city";
			out << YAML::Value << "East Westville";
			out << YAML::Key << "state";
			out << YAML::Value << "KS";
			out << YAML::EndMap;
			out << YAML::Key << "ship-to";
			out << YAML::Value << YAML::Alias("id001");
			out << YAML::EndMap;
			
			desiredOutput = "---\nreceipt: Oz-Ware Purchase Invoice\ndate: 2007-08-06\ncustomer:\n  given: Dorothy\n  family: Gale\nitems:\n  -\n    part_no: A4786\n    descrip: Water Bucket (Filled)\n    price: 1.47\n    quantity: 4\n  -\n    part_no: E1628\n    descrip: High Heeled \"Ruby\" Slippers\n    price: 100.27\n    quantity: 1\nbill-to: &id001\n  street: |\n    123 Tornado Alley\n    Suite 16\n  city: East Westville\n  state: KS\nship-to: *id001";
		}

		void STLContainers(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			std::vector <int> primes;
			primes.push_back(2);
			primes.push_back(3);
			primes.push_back(5);
			primes.push_back(7);
			primes.push_back(11);
			primes.push_back(13);
			out << YAML::Flow << primes;
			std::map <std::string, int> ages;
			ages["Daniel"] = 26;
			ages["Jesse"] = 24;
			out << ages;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- [2, 3, 5, 7, 11, 13]\n-\n  Daniel: 26\n  Jesse: 24";
		}

		void SimpleComment(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "method";
			out << YAML::Value << "least squares" << YAML::Comment("should we change this method?");
			out << YAML::EndMap;
			
			desiredOutput = "---\nmethod: least squares  # should we change this method?";
		}

		void MultiLineComment(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << "item 1" << YAML::Comment("really really long\ncomment that couldn't possibly\nfit on one line");
			out << "item 2";
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- item 1  # really really long\n          # comment that couldn't possibly\n          # fit on one line\n- item 2";
		}

		void ComplexComments(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::LongKey << YAML::Key << "long key" << YAML::Comment("long key");
			out << YAML::Value << "value";
			out << YAML::EndMap;
			
			desiredOutput = "---\n? long key  # long key\n: value";
		}

		void Indentation(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Indent(4);
			out << YAML::BeginSeq;
			out << YAML::BeginMap;
			out << YAML::Key << "key 1" << YAML::Value << "value 1";
			out << YAML::Key << "key 2" << YAML::Value << YAML::BeginSeq << "a" << "b" << "c" << YAML::EndSeq;
			out << YAML::EndMap;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n-\n    key 1: value 1\n    key 2:\n        - a\n        - b\n        - c";
		}

		void SimpleGlobalSettings(YAML::Emitter& out, std::string& desiredOutput)
		{
			out.SetIndent(4);
			out.SetMapFormat(YAML::LongKey);

			out << YAML::BeginSeq;
			out << YAML::BeginMap;
			out << YAML::Key << "key 1" << YAML::Value << "value 1";
			out << YAML::Key << "key 2" << YAML::Value << YAML::Flow << YAML::BeginSeq << "a" << "b" << "c" << YAML::EndSeq;
			out << YAML::EndMap;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n-\n    ? key 1\n    : value 1\n    ? key 2\n    : [a, b, c]";
		}
		
		void ComplexGlobalSettings(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::Block;
			out << YAML::BeginMap;
			out << YAML::Key << "key 1" << YAML::Value << "value 1";
			out << YAML::Key << "key 2" << YAML::Value;
			out.SetSeqFormat(YAML::Flow);
			out << YAML::BeginSeq << "a" << "b" << "c" << YAML::EndSeq;
			out << YAML::EndMap;
			out << YAML::BeginMap;
			out << YAML::Key << YAML::BeginSeq << 1 << 2 << YAML::EndSeq;
			out << YAML::Value << YAML::BeginMap << YAML::Key << "a" << YAML::Value << "b" << YAML::EndMap;
			out << YAML::EndMap;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n-\n  key 1: value 1\n  key 2: [a, b, c]\n-\n  ? [1, 2]\n  :\n    a: b";
		}

		void Null(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::Null;
			out << YAML::BeginMap;
			out << YAML::Key << "null value" << YAML::Value << YAML::Null;
			out << YAML::Key << YAML::Null << YAML::Value << "null key";
			out << YAML::EndMap;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- ~\n-\n  null value: ~\n  ~: null key";
		}
		
		void EscapedUnicode(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::EscapeNonAscii << "\x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2";
			
			desiredOutput = "--- \"$ \\xa2 \\u20ac \\U00024b62\"";
		}
		
		void Unicode(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << "\x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2";
			desiredOutput = "--- \x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2";
		}
		
		void DoubleQuotedUnicode(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::DoubleQuoted << "\x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2";
			desiredOutput = "--- \"\x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2\"";
		}
		
		struct Foo {
			Foo(): x(0) {}
			Foo(int x_, const std::string& bar_): x(x_), bar(bar_) {}
			
			int x;
			std::string bar;
		};
		
		YAML::Emitter& operator << (YAML::Emitter& out, const Foo& foo) {
			out << YAML::BeginMap;
			out << YAML::Key << "x" << YAML::Value << foo.x;
			out << YAML::Key << "bar" << YAML::Value << foo.bar;
			out << YAML::EndMap;
			return out;
		}
		
		void UserType(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << Foo(5, "hello");
			out << Foo(3, "goodbye");
			out << YAML::EndSeq;
			
			desiredOutput = "---\n-\n  x: 5\n  bar: hello\n-\n  x: 3\n  bar: goodbye";
		}
		
		void UserTypeInContainer(YAML::Emitter& out, std::string& desiredOutput)
		{
			std::vector<Foo> fv;
			fv.push_back(Foo(5, "hello"));
			fv.push_back(Foo(3, "goodbye"));
			out << fv;
			
			desiredOutput = "---\n-\n  x: 5\n  bar: hello\n-\n  x: 3\n  bar: goodbye";
		}
		
		template <typename T>
		YAML::Emitter& operator << (YAML::Emitter& out, const T *v) {
			if(v)
				out << *v;
			else
				out << YAML::Null;
			return out;
		}
		
		void PointerToInt(YAML::Emitter& out, std::string& desiredOutput)
		{
			int foo = 5;
			int *bar = &foo;
			int *baz = 0;
			out << YAML::BeginSeq;
			out << bar << baz;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n- 5\n- ~";
		}

		void PointerToUserType(YAML::Emitter& out, std::string& desiredOutput)
		{
			Foo foo(5, "hello");
			Foo *bar = &foo;
			Foo *baz = 0;
			out << YAML::BeginSeq;
			out << bar << baz;
			out << YAML::EndSeq;
			
			desiredOutput = "---\n-\n  x: 5\n  bar: hello\n- ~";
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// incorrect emitting
		
		void ExtraEndSeq(YAML::Emitter& out, std::string& desiredError)
		{
			desiredError = YAML::ErrorMsg::UNEXPECTED_END_SEQ;

			out << YAML::BeginSeq;
			out << "Hello";
			out << "World";
			out << YAML::EndSeq;
			out << YAML::EndSeq;
		}

		void ExtraEndMap(YAML::Emitter& out, std::string& desiredError)
		{
			desiredError = YAML::ErrorMsg::UNEXPECTED_END_MAP;
			
			out << YAML::BeginMap;
			out << YAML::Key << "Hello" << YAML::Value << "World";
			out << YAML::EndMap;
			out << YAML::EndMap;
		}

		void BadSingleQuoted(YAML::Emitter& out, std::string& desiredError)
		{
			desiredError = YAML::ErrorMsg::SINGLE_QUOTED_CHAR;
			
			out << YAML::SingleQuoted << "Hello\nWorld";
		}

		void InvalidAnchor(YAML::Emitter& out, std::string& desiredError)
		{
			desiredError = YAML::ErrorMsg::INVALID_ANCHOR;
			
			out << YAML::BeginSeq;
			out << YAML::Anchor("new\nline") << "Test";
			out << YAML::EndSeq;
		}
		
		void InvalidAlias(YAML::Emitter& out, std::string& desiredError)
		{
			desiredError = YAML::ErrorMsg::INVALID_ALIAS;
			
			out << YAML::BeginSeq;
			out << YAML::Alias("new\nline");
			out << YAML::EndSeq;
		}

		void MissingKey(YAML::Emitter& out, std::string& desiredError)
		{
			desiredError = YAML::ErrorMsg::EXPECTED_KEY_TOKEN;
			
			out << YAML::BeginMap;
			out << YAML::Key << "key" << YAML::Value << "value";
			out << "missing key" << YAML::Value << "value";
			out << YAML::EndMap;
		}
		
		void MissingValue(YAML::Emitter& out, std::string& desiredError)
		{
			desiredError = YAML::ErrorMsg::EXPECTED_VALUE_TOKEN;
			
			out << YAML::BeginMap;
			out << YAML::Key << "key" << "value";
			out << YAML::EndMap;
		}

		void UnexpectedKey(YAML::Emitter& out, std::string& desiredError)
		{
			desiredError = YAML::ErrorMsg::UNEXPECTED_KEY_TOKEN;
			
			out << YAML::BeginSeq;
			out << YAML::Key << "hi";
			out << YAML::EndSeq;
		}
		
		void UnexpectedValue(YAML::Emitter& out, std::string& desiredError)
		{
			desiredError = YAML::ErrorMsg::UNEXPECTED_VALUE_TOKEN;
			
			out << YAML::BeginSeq;
			out << YAML::Value << "hi";
			out << YAML::EndSeq;
		}
	}
	
	namespace {
		void RunEmitterTest(void (*test)(YAML::Emitter&, std::string&), const std::string& name, int& passed, int& total) {
			YAML::Emitter out;
			std::string desiredOutput;
			test(out, desiredOutput);
			std::string output = out.c_str();

			if(output == desiredOutput) {
				passed++;
			} else {
				std::cout << "Emitter test failed: " << name << "\n";
				std::cout << "Output:\n";
				std::cout << output << "<<<\n";
				std::cout << "Desired output:\n";
				std::cout << desiredOutput << "<<<\n";				
			}
			total++;
		}
		
		void RunEmitterErrorTest(void (*test)(YAML::Emitter&, std::string&), const std::string& name, int& passed, int& total) {
			YAML::Emitter out;
			std::string desiredError;
			test(out, desiredError);
			std::string lastError = out.GetLastError();
			if(!out.good() && lastError == desiredError) {
				passed++;
			} else {
				std::cout << "Emitter test failed: " << name << "\n";
				if(out.good())
					std::cout << "No error detected\n";
				else
					std::cout << "Detected error: " << lastError << "\n";
				std::cout << "Expected error: " << desiredError << "\n";
			}
			total++;
		}
	}
	
	bool RunEmitterTests()
	{
		int passed = 0;
		int total = 0;
		RunEmitterTest(&Emitter::SimpleScalar, "simple scalar", passed, total);
		RunEmitterTest(&Emitter::SimpleSeq, "simple seq", passed, total);
		RunEmitterTest(&Emitter::SimpleFlowSeq, "simple flow seq", passed, total);
		RunEmitterTest(&Emitter::EmptyFlowSeq, "empty flow seq", passed, total);
		RunEmitterTest(&Emitter::NestedBlockSeq, "nested block seq", passed, total);
		RunEmitterTest(&Emitter::NestedFlowSeq, "nested flow seq", passed, total);
		RunEmitterTest(&Emitter::SimpleMap, "simple map", passed, total);
		RunEmitterTest(&Emitter::SimpleFlowMap, "simple flow map", passed, total);
		RunEmitterTest(&Emitter::MapAndList, "map and list", passed, total);
		RunEmitterTest(&Emitter::ListAndMap, "list and map", passed, total);
		RunEmitterTest(&Emitter::NestedBlockMap, "nested block map", passed, total);
		RunEmitterTest(&Emitter::NestedFlowMap, "nested flow map", passed, total);
		RunEmitterTest(&Emitter::MapListMix, "map list mix", passed, total);
		RunEmitterTest(&Emitter::SimpleLongKey, "simple long key", passed, total);
		RunEmitterTest(&Emitter::SingleLongKey, "single long key", passed, total);
		RunEmitterTest(&Emitter::ComplexLongKey, "complex long key", passed, total);
		RunEmitterTest(&Emitter::AutoLongKey, "auto long key", passed, total);
		RunEmitterTest(&Emitter::ScalarFormat, "scalar format", passed, total);
		RunEmitterTest(&Emitter::AutoLongKeyScalar, "auto long key scalar", passed, total);
		RunEmitterTest(&Emitter::LongKeyFlowMap, "long key flow map", passed, total);
		RunEmitterTest(&Emitter::BlockMapAsKey, "block map as key", passed, total);
		RunEmitterTest(&Emitter::AliasAndAnchor, "alias and anchor", passed, total);
		RunEmitterTest(&Emitter::AliasAndAnchorWithNull, "alias and anchor with null", passed, total);
		RunEmitterTest(&Emitter::SimpleVerbatimTag, "simple verbatim tag", passed, total);
		RunEmitterTest(&Emitter::VerbatimTagInBlockSeq, "verbatim tag in block seq", passed, total);
		RunEmitterTest(&Emitter::VerbatimTagInFlowSeq, "verbatim tag in flow seq", passed, total);
		RunEmitterTest(&Emitter::VerbatimTagInFlowSeqWithNull, "verbatim tag in flow seq with null", passed, total);
		RunEmitterTest(&Emitter::VerbatimTagInBlockMap, "verbatim tag in block map", passed, total);
		RunEmitterTest(&Emitter::VerbatimTagInFlowMap, "verbatim tag in flow map", passed, total);
		RunEmitterTest(&Emitter::VerbatimTagInFlowMapWithNull, "verbatim tag in flow map with null", passed, total);
		RunEmitterTest(&Emitter::VerbatimTagWithEmptySeq, "verbatim tag with empty seq", passed, total);
		RunEmitterTest(&Emitter::VerbatimTagWithEmptyMap, "verbatim tag with empty map", passed, total);
		RunEmitterTest(&Emitter::VerbatimTagWithEmptySeqAndMap, "verbatim tag with empty seq and map", passed, total);
		RunEmitterTest(&Emitter::ComplexDoc, "complex doc", passed, total);
		RunEmitterTest(&Emitter::STLContainers, "STL containers", passed, total);
		RunEmitterTest(&Emitter::SimpleComment, "simple comment", passed, total);
		RunEmitterTest(&Emitter::MultiLineComment, "multi-line comment", passed, total);
		RunEmitterTest(&Emitter::ComplexComments, "complex comments", passed, total);
		RunEmitterTest(&Emitter::Indentation, "indentation", passed, total);
		RunEmitterTest(&Emitter::SimpleGlobalSettings, "simple global settings", passed, total);
		RunEmitterTest(&Emitter::ComplexGlobalSettings, "complex global settings", passed, total);
		RunEmitterTest(&Emitter::Null, "null", passed, total);
		RunEmitterTest(&Emitter::EscapedUnicode, "escaped unicode", passed, total);
		RunEmitterTest(&Emitter::Unicode, "unicode", passed, total);
		RunEmitterTest(&Emitter::DoubleQuotedUnicode, "double quoted unicode", passed, total);
		RunEmitterTest(&Emitter::UserType, "user type", passed, total);
		RunEmitterTest(&Emitter::UserTypeInContainer, "user type in container", passed, total);
		RunEmitterTest(&Emitter::PointerToInt, "pointer to int", passed, total);
		RunEmitterTest(&Emitter::PointerToUserType, "pointer to user type", passed, total);
		
		RunEmitterErrorTest(&Emitter::ExtraEndSeq, "extra EndSeq", passed, total);
		RunEmitterErrorTest(&Emitter::ExtraEndMap, "extra EndMap", passed, total);
		RunEmitterErrorTest(&Emitter::BadSingleQuoted, "bad single quoted string", passed, total);
		RunEmitterErrorTest(&Emitter::InvalidAnchor, "invalid anchor", passed, total);
		RunEmitterErrorTest(&Emitter::InvalidAlias, "invalid alias", passed, total);
		RunEmitterErrorTest(&Emitter::MissingKey, "missing key", passed, total);
		RunEmitterErrorTest(&Emitter::MissingValue, "missing value", passed, total);
		RunEmitterErrorTest(&Emitter::UnexpectedKey, "unexpected key", passed, total);
		RunEmitterErrorTest(&Emitter::UnexpectedValue, "unexpected value", passed, total);

		std::cout << "Emitter tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
}

