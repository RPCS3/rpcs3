#include "spectests.h"
#include "specexamples.h"
#include "yaml-cpp/yaml.h"

#define YAML_ASSERT(cond) do { if(!(cond)) return "  Assert failed: " #cond; } while(false)

namespace Test
{
	namespace Spec
	{
		// 2.1
		TEST SeqScalars() {
			YAML::Node doc = YAML::Parse(ex2_1);
			YAML_ASSERT(doc.Type() == YAML::NodeType::Sequence);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].as<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc[1].as<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc[2].as<std::string>() == "Ken Griffey");
			return true;
		}
		
		// 2.2
		TEST MappingScalarsToScalars() {
			YAML::Node doc = YAML::Parse(ex2_2);
			YAML_ASSERT(doc.Type() == YAML::NodeType::Map);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["hr"].as<std::string>() == "65");
			YAML_ASSERT(doc["avg"].as<std::string>() == "0.278");
			YAML_ASSERT(doc["rbi"].as<std::string>() == "147");
			return true;
		}
		
		// 2.3
		TEST MappingScalarsToSequences() {
			YAML::Node doc = YAML::Parse(ex2_3);
			YAML_ASSERT(doc.Type() == YAML::NodeType::Map);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["american"].size() == 3);
			YAML_ASSERT(doc["american"][0].as<std::string>() == "Boston Red Sox");
			YAML_ASSERT(doc["american"][1].as<std::string>() == "Detroit Tigers");
			YAML_ASSERT(doc["american"][2].as<std::string>() == "New York Yankees");
			YAML_ASSERT(doc["national"].size() == 3);
			YAML_ASSERT(doc["national"][0].as<std::string>() == "New York Mets");
			YAML_ASSERT(doc["national"][1].as<std::string>() == "Chicago Cubs");
			YAML_ASSERT(doc["national"][2].as<std::string>() == "Atlanta Braves");
			return true;
		}
		
		// 2.4
		TEST SequenceOfMappings() {
			YAML::Node doc = YAML::Parse(ex2_4);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 3);
			YAML_ASSERT(doc[0]["name"].as<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc[0]["hr"].as<std::string>() == "65");
			YAML_ASSERT(doc[0]["avg"].as<std::string>() == "0.278");
			YAML_ASSERT(doc[1].size() == 3);
			YAML_ASSERT(doc[1]["name"].as<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc[1]["hr"].as<std::string>() == "63");
			YAML_ASSERT(doc[1]["avg"].as<std::string>() == "0.288");
			return true;
		}
		
		// 2.5
		TEST SequenceOfSequences() {
			YAML::Node doc = YAML::Parse(ex2_5);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 3);
			YAML_ASSERT(doc[0][0].as<std::string>() == "name");
			YAML_ASSERT(doc[0][1].as<std::string>() == "hr");
			YAML_ASSERT(doc[0][2].as<std::string>() == "avg");
			YAML_ASSERT(doc[1].size() == 3);
			YAML_ASSERT(doc[1][0].as<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc[1][1].as<std::string>() == "65");
			YAML_ASSERT(doc[1][2].as<std::string>() == "0.278");
			YAML_ASSERT(doc[2].size() == 3);
			YAML_ASSERT(doc[2][0].as<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc[2][1].as<std::string>() == "63");
			YAML_ASSERT(doc[2][2].as<std::string>() == "0.288");
			return true;
		}
		
		// 2.6
		TEST MappingOfMappings() {
			YAML::Node doc = YAML::Parse(ex2_6);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["Mark McGwire"].size() == 2);
			YAML_ASSERT(doc["Mark McGwire"]["hr"].as<std::string>() == "65");
			YAML_ASSERT(doc["Mark McGwire"]["avg"].as<std::string>() == "0.278");
			YAML_ASSERT(doc["Sammy Sosa"].size() == 2);
			YAML_ASSERT(doc["Sammy Sosa"]["hr"].as<std::string>() == "63");
			YAML_ASSERT(doc["Sammy Sosa"]["avg"].as<std::string>() == "0.288");
			return true;
		}
		
		// 2.7
		TEST TwoDocumentsInAStream() {
			return "  not written yet"; 
		}
		
		// 2.8
		TEST PlayByPlayFeed() {
			return "  not written yet";
		}
		
		// 2.9
		TEST SingleDocumentWithTwoComments() {
			YAML::Node doc = YAML::Parse(ex2_9);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["hr"].size() == 2);
			YAML_ASSERT(doc["hr"][0].as<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc["hr"][1].as<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"].size() == 2);
			YAML_ASSERT(doc["rbi"][0].as<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"][1].as<std::string>() == "Ken Griffey");
			return true;
		}
		
		// 2.10
		TEST SimpleAnchor() {
			YAML::Node doc = YAML::Parse(ex2_10);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["hr"].size() == 2);
			YAML_ASSERT(doc["hr"][0].as<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc["hr"][1].as<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"].size() == 2);
			YAML_ASSERT(doc["rbi"][0].as<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"][1].as<std::string>() == "Ken Griffey");
			return true;
		}
		
		// 2.11
		TEST MappingBetweenSequences() {
			YAML::Node doc = YAML::Parse(ex2_11);

			std::vector<std::string> tigers_cubs;
			tigers_cubs.push_back("Detroit Tigers");
			tigers_cubs.push_back("Chicago cubs");
			
			std::vector<std::string> yankees_braves;
			yankees_braves.push_back("New York Yankees");
			yankees_braves.push_back("Atlanta Braves");
			
			
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[tigers_cubs].size() == 1);
			YAML_ASSERT(doc[tigers_cubs][0].as<std::string>() == "2001-07-23");
			YAML_ASSERT(doc[yankees_braves].size() == 3);
			YAML_ASSERT(doc[yankees_braves][0].as<std::string>() == "2001-07-02");
			YAML_ASSERT(doc[yankees_braves][1].as<std::string>() == "2001-08-12");
			YAML_ASSERT(doc[yankees_braves][2].as<std::string>() == "2001-08-14");
			return true;
		}
		
		// 2.12
		TEST CompactNestedMapping() {
			YAML::Node doc = YAML::Parse(ex2_12);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0]["item"].as<std::string>() == "Super Hoop");
			YAML_ASSERT(doc[0]["quantity"].as<int>() == 1);
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1]["item"].as<std::string>() == "Basketball");
			YAML_ASSERT(doc[1]["quantity"].as<int>() == 4);
			YAML_ASSERT(doc[2].size() == 2);
			YAML_ASSERT(doc[2]["item"].as<std::string>() == "Big Shoes");
			YAML_ASSERT(doc[2]["quantity"].as<int>() == 1);
			return true;
		}
		
		// 2.13
		TEST InLiteralsNewlinesArePreserved() {
			YAML::Node doc = YAML::Parse(ex2_13);
			YAML_ASSERT(doc.as<std::string>() ==
						"\\//||\\/||\n"
						"// ||  ||__");
			return true;
		}
		
		// 2.14
		TEST InFoldedScalarsNewlinesBecomeSpaces() {
			YAML::Node doc = YAML::Parse(ex2_14);
			YAML_ASSERT(doc.as<std::string>() == "Mark McGwire's year was crippled by a knee injury.");
			return true;
		}
		
		// 2.15
		TEST FoldedNewlinesArePreservedForMoreIndentedAndBlankLines() {
			YAML::Node doc = YAML::Parse(ex2_15);
			YAML_ASSERT(doc.as<std::string>() ==
						"Sammy Sosa completed another fine season with great stats.\n\n"
						"  63 Home Runs\n"
						"  0.288 Batting Average\n\n"
						"What a year!");
			return true;
		}
		
		// 2.16
		TEST IndentationDeterminesScope() {
			YAML::Node doc = YAML::Parse(ex2_16);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["name"].as<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc["accomplishment"].as<std::string>() == "Mark set a major league home run record in 1998.\n");
			YAML_ASSERT(doc["stats"].as<std::string>() == "65 Home Runs\n0.278 Batting Average\n");
			return true;
		}
		
		// 2.17
		TEST QuotedScalars() {
			YAML::Node doc = YAML::Parse(ex2_17);
			YAML_ASSERT(doc.size() == 6);
			YAML_ASSERT(doc["unicode"].as<std::string>() == "Sosa did fine.\xe2\x98\xba");
			YAML_ASSERT(doc["control"].as<std::string>() == "\b1998\t1999\t2000\n");
			YAML_ASSERT(doc["hex esc"].as<std::string>() == "\x0d\x0a is \r\n");
			YAML_ASSERT(doc["single"].as<std::string>() == "\"Howdy!\" he cried.");
			YAML_ASSERT(doc["quoted"].as<std::string>() == " # Not a 'comment'.");
			YAML_ASSERT(doc["tie-fighter"].as<std::string>() == "|\\-*-/|");
			return true;
		}
		
		// 2.18
		TEST MultiLineFlowScalars() {
			YAML::Node doc = YAML::Parse(ex2_18);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["plain"].as<std::string>() == "This unquoted scalar spans many lines.");
			YAML_ASSERT(doc["quoted"].as<std::string>() == "So does this quoted scalar.\n");
			return true;
		}
		
		// TODO: 2.19 - 2.22 schema tags
		
		// 2.23
		TEST VariousExplicitTags() { return "  not written yet"; }
		
		// 2.24
		TEST GlobalTags() { return "  not written yet"; }
		
		// 2.25
		TEST UnorderedSets() { return "  not written yet"; }
		
		// 2.26
		TEST OrderedMappings() { return "  not written yet"; }
		
		// 2.27
		TEST Invoice() { return "  not written yet"; }
		
		// 2.28
		TEST LogFile() { return "  not written yet"; }
		
		// TODO: 5.1 - 5.2 BOM
		
		// 5.3
		TEST BlockStructureIndicators() {
			YAML::Node doc = YAML::Parse(ex5_3);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["sequence"].size() == 2);
			YAML_ASSERT(doc["sequence"][0].as<std::string>() == "one");
			YAML_ASSERT(doc["sequence"][1].as<std::string>() == "two");
			YAML_ASSERT(doc["mapping"].size() == 2);
			YAML_ASSERT(doc["mapping"]["sky"].as<std::string>() == "blue");
			YAML_ASSERT(doc["mapping"]["sea"].as<std::string>() == "green");
			return true;
		}
		
		// 5.4
		TEST FlowStructureIndicators() {
			YAML::Node doc = YAML::Parse(ex5_4);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["sequence"].size() == 2);
			YAML_ASSERT(doc["sequence"][0].as<std::string>() == "one");
			YAML_ASSERT(doc["sequence"][1].as<std::string>() == "two");
			YAML_ASSERT(doc["mapping"].size() == 2);
			YAML_ASSERT(doc["mapping"]["sky"].as<std::string>() == "blue");
			YAML_ASSERT(doc["mapping"]["sea"].as<std::string>() == "green");
			return true;
		}
		
		// 5.5
		TEST CommentIndicator() {
			YAML::Node doc = YAML::Parse(ex5_5);
			YAML_ASSERT(doc.Type() == YAML::NodeType::Null);
			return true;
		}
		
		// 5.6
		TEST NodePropertyIndicators() {
			YAML::Node doc = YAML::Parse(ex5_6);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["anchored"].as<std::string>() == "value"); // TODO: assert tag
			YAML_ASSERT(doc["alias"].as<std::string>() == "value");
			return true;
		}
		
		// 5.7
		TEST BlockScalarIndicators() {
			YAML::Node doc = YAML::Parse(ex5_7);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["literal"].as<std::string>() == "some\ntext\n");
			YAML_ASSERT(doc["folded"].as<std::string>() == "some text\n");
			return true;
		}
		
		// 5.8
		TEST QuotedScalarIndicators() {
			YAML::Node doc = YAML::Parse(ex5_8);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["single"].as<std::string>() == "text");
			YAML_ASSERT(doc["double"].as<std::string>() == "text");
			return true;
		}
		
		// TODO: 5.9 directive
		// TODO: 5.10 reserved indicator
		
		// 5.11
		TEST LineBreakCharacters() {
			YAML::Node doc = YAML::Parse(ex5_11);
			YAML_ASSERT(doc.as<std::string>() == "Line break (no glyph)\nLine break (glyphed)\n");
			return true;
		}
		
		// 5.12
		TEST TabsAndSpaces() {
			YAML::Node doc = YAML::Parse(ex5_12);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["quoted"].as<std::string>() == "Quoted\t");
			YAML_ASSERT(doc["block"].as<std::string>() ==
						"void main() {\n"
						"\tprintf(\"Hello, world!\\n\");\n"
						"}");
			return true;
		}
		
		// 5.13
		TEST EscapedCharacters() {
			YAML::Node doc = YAML::Parse(ex5_13);
			YAML_ASSERT(doc.as<std::string>() == "Fun with \x5C \x22 \x07 \x08 \x1B \x0C \x0A \x0D \x09 \x0B " + std::string("\x00", 1) + " \x20 \xA0 \x85 \xe2\x80\xa8 \xe2\x80\xa9 A A A");
			return true;
		}
		
		// 5.14
		TEST InvalidEscapedCharacters() {
			try {
				YAML::Parse(ex5_14);
			} catch(const YAML::ParserException& e) {
				YAML_ASSERT(e.msg == std::string(YAML::ErrorMsg::INVALID_ESCAPE) + "c");
				return true;
			}
			
			return false;
		}
		
		// 6.1
		TEST IndentationSpaces() {
			YAML::Node doc = YAML::Parse(ex6_1);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["Not indented"].size() == 2);
			YAML_ASSERT(doc["Not indented"]["By one space"].as<std::string>() == "By four\n  spaces\n");
			YAML_ASSERT(doc["Not indented"]["Flow style"].size() == 3);
			YAML_ASSERT(doc["Not indented"]["Flow style"][0].as<std::string>() == "By two");
			YAML_ASSERT(doc["Not indented"]["Flow style"][1].as<std::string>() == "Also by two");
			YAML_ASSERT(doc["Not indented"]["Flow style"][2].as<std::string>() == "Still by two");
			return true;
		}
		
		// 6.2
		TEST IndentationIndicators() {
			YAML::Node doc = YAML::Parse(ex6_2);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["a"].size() == 2);
			YAML_ASSERT(doc["a"][0].as<std::string>() == "b");
			YAML_ASSERT(doc["a"][1].size() == 2);
			YAML_ASSERT(doc["a"][1][0].as<std::string>() == "c");
			YAML_ASSERT(doc["a"][1][1].as<std::string>() == "d");
			return true;
		}
		
		// 6.3
		TEST SeparationSpaces() {
			YAML::Node doc = YAML::Parse(ex6_3);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo"].as<std::string>() == "bar");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1][0].as<std::string>() == "baz");
			YAML_ASSERT(doc[1][1].as<std::string>() == "baz");
			return true;
		}
		
		// 6.4
		TEST LinePrefixes() {
			YAML::Node doc = YAML::Parse(ex6_4);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["plain"].as<std::string>() == "text lines");
			YAML_ASSERT(doc["quoted"].as<std::string>() == "text lines");
			YAML_ASSERT(doc["block"].as<std::string>() == "text\n \tlines\n");
			return true;
		}
		
		// 6.5
		TEST EmptyLines() {
			YAML::Node doc = YAML::Parse(ex6_5);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["Folding"].as<std::string>() == "Empty line\nas a line feed");
			YAML_ASSERT(doc["Chomping"].as<std::string>() == "Clipped empty lines\n");
			return true;
		}
		
		// 6.6
		TEST LineFolding() {
			YAML::Node doc = YAML::Parse(ex6_6);
			YAML_ASSERT(doc.as<std::string>() == "trimmed\n\n\nas space");
			return true;
		}
		
		// 6.7
		TEST BlockFolding() {
			YAML::Node doc = YAML::Parse(ex6_7);
			YAML_ASSERT(doc.as<std::string>() == "foo \n\n\t bar\n\nbaz\n");
			return true;
		}
		
		// 6.8
		TEST FlowFolding() {
			YAML::Node doc = YAML::Parse(ex6_8);
			YAML_ASSERT(doc.as<std::string>() == " foo\nbar\nbaz ");
			return true;
		}
		
		// 6.9
		TEST SeparatedComment() {
			YAML::Node doc = YAML::Parse(ex6_9);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["key"].as<std::string>() == "value");
			return true;
		}
		
		// 6.10
		TEST CommentLines() {
			YAML::Node doc = YAML::Parse(ex6_10);
			YAML_ASSERT(doc.Type() == YAML::NodeType::Null);
			return true;
		}
		
		// 6.11
		TEST MultiLineComments() {
			YAML::Node doc = YAML::Parse(ex6_11);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["key"].as<std::string>() == "value");
			return true;
		}
		
		// 6.12
		TEST SeparationSpacesII() {
			YAML::Node doc = YAML::Parse(ex6_12);
			
			std::map<std::string, std::string> sammy;
			sammy["first"] = "Sammy";
			sammy["last"] = "Sosa";
			
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[sammy].size() == 2);
			YAML_ASSERT(doc[sammy]["hr"].as<int>() == 65);
			YAML_ASSERT(doc[sammy]["avg"].as<std::string>() == "0.278");
			return true;
		}
		
		// 6.13
		TEST ReservedDirectives() {
			YAML::Node doc = YAML::Parse(ex6_13);
			YAML_ASSERT(doc.as<std::string>() == "foo");
			return true;
		}
		
		// 6.14
		TEST YAMLDirective() {
			YAML::Node doc = YAML::Parse(ex6_14);
			YAML_ASSERT(doc.as<std::string>() == "foo");
			return true;
		}
		
		// 6.15
		TEST InvalidRepeatedYAMLDirective() {
			try {
				YAML::Parse(ex6_15);
			} catch(const YAML::ParserException& e) {
				YAML_ASSERT(e.msg == YAML::ErrorMsg::REPEATED_YAML_DIRECTIVE);
				return true;
			}
			
			return "  No exception was thrown";
		}
		
		// 6.16
		TEST TagDirective() { return "  not written yet"; }
		
		// 6.17
		TEST InvalidRepeatedTagDirective() { return "  not written yet"; }
		
		// 6.18
		TEST PrimaryTagHandle() { return "  not written yet"; }
		
		// 6.19
		TEST SecondaryTagHandle() { return "  not written yet"; }
		
		// 6.20
		TEST TagHandles() { return "  not written yet"; }
		
		// 6.21
		TEST LocalTagPrefix() { return "  not written yet"; }
		
		// 6.22
		TEST GlobalTagPrefix() { return "  not written yet"; }
		
		// 6.23
		TEST NodeProperties() { return "  not written yet"; }
		
		// 6.24
		TEST VerbatimTags() { return "  not written yet"; }
		
		// 6.25
		TEST InvalidVerbatimTags() { return "  not written yet"; }
		
		// 6.26
		TEST TagShorthands() { return "  not written yet"; }
		
		// 6.27
		TEST InvalidTagShorthands() { return "  not written yet"; }
		
		// 6.28
		TEST NonSpecificTags() { return "  not written yet"; }
		
		// 6.29
		TEST NodeAnchors() {
			YAML::Node doc = YAML::Parse(ex6_29);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["First occurrence"].as<std::string>() == "Value");
			YAML_ASSERT(doc["Second occurrence"].as<std::string>() == "Value");
			return true;
		}
		
		// 7.1
		TEST AliasNodes() {
			YAML::Node doc = YAML::Parse(ex7_1);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc["First occurrence"].as<std::string>() == "Foo");
			YAML_ASSERT(doc["Second occurrence"].as<std::string>() == "Foo");
			YAML_ASSERT(doc["Override anchor"].as<std::string>() == "Bar");
			YAML_ASSERT(doc["Reuse anchor"].as<std::string>() == "Bar");
			return true;
		}
		
		// 7.2
		TEST EmptyNodes() { return "  not written yet"; }
		
		// 7.3
		TEST CompletelyEmptyNodes() {
			YAML::Node doc = YAML::Parse(ex7_3);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["foo"].Type() == YAML::NodeType::Null);
