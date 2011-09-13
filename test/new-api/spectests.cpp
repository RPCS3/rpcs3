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
		TEST BlockStructureIndicators() { return "  not written yet"; }
		
		// 5.4
		TEST FlowStructureIndicators() { return "  not written yet"; }
		
		// 5.5
		TEST CommentIndicator() { return "  not written yet"; }
		
		// 5.6
		TEST NodePropertyIndicators() { return "  not written yet"; }
		
		// 5.7
		TEST BlockScalarIndicators() { return "  not written yet"; }
		
		// 5.8
		TEST QuotedScalarIndicators() { return "  not written yet"; }
		
		// TODO: 5.9 directive
		// TODO: 5.10 reserved indicator
		
		// 5.11
		TEST LineBreakCharacters() { return "  not written yet"; }
		
		// 5.12
		TEST TabsAndSpaces() { return "  not written yet"; }
		
		// 5.13
		TEST EscapedCharacters() { return "  not written yet"; }
		
		// 5.14
		TEST InvalidEscapedCharacters() { return "  not written yet"; }
		
		// 6.1
		TEST IndentationSpaces() { return "  not written yet"; }
		
		// 6.2
		TEST IndentationIndicators() { return "  not written yet"; }
		
		// 6.3
		TEST SeparationSpaces() { return "  not written yet"; }
		
		// 6.4
		TEST LinePrefixes() { return "  not written yet"; }
		
		// 6.5
		TEST EmptyLines() { return "  not written yet"; }
		
		// 6.6
		TEST LineFolding() { return "  not written yet"; }
		
		// 6.7
		TEST BlockFolding() { return "  not written yet"; }
		
		// 6.8
		TEST FlowFolding() { return "  not written yet"; }
		
		// 6.9
		TEST SeparatedComment() { return "  not written yet"; }
		
		// 6.10
		TEST CommentLines() { return "  not written yet"; }
		
		// 6.11
		TEST MultiLineComments() { return "  not written yet"; }
		
		// 6.12
		TEST SeparationSpacesII() { return "  not written yet"; }
		
		// 6.13
		TEST ReservedDirectives() { return "  not written yet"; }
		
		// 6.14
		TEST YAMLDirective() { return "  not written yet"; }
		
		// 6.15
		TEST InvalidRepeatedYAMLDirective() { return "  not written yet"; }
		
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
		TEST NodeAnchors() { return "  not written yet"; }
		
		// 7.1
		TEST AliasNodes() { return "  not written yet"; }
		
		// 7.2
		TEST EmptyNodes() { return "  not written yet"; }
		
		// 7.3
		TEST CompletelyEmptyNodes() { return "  not written yet"; }
		
		// 7.4
		TEST DoubleQuotedImplicitKeys() { return "  not written yet"; }
		
		// 7.5
		TEST DoubleQuotedLineBreaks() { return "  not written yet"; }
		
		// 7.6
		TEST DoubleQuotedLines() { return "  not written yet"; }
		
		// 7.7
		TEST SingleQuotedCharacters() { return "  not written yet"; }
		
		// 7.8
		TEST SingleQuotedImplicitKeys() { return "  not written yet"; }
		
		// 7.9
		TEST SingleQuotedLines() { return "  not written yet"; }
		
		// 7.10
		TEST PlainCharacters() { return "  not written yet"; }
		
		// 7.11
		TEST PlainImplicitKeys() { return "  not written yet"; }
		
		// 7.12
		TEST PlainLines() { return "  not written yet"; }
		
		// 7.13
		TEST FlowSequence() { return "  not written yet"; }
		
		// 7.14
		TEST FlowSequenceEntries() { return "  not written yet"; }
		
		// 7.15
		TEST FlowMappings() { return "  not written yet"; }
		
		// 7.16
		TEST FlowMappingEntries() { return "  not written yet"; }
		
		// 7.17
		TEST FlowMappingSeparateValues() { return "  not written yet"; }
		
		// 7.18
		TEST FlowMappingAdjacentValues() { return "  not written yet"; }
		
		// 7.19
		TEST SinglePairFlowMappings() { return "  not written yet"; }
		
		// 7.20
		TEST SinglePairExplicitEntry() { return "  not written yet"; }
		
		// 7.21
		TEST SinglePairImplicitEntries() { return "  not written yet"; }
		
		// 7.22
		TEST InvalidImplicitKeys() { return "  not written yet"; }
		
		// 7.23
		TEST FlowContent() { return "  not written yet"; }
		
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
