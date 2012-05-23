#include "tests.h"
#include "handlermacros.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

namespace Test
{
	namespace Emitter {
		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// correct emitting

		void SimpleScalar(YAML::Emitter& out, std::string& desiredOutput) {
			out << "Hello, World!";
			desiredOutput = "Hello, World!";
		}
		
		void SimpleSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginSeq;
			out << "eggs";
			out << "bread";
			out << "milk";
			out << YAML::EndSeq;

			desiredOutput = "- eggs\n- bread\n- milk";
		}
		
		void SimpleFlowSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::Flow;
			out << YAML::BeginSeq;
			out << "Larry";
			out << "Curly";
			out << "Moe";
			out << YAML::EndSeq;
			
			desiredOutput = "[Larry, Curly, Moe]";
		}
		
		void EmptyFlowSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::Flow;
			out << YAML::BeginSeq;
			out << YAML::EndSeq;
			
			desiredOutput = "[]";
		}
		
		void NestedBlockSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginSeq;
			out << "item 1";
			out << YAML::BeginSeq << "subitem 1" << "subitem 2" << YAML::EndSeq;
			out << YAML::EndSeq;
			
			desiredOutput = "- item 1\n-\n  - subitem 1\n  - subitem 2";
		}
		
		void NestedFlowSeq(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginSeq;
			out << "one";
			out << YAML::Flow << YAML::BeginSeq << "two" << "three" << YAML::EndSeq;
			out << YAML::EndSeq;
			
			desiredOutput = "- one\n- [two, three]";
		}

		void SimpleMap(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginMap;
			out << YAML::Key << "name";
			out << YAML::Value << "Ryan Braun";
			out << YAML::Key << "position";
			out << YAML::Value << "3B";
			out << YAML::EndMap;

			desiredOutput = "name: Ryan Braun\nposition: 3B";
		}
		
		void SimpleFlowMap(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::Flow;
			out << YAML::BeginMap;
			out << YAML::Key << "shape";
			out << YAML::Value << "square";
			out << YAML::Key << "color";
			out << YAML::Value << "blue";
			out << YAML::EndMap;
			
			desiredOutput = "{shape: square, color: blue}";
		}
		
		void MapAndList(YAML::Emitter& out, std::string& desiredOutput) {
			out << YAML::BeginMap;
			out << YAML::Key << "name";
			out << YAML::Value << "Barack Obama";
			out << YAML::Key << "children";
			out << YAML::Value << YAML::BeginSeq << "Sasha" << "Malia" << YAML::EndSeq;
			out << YAML::EndMap;

			desiredOutput = "name: Barack Obama\nchildren:\n  - Sasha\n  - Malia";
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
			
			desiredOutput = "- item 1\n- pens: 8\n  pencils: 14\n- item 2";
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
			
			desiredOutput = "name: Fred\ngrades:\n  algebra: A\n  physics: C+\n  literature: B";
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
			
			desiredOutput = "{name: Fred, grades: {algebra: A, physics: C+, literature: B}}";
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
			
			desiredOutput = "name: Bob\nposition: [2, 4]\ninvincible: off";
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
			
			desiredOutput = "? height\n: 5'9\"\n? weight\n: 145";
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
			
			desiredOutput = "age: 24\n? height\n: 5'9\"\nweight: 145";
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
			
			desiredOutput = "? - 1\n  - 3\n: monster\n? [2, 0]\n: demon";
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
			
			desiredOutput = "? - 1\n  - 3\n: monster\n[2, 0]: demon\nthe origin: angel";
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
			
			desiredOutput = "- simple scalar\n- 'explicit single-quoted scalar'\n- \"explicit double-quoted scalar\"\n- \"auto-detected\\ndouble-quoted scalar\"\n- a non-\"auto-detected\" double-quoted scalar\n- |\n  literal scalar\n  that may span\n  many, many\n  lines and have \"whatever\" crazy\tsymbols that we like";
		}

		void AutoLongKeyScalar(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << YAML::Literal << "multi-line\nscalar";
			out << YAML::Value << "and its value";
			out << YAML::EndMap;
			
			desiredOutput = "? |\n  multi-line\n  scalar\n: and its value";
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
			
			desiredOutput = "{simple key: and value, ? long key: and its value}";
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
			
			desiredOutput = "? key: value\n  next key: next value\n: total value";
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
			
			desiredOutput = "- &fred\n  name: Fred\n  age: 42\n- *fred";
		}

		void AliasAndAnchorWithNull(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::Anchor("fred") << YAML::Null;
			out << YAML::Alias("fred");
			out << YAML::EndSeq;
			
			desiredOutput = "- &fred ~\n- *fred";
		}
		
		void AliasAndAnchorInFlow(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginSeq;
			out << YAML::Anchor("fred");
			out << YAML::BeginMap;
			out << YAML::Key << "name" << YAML::Value << "Fred";
			out << YAML::Key << "age" << YAML::Value << 42;
			out << YAML::EndMap;
			out << YAML::Alias("fred");
			out << YAML::EndSeq;
			
			desiredOutput = "[&fred {name: Fred, age: 42}, *fred]";
		}
		
		void SimpleVerbatimTag(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::VerbatimTag("!foo") << "bar";
			
			desiredOutput = "!<!foo> bar";
		}

		void VerbatimTagInBlockSeq(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::VerbatimTag("!foo") << "bar";
			out << "baz";
			out << YAML::EndSeq;
			
			desiredOutput = "- !<!foo> bar\n- baz";
		}

		void VerbatimTagInFlowSeq(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginSeq;
			out << YAML::VerbatimTag("!foo") << "bar";
			out << "baz";
			out << YAML::EndSeq;
			
			desiredOutput = "[!<!foo> bar, baz]";
		}

		void VerbatimTagInFlowSeqWithNull(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginSeq;
			out << YAML::VerbatimTag("!foo") << YAML::Null;
			out << "baz";
			out << YAML::EndSeq;
			
			desiredOutput = "[!<!foo> ~, baz]";
		}

		void VerbatimTagInBlockMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << YAML::VerbatimTag("!foo") << "bar";
			out << YAML::Value << YAML::VerbatimTag("!waz") << "baz";
			out << YAML::EndMap;
			
			desiredOutput = "!<!foo> bar: !<!waz> baz";
		}

		void VerbatimTagInFlowMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginMap;
			out << YAML::Key << YAML::VerbatimTag("!foo") << "bar";
			out << YAML::Value << "baz";
			out << YAML::EndMap;
			
			desiredOutput = "{!<!foo> bar: baz}";
		}

		void VerbatimTagInFlowMapWithNull(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginMap;
			out << YAML::Key << YAML::VerbatimTag("!foo") << YAML::Null;
			out << YAML::Value << "baz";
			out << YAML::EndMap;
			
			desiredOutput = "{!<!foo> ~: baz}";
		}
		
		void VerbatimTagWithEmptySeq(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::VerbatimTag("!foo") << YAML::BeginSeq << YAML::EndSeq;
			
			desiredOutput = "!<!foo>\n[]";
		}

		void VerbatimTagWithEmptyMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::VerbatimTag("!bar") << YAML::BeginMap << YAML::EndMap;
			
			desiredOutput = "!<!bar>\n{}";
		}

		void VerbatimTagWithEmptySeqAndMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::VerbatimTag("!foo") << YAML::BeginSeq << YAML::EndSeq;
			out << YAML::VerbatimTag("!bar") << YAML::BeginMap << YAML::EndMap;
			out << YAML::EndSeq;
			
			desiredOutput = "- !<!foo>\n  []\n- !<!bar>\n  {}";
		}

		void ByKindTagWithScalar(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::DoubleQuoted << "12";
			out << "12";
			out << YAML::TagByKind << "12";
			out << YAML::EndSeq;
			
			desiredOutput = "- \"12\"\n- 12\n- ! 12";
		}

		void LocalTagWithScalar(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::LocalTag("foo") << "bar";
			
			desiredOutput = "!foo bar";
		}

		void BadLocalTag(YAML::Emitter& out, std::string& desiredError)
		{
			out << YAML::LocalTag("e!far") << "bar";
			
			desiredError = "invalid tag";
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
			
			desiredOutput = "receipt: Oz-Ware Purchase Invoice\ndate: 2007-08-06\ncustomer:\n  given: Dorothy\n  family: Gale\nitems:\n  - part_no: A4786\n    descrip: Water Bucket (Filled)\n    price: 1.47\n    quantity: 4\n  - part_no: E1628\n    descrip: High Heeled \"Ruby\" Slippers\n    price: 100.27\n    quantity: 1\nbill-to: &id001\n  street: |\n    123 Tornado Alley\n    Suite 16\n  city: East Westville\n  state: KS\nship-to: *id001";
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
			
			desiredOutput = "- [2, 3, 5, 7, 11, 13]\n- Daniel: 26\n  Jesse: 24";
		}

		void SimpleComment(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "method";
			out << YAML::Value << "least squares" << YAML::Comment("should we change this method?");
			out << YAML::EndMap;
			
			desiredOutput = "method: least squares  # should we change this method?";
		}

		void MultiLineComment(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << "item 1" << YAML::Comment("really really long\ncomment that couldn't possibly\nfit on one line");
			out << "item 2";
			out << YAML::EndSeq;
			
			desiredOutput = "- item 1  # really really long\n          # comment that couldn't possibly\n          # fit on one line\n- item 2";
		}

		void ComplexComments(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::LongKey << YAML::Key << "long key" << YAML::Comment("long key");
			out << YAML::Value << "value";
			out << YAML::EndMap;
			
			desiredOutput = "? long key  # long key\n: value";
		}
		
		void InitialComment(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Comment("A comment describing the purpose of the file.");
			out << YAML::BeginMap << YAML::Key << "key" << YAML::Value << "value" << YAML::EndMap;
			
			desiredOutput = "# A comment describing the purpose of the file.\nkey: value";
		}

		void InitialCommentWithDocIndicator(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginDoc << YAML::Comment("A comment describing the purpose of the file.");
			out << YAML::BeginMap << YAML::Key << "key" << YAML::Value << "value" << YAML::EndMap;
			
			desiredOutput = "---\n# A comment describing the purpose of the file.\nkey: value";
		}

		void CommentInFlowSeq(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginSeq << "foo" << YAML::Comment("foo!") << "bar" << YAML::EndSeq;
			
			desiredOutput = "[foo,  # foo!\nbar]";
		}

		void CommentInFlowMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginMap;
			out << YAML::Key << "foo" << YAML::Value << "foo value";
			out << YAML::Key << "bar" << YAML::Value << "bar value" << YAML::Comment("bar!");
			out << YAML::Key << "baz" << YAML::Value << "baz value" << YAML::Comment("baz!");
			out << YAML::EndMap;
			
			desiredOutput = "{foo: foo value, bar: bar value,  # bar!\nbaz: baz value,  # baz!\n}";
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
			
			desiredOutput = "-   key 1: value 1\n    key 2:\n        -   a\n        -   b\n        -   c";
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
			
			desiredOutput = "-   ? key 1\n    : value 1\n    ? key 2\n    : [a, b, c]";
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
			
			desiredOutput = "- key 1: value 1\n  key 2: [a, b, c]\n- [1, 2]:\n    a: b";
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
			
			desiredOutput = "- ~\n- null value: ~\n  ~: null key";
		}
		
		void EscapedUnicode(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::EscapeNonAscii << "\x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2";
			
			desiredOutput = "\"$ \\xa2 \\u20ac \\U00024b62\"";
		}
		
		void Unicode(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << "\x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2";
			desiredOutput = "\x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2";
		}
		
		void DoubleQuotedUnicode(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::DoubleQuoted << "\x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2";
			desiredOutput = "\"\x24 \xC2\xA2 \xE2\x82\xAC \xF0\xA4\xAD\xA2\"";
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
			
			desiredOutput = "- x: 5\n  bar: hello\n- x: 3\n  bar: goodbye";
		}
		
		void UserTypeInContainer(YAML::Emitter& out, std::string& desiredOutput)
		{
			std::vector<Foo> fv;
			fv.push_back(Foo(5, "hello"));
			fv.push_back(Foo(3, "goodbye"));
			out << fv;
			
			desiredOutput = "- x: 5\n  bar: hello\n- x: 3\n  bar: goodbye";
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
			
			desiredOutput = "- 5\n- ~";
		}

		void PointerToUserType(YAML::Emitter& out, std::string& desiredOutput)
		{
			Foo foo(5, "hello");
			Foo *bar = &foo;
			Foo *baz = 0;
			out << YAML::BeginSeq;
			out << bar << baz;
			out << YAML::EndSeq;
			
			desiredOutput = "- x: 5\n  bar: hello\n- ~";
		}
		
		void NewlineAtEnd(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << "Hello" << YAML::Newline << YAML::Newline;
			desiredOutput = "Hello\n\n";
		}
		
		void NewlineInBlockSequence(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << "a" << YAML::Newline << "b" << "c" << YAML::Newline << "d";
			out << YAML::EndSeq;
			desiredOutput = "- a\n\n- b\n- c\n\n- d";
		}
		
		void NewlineInFlowSequence(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginSeq;
			out << "a" << YAML::Newline << "b" << "c" << YAML::Newline << "d";
			out << YAML::EndSeq;
			desiredOutput = "[a,\nb, c,\nd]";
		}

		void NewlineInBlockMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "a" << YAML::Value << "foo" << YAML::Newline;
			out << YAML::Key << "b" << YAML::Newline << YAML::Value << "bar";
			out << YAML::LongKey << YAML::Key << "c" << YAML::Newline << YAML::Value << "car";
			out << YAML::EndMap;
			desiredOutput = "a: foo\nb:\n  bar\n? c\n\n: car";
		}
		
		void NewlineInFlowMap(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginMap;
			out << YAML::Key << "a" << YAML::Value << "foo" << YAML::Newline;
			out << YAML::Key << "b" << YAML::Value << "bar";
			out << YAML::EndMap;
			desiredOutput = "{a: foo,\nb: bar}";
		}
		
		void LotsOfNewlines(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << "a" << YAML::Newline;
			out << YAML::BeginSeq;
			out << "b" << "c" << YAML::Newline;
			out << YAML::EndSeq;
			out << YAML::Newline;
			out << YAML::BeginMap;
			out << YAML::Newline << YAML::Key << "d" << YAML::Value << YAML::Newline << "e";
			out << YAML::LongKey << YAML::Key << "f" << YAML::Newline << YAML::Value << "foo";
			out << YAML::EndMap;
			out << YAML::EndSeq;
			desiredOutput = "- a\n\n-\n  - b\n  - c\n\n\n-\n  d:\n    e\n  ? f\n\n  : foo";
		}
		
		void Binary(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Binary(reinterpret_cast<const unsigned char*>("Hello, World!"), 13);
			desiredOutput = "!!binary \"SGVsbG8sIFdvcmxkIQ==\"";
		}

		void LongBinary(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Binary(reinterpret_cast<const unsigned char*>("Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.\n"), 270);
			desiredOutput = "!!binary \"TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4K\"";
		}

		void EmptyBinary(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Binary(reinterpret_cast<const unsigned char *>(""), 0);
			desiredOutput = "!!binary \"\"";
		}
		
		void ColonAtEndOfScalar(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << "a:";
			desiredOutput = "\"a:\"";
		}

		void ColonAsScalar(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "apple" << YAML::Value << ":";
			out << YAML::Key << "banana" << YAML::Value << ":";
			out << YAML::EndMap;
			desiredOutput = "apple: \":\"\nbanana: \":\"";
		}
		
		void ColonAtEndOfScalarInFlow(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::Flow << YAML::BeginMap << YAML::Key << "C:" << YAML::Value << "C:" << YAML::EndMap;
			desiredOutput = "{\"C:\": \"C:\"}";
		}
		
		void BoolFormatting(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << YAML::TrueFalseBool << YAML::UpperCase << true;
			out << YAML::TrueFalseBool << YAML::CamelCase << true;
			out << YAML::TrueFalseBool << YAML::LowerCase << true;
			out << YAML::TrueFalseBool << YAML::UpperCase << false;
			out << YAML::TrueFalseBool << YAML::CamelCase << false;
			out << YAML::TrueFalseBool << YAML::LowerCase << false;
			out << YAML::YesNoBool << YAML::UpperCase << true;
			out << YAML::YesNoBool << YAML::CamelCase << true;
			out << YAML::YesNoBool << YAML::LowerCase << true;
			out << YAML::YesNoBool << YAML::UpperCase << false;
			out << YAML::YesNoBool << YAML::CamelCase << false;
			out << YAML::YesNoBool << YAML::LowerCase << false;
			out << YAML::OnOffBool << YAML::UpperCase << true;
			out << YAML::OnOffBool << YAML::CamelCase << true;
			out << YAML::OnOffBool << YAML::LowerCase << true;
			out << YAML::OnOffBool << YAML::UpperCase << false;
			out << YAML::OnOffBool << YAML::CamelCase << false;
			out << YAML::OnOffBool << YAML::LowerCase << false;
			out << YAML::ShortBool << YAML::UpperCase << true;
			out << YAML::ShortBool << YAML::CamelCase << true;
			out << YAML::ShortBool << YAML::LowerCase << true;
			out << YAML::ShortBool << YAML::UpperCase << false;
			out << YAML::ShortBool << YAML::CamelCase << false;
			out << YAML::ShortBool << YAML::LowerCase << false;
			out << YAML::EndSeq;
			desiredOutput =
			"- TRUE\n- True\n- true\n- FALSE\n- False\n- false\n"
			"- YES\n- Yes\n- yes\n- NO\n- No\n- no\n"
			"- ON\n- On\n- on\n- OFF\n- Off\n- off\n"
			"- Y\n- Y\n- y\n- N\n- N\n- n";
		}
		
		void DocStartAndEnd(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginDoc;
			out << YAML::BeginSeq << 1 << 2 << 3 << YAML::EndSeq;
			out << YAML::BeginDoc;
			out << "Hi there!";
			out << YAML::EndDoc;
			out << YAML::EndDoc;
			out << YAML::EndDoc;
			out << YAML::BeginDoc;
			out << YAML::VerbatimTag("foo") << "bar";
			desiredOutput = "---\n- 1\n- 2\n- 3\n---\nHi there!\n...\n...\n...\n---\n!<foo> bar";
		}
		
		void ImplicitDocStart(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << "Hi";
			out << "Bye";
			out << "Oops";
			desiredOutput = "Hi\n---\nBye\n---\nOops";
		}
		
		void EmptyString(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "key" << YAML::Value << "";
			out << YAML::EndMap;
			desiredOutput = "key: \"\"";
		}
		
		void SingleChar(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::BeginSeq;
			out << 'a';
			out << ':';
			out << (char)0x10;
			out << '\n';
			out << ' ';
			out << '\t';
			out << YAML::EndSeq;
			desiredOutput = "- a\n- \":\"\n- \"\\x10\"\n- \"\\n\"\n- \" \"\n- \"\\t\"";
		}
        
        void DefaultPrecision(YAML::Emitter& out, std::string& desiredOutput)
        {
            out << YAML::BeginSeq;
            out << 1.234f;
            out << 3.14159265358979;
            out << YAML::EndSeq;
            desiredOutput = "- 1.234\n- 3.14159265358979";
        }

        void SetPrecision(YAML::Emitter& out, std::string& desiredOutput)
        {
            out << YAML::BeginSeq;
            out << YAML::FloatPrecision(3) << 1.234f;
            out << YAML::DoublePrecision(6) << 3.14159265358979;
            out << YAML::EndSeq;
            desiredOutput = "- 1.23\n- 3.14159";
        }
        
        void DashInBlockContext(YAML::Emitter& out, std::string& desiredOutput)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "key" << YAML::Value << "-";
            out << YAML::EndMap;
            desiredOutput = "key: \"-\"";
        }
        
        void HexAndOct(YAML::Emitter& out, std::string& desiredOutput)
        {
            out << YAML::Flow << YAML::BeginSeq;
            out << 31;
            out << YAML::Hex << 31;
            out << YAML::Oct << 31;
            out << YAML::EndSeq;
            desiredOutput = "[31, 0x1f, 037]";
        }
        
        void CompactMapWithNewline(YAML::Emitter& out, std::string& desiredOutput)
        {
            out << YAML::Comment("Characteristics");
            out << YAML::BeginSeq;
            out << YAML::BeginMap;
            out << YAML::Key << "color" << YAML::Value << "blue";
            out << YAML::Key << "height" << YAML::Value << 120;
            out << YAML::EndMap;
            out << YAML::Newline << YAML::Newline;
            out << YAML::Comment("Skills");
            out << YAML::BeginMap;
            out << YAML::Key << "attack" << YAML::Value << 23;
            out << YAML::Key << "intelligence" << YAML::Value << 56;
            out << YAML::EndMap;
            out << YAML::EndSeq;
            
            desiredOutput =
            "# Characteristics\n"
            "- color: blue\n"
            "  height: 120\n"
            "\n"
            "# Skills\n"
            "- attack: 23\n"
            "  intelligence: 56";
        }
        
		void ForceSingleQuotedToDouble(YAML::Emitter& out, std::string& desiredOutput)
		{
			out << YAML::SingleQuoted << "Hello\nWorld";

            desiredOutput = "\"Hello\\nWorld\"";
		}
        
        ////////////////////////////////////////////////////////////////////////////////
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
	}
	
	namespace {
		void RunEmitterTest(void (*test)(YAML::Emitter&, std::string&), const std::string& name, int& passed, int& total) {
			YAML::Emitter out;
			std::string desiredOutput;
			test(out, desiredOutput);
			std::string output = out.c_str();
			std::string lastError = out.GetLastError();

			if(output == desiredOutput) {
				try {
                    YAML::Node node = YAML::Load(output);
					passed++;
				} catch(const YAML::Exception& e) {
					std::cout << "Emitter test failed: " << name << "\n";
					std::cout << "Parsing output error: " << e.what() << "\n";
				}
			} else {
				std::cout << "Emitter test failed: " << name << "\n";
				std::cout << "Output:\n";
				std::cout << output << "<<<\n";
				std::cout << "Desired output:\n";
				std::cout << desiredOutput << "<<<\n";				
				if(!out.good())
					std::cout << "Emitter error: " << lastError << "\n";
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
        
        void RunGenEmitterTest(TEST (*test)(YAML::Emitter&), const std::string& name, int& passed, int& total) {
            YAML::Emitter out;
            TEST ret;

			try {
				ret = test(out);
			} catch(const YAML::Exception& e) {
				ret.ok = false;
				ret.error = std::string("  Exception caught: ") + e.what();
			}
            
            if(!out.good()) {
                ret.ok = false;
                ret.error = out.GetLastError();
            }
			
			if(!ret.ok) {
				std::cout << "Generated emitter test failed: " << name << "\n";
                std::cout << "Output:\n";
                std::cout << out.c_str() << "<<<\n";
				std::cout << ret.error << "\n";
			}
			
			if(ret.ok)
				passed++;
			total++;
        }
	}
    
#include "genemittertests.h"
	
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
		RunEmitterTest(&Emitter::AliasAndAnchorInFlow, "alias and anchor in flow", passed, total);
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
		RunEmitterTest(&Emitter::ByKindTagWithScalar, "by-kind tag with scalar", passed, total);
		RunEmitterTest(&Emitter::LocalTagWithScalar, "local tag with scalar", passed, total);
		RunEmitterTest(&Emitter::ComplexDoc, "complex doc", passed, total);
		RunEmitterTest(&Emitter::STLContainers, "STL containers", passed, total);
		RunEmitterTest(&Emitter::SimpleComment, "simple comment", passed, total);
		RunEmitterTest(&Emitter::MultiLineComment, "multi-line comment", passed, total);
		RunEmitterTest(&Emitter::ComplexComments, "complex comments", passed, total);
		RunEmitterTest(&Emitter::InitialComment, "initial comment", passed, total);
		RunEmitterTest(&Emitter::InitialCommentWithDocIndicator, "initial comment with doc indicator", passed, total);
		RunEmitterTest(&Emitter::CommentInFlowSeq, "comment in flow seq", passed, total);
		RunEmitterTest(&Emitter::CommentInFlowMap, "comment in flow map", passed, total);
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
		RunEmitterTest(&Emitter::NewlineAtEnd, "newline at end", passed, total);
		RunEmitterTest(&Emitter::NewlineInBlockSequence, "newline in block sequence", passed, total);
		RunEmitterTest(&Emitter::NewlineInFlowSequence, "newline in flow sequence", passed, total);
		RunEmitterTest(&Emitter::NewlineInBlockMap, "newline in block map", passed, total);
		RunEmitterTest(&Emitter::NewlineInFlowMap, "newline in flow map", passed, total);
		RunEmitterTest(&Emitter::LotsOfNewlines, "lots of newlines", passed, total);
		RunEmitterTest(&Emitter::Binary, "binary", passed, total);
		RunEmitterTest(&Emitter::LongBinary, "long binary", passed, total);
		RunEmitterTest(&Emitter::EmptyBinary, "empty binary", passed, total);
		RunEmitterTest(&Emitter::ColonAtEndOfScalar, "colon at end of scalar", passed, total);
		RunEmitterTest(&Emitter::ColonAsScalar, "colon as scalar", passed, total);
		RunEmitterTest(&Emitter::ColonAtEndOfScalarInFlow, "colon at end of scalar in flow", passed, total);
		RunEmitterTest(&Emitter::BoolFormatting, "bool formatting", passed, total);
		RunEmitterTest(&Emitter::DocStartAndEnd, "doc start and end", passed, total);
		RunEmitterTest(&Emitter::ImplicitDocStart, "implicit doc start", passed, total);
		RunEmitterTest(&Emitter::EmptyString, "empty string", passed, total);
		RunEmitterTest(&Emitter::SingleChar, "single char", passed, total);
		RunEmitterTest(&Emitter::DefaultPrecision, "default precision", passed, total);
		RunEmitterTest(&Emitter::SetPrecision, "set precision", passed, total);
		RunEmitterTest(&Emitter::DashInBlockContext, "dash in block context", passed, total);
		RunEmitterTest(&Emitter::HexAndOct, "hex and oct", passed, total);
		RunEmitterTest(&Emitter::CompactMapWithNewline, "compact map with newline", passed, total);
        RunEmitterTest(&Emitter::ForceSingleQuotedToDouble, "force single quoted to double", passed, total);
		
		RunEmitterErrorTest(&Emitter::ExtraEndSeq, "extra EndSeq", passed, total);
		RunEmitterErrorTest(&Emitter::ExtraEndMap, "extra EndMap", passed, total);
		RunEmitterErrorTest(&Emitter::InvalidAnchor, "invalid anchor", passed, total);
		RunEmitterErrorTest(&Emitter::InvalidAlias, "invalid alias", passed, total);
		RunEmitterErrorTest(&Emitter::BadLocalTag, "bad local tag", passed, total);
        
        RunGenEmitterTests(passed, total);

		std::cout << "Emitter tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
}