//			YAML_ASSERT(doc[YAML::Null].as<std::string>() == "bar");
			return "  null as a key not implemented";
			return true;
		}
		
		// 7.4
		TEST DoubleQuotedImplicitKeys() {
			YAML::Node doc = YAML::Parse(ex7_4);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].as<std::string>() == "value");
			return true;
		}
		
		// 7.5
		TEST DoubleQuotedLineBreaks() {
			YAML::Node doc = YAML::Parse(ex7_5);
			YAML_ASSERT(doc.as<std::string>() == "folded to a space,\nto a line feed, or \t \tnon-content");
			return true;
		}
		
		// 7.6
		TEST DoubleQuotedLines() {
			YAML::Node doc = YAML::Parse(ex7_6);
			YAML_ASSERT(doc.as<std::string>() == " 1st non-empty\n2nd non-empty 3rd non-empty ");
			return true;
		}
		
		// 7.7
		TEST SingleQuotedCharacters() {
			YAML::Node doc = YAML::Parse(ex7_7);
			YAML_ASSERT(doc.as<std::string>() == "here's to \"quotes\"");
			return true;
		}
		
		// 7.8
		TEST SingleQuotedImplicitKeys() {
			YAML::Node doc = YAML::Parse(ex7_8);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].as<std::string>() == "value");
			return true;
		}
		
		// 7.9
		TEST SingleQuotedLines() {
			YAML::Node doc = YAML::Parse(ex7_9);
			YAML_ASSERT(doc.as<std::string>() == " 1st non-empty\n2nd non-empty 3rd non-empty ");
			return true;
		}
		
		// 7.10
		TEST PlainCharacters() {
			YAML::Node doc = YAML::Parse(ex7_10);
			YAML_ASSERT(doc.size() == 6);
			YAML_ASSERT(doc[0].as<std::string>() == "::vector");
			YAML_ASSERT(doc[1].as<std::string>() == ": - ()");
			YAML_ASSERT(doc[2].as<std::string>() == "Up, up, and away!");
			YAML_ASSERT(doc[3].as<int>() == -123);
			YAML_ASSERT(doc[4].as<std::string>() == "http://example.com/foo#bar");
			YAML_ASSERT(doc[5].size() == 5);
			YAML_ASSERT(doc[5][0].as<std::string>() == "::vector");
			YAML_ASSERT(doc[5][1].as<std::string>() == ": - ()");
			YAML_ASSERT(doc[5][2].as<std::string>() == "Up, up, and away!");
			YAML_ASSERT(doc[5][3].as<int>() == -123);
			YAML_ASSERT(doc[5][4].as<std::string>() == "http://example.com/foo#bar");
			return true;
		}
		
		// 7.11
		TEST PlainImplicitKeys() {
			YAML::Node doc = YAML::Parse(ex7_11);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].as<std::string>() == "value");
			return true;
		}
		
		// 7.12
		TEST PlainLines() {
			YAML::Node doc = YAML::Parse(ex7_12);
			YAML_ASSERT(doc.as<std::string>() == "1st non-empty\n2nd non-empty 3rd non-empty");
			return true;
		}
		
		// 7.13
		TEST FlowSequence() {
			YAML::Node doc = YAML::Parse(ex7_13);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0][0].as<std::string>() == "one");
			YAML_ASSERT(doc[0][1].as<std::string>() == "two");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1][0].as<std::string>() == "three");
			YAML_ASSERT(doc[1][1].as<std::string>() == "four");
			return true;
		}
		
		// 7.14
		TEST FlowSequenceEntries() {
			YAML::Node doc = YAML::Parse(ex7_14);
			YAML_ASSERT(doc.size() == 5);
			YAML_ASSERT(doc[0].as<std::string>() == "double quoted");
			YAML_ASSERT(doc[1].as<std::string>() == "single quoted");
			YAML_ASSERT(doc[2].as<std::string>() == "plain text");
			YAML_ASSERT(doc[3].size() == 1);
			YAML_ASSERT(doc[3][0].as<std::string>() == "nested");
			YAML_ASSERT(doc[4].size() == 1);
			YAML_ASSERT(doc[4]["single"].as<std::string>() == "pair");
			return true;
		}
		
		// 7.15
		TEST FlowMappings() {
			YAML::Node doc = YAML::Parse(ex7_15);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0]["one"].as<std::string>() == "two");
			YAML_ASSERT(doc[0]["three"].as<std::string>() == "four");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1]["five"].as<std::string>() == "six");
			YAML_ASSERT(doc[1]["seven"].as<std::string>() == "eight");
			return true;
		}
		
		// 7.16
		TEST FlowMappingEntries() {
			YAML::Node doc = YAML::Parse(ex7_16);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["explicit"].as<std::string>() == "entry");
			YAML_ASSERT(doc["implicit"].as<std::string>() == "entry");
//			YAML_ASSERT(doc[YAML::Null].Type() == YAML::NodeType::Null);
			return "  null as a key not implemented";
			return true;
		}
		
		// 7.17
		TEST FlowMappingSeparateValues() {
			YAML::Node doc = YAML::Parse(ex7_17);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc["unquoted"].as<std::string>() == "separate");
			YAML_ASSERT(doc["http://foo.com"].Type() == YAML::NodeType::Null);
			YAML_ASSERT(doc["omitted value"].Type() == YAML::NodeType::Null);
//			YAML_ASSERT(doc[YAML::Null].as<std::string>() == "omitted key");
			return "  null as a key not implemented";
			return true;
		}
		
		// 7.18
		TEST FlowMappingAdjacentValues() {
			YAML::Node doc = YAML::Parse(ex7_18);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["adjacent"].as<std::string>() == "value");
			YAML_ASSERT(doc["readable"].as<std::string>() == "value");
			YAML_ASSERT(doc["empty"].Type() == YAML::NodeType::Null);
			return true;
		}
		
		// 7.19
		TEST SinglePairFlowMappings() {
			YAML::Node doc = YAML::Parse(ex7_19);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo"].as<std::string>() == "bar");
			return true;
		}
		
		// 7.20
		TEST SinglePairExplicitEntry() {
			YAML::Node doc = YAML::Parse(ex7_20);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo bar"].as<std::string>() == "baz");
			return true;
		}
		
		// 7.21
		TEST SinglePairImplicitEntries() {
			YAML::Node doc = YAML::Parse(ex7_21);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0][0].size() == 1);
			YAML_ASSERT(doc[0][0]["YAML"].as<std::string>() == "separate");
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1][0].size() == 1);
//			YAML_ASSERT(doc[1][0][YAML::Null].as<std::string>() == "empty key entry");
			return "  null as a key not implemented";
			YAML_ASSERT(doc[2].size() == 1);
			YAML_ASSERT(doc[2][0].size() == 1);
			
			std::map<std::string, std::string> key;
			key["JSON"] = "like";
			YAML_ASSERT(doc[2][0][key].as<std::string>() == "adjacent");
			return true;
		}
		
		// 7.22
		TEST InvalidImplicitKeys() {
			try {
				YAML::Parse(ex7_22);
			} catch(const YAML::Exception& e) {
				if(e.msg == YAML::ErrorMsg::END_OF_SEQ_FLOW)
					return true;
				
				throw;
			}
			return "  no exception thrown";
		}
		
		// 7.23
		TEST FlowContent() {
			YAML::Node doc = YAML::Parse(ex7_23);
			YAML_ASSERT(doc.size() == 5);
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0][0].as<std::string>() == "a");
			YAML_ASSERT(doc[0][1].as<std::string>() == "b");
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1]["a"].as<std::string>() == "b");
			YAML_ASSERT(doc[2].as<std::string>() == "a");
			YAML_ASSERT(doc[3].as<char>() == 'b');
			YAML_ASSERT(doc[4].as<std::string>() == "c");
			return true;
		}
		
		// 7.24
		TEST FlowNodes() { return "  not written yet"; }
		
		// 8.1
		TEST BlockScalarHeader() { return "  not written yet"; }
		
		// 8.2
		TEST BlockIndentationHeader() { return "  not written yet"; }
		
		// 8.3
		TEST InvalidBlockScalarIndentationIndicators() { return "  not written yet"; }
		
		// 8.4
		TEST ChompingFinalLineBreak() { return "  not written yet"; }
		
		// 8.5
		TEST ChompingTrailingLines() { return "  not written yet"; }
		
		// 8.6
		TEST EmptyScalarChomping() { return "  not written yet"; }
		
		// 8.7
		TEST LiteralScalar() { return "  not written yet"; }
		
		// 8.8
		TEST LiteralContent() { return "  not written yet"; }
		
		// 8.9
		TEST FoldedScalar() { return "  not written yet"; }
		
		// 8.10
		TEST FoldedLines() { return "  not written yet"; }
		
		// 8.11
		TEST MoreIndentedLines() { return "  not written yet"; }
		
		// 8.12
		TEST EmptySeparationLines() { return "  not written yet"; }
		
		// 8.13
		TEST FinalEmptyLines() { return "  not written yet"; }
		
		// 8.14
		TEST BlockSequence() { return "  not written yet"; }
		
		// 8.15
		TEST BlockSequenceEntryTypes() { return "  not written yet"; }
		
		// 8.16
		TEST BlockMappings() { return "  not written yet"; }
		
		// 8.17
		TEST ExplicitBlockMappingEntries() { return "  not written yet"; }
		
		// 8.18
		TEST ImplicitBlockMappingEntries() { return "  not written yet"; }
		
		// 8.19
		TEST CompactBlockMappings() { return "  not written yet"; }
		
		// 8.20
		TEST BlockNodeTypes() { return "  not written yet"; }
		
		// 8.21
		TEST BlockScalarNodes() { return "  not written yet"; }
		
		// 8.22
		TEST BlockCollectionNodes() { return "  not written yet"; }
	}
}
