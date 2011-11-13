#include "spectests.h"
#include "specexamples.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

#define YAML_ASSERT(cond) do { if(!(cond)) return "  Assert failed: " #cond; } while(false)

namespace Test
{
	namespace Spec
	{
		// 2.1
		TEST SeqScalars() {
			YAML::Node doc = YAML::Load(ex2_1);
			YAML_ASSERT(doc.IsSequence());
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].as<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc[1].as<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc[2].as<std::string>() == "Ken Griffey");
			return true;
		}
		
		// 2.2
		TEST MappingScalarsToScalars() {
			YAML::Node doc = YAML::Load(ex2_2);
			YAML_ASSERT(doc.IsMap());
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["hr"].as<std::string>() == "65");
			YAML_ASSERT(doc["avg"].as<std::string>() == "0.278");
			YAML_ASSERT(doc["rbi"].as<std::string>() == "147");
			return true;
		}
		
		// 2.3
		TEST MappingScalarsToSequences() {
			YAML::Node doc = YAML::Load(ex2_3);
			YAML_ASSERT(doc.IsMap());
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
			YAML::Node doc = YAML::Load(ex2_4);
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
			YAML::Node doc = YAML::Load(ex2_5);
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
			YAML::Node doc = YAML::Load(ex2_6);
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
			std::vector<YAML::Node> docs = YAML::LoadAll(ex2_7);
			YAML_ASSERT(docs.size() == 2);
			
			{
				YAML::Node doc = docs[0];
				YAML_ASSERT(doc.size() == 3);
				YAML_ASSERT(doc[0].as<std::string>() == "Mark McGwire");
				YAML_ASSERT(doc[1].as<std::string>() == "Sammy Sosa");
				YAML_ASSERT(doc[2].as<std::string>() == "Ken Griffey");
			}

			{
				YAML::Node doc = docs[1];
				YAML_ASSERT(doc.size() == 2);
				YAML_ASSERT(doc[0].as<std::string>() == "Chicago Cubs");
				YAML_ASSERT(doc[1].as<std::string>() == "St Louis Cardinals");
			}
			return true;
		}
		
		// 2.8
		TEST PlayByPlayFeed() {
			std::vector<YAML::Node> docs = YAML::LoadAll(ex2_8);
			YAML_ASSERT(docs.size() == 2);
			
			{
				YAML::Node doc = docs[0];
				YAML_ASSERT(doc.size() == 3);
				YAML_ASSERT(doc["time"].as<std::string>() == "20:03:20");
				YAML_ASSERT(doc["player"].as<std::string>() == "Sammy Sosa");
				YAML_ASSERT(doc["action"].as<std::string>() == "strike (miss)");
			}

			{
				YAML::Node doc = docs[1];
				YAML_ASSERT(doc.size() == 3);
				YAML_ASSERT(doc["time"].as<std::string>() == "20:03:47");
				YAML_ASSERT(doc["player"].as<std::string>() == "Sammy Sosa");
				YAML_ASSERT(doc["action"].as<std::string>() == "grand slam");
			}
			return true;
		}
		
		// 2.9
		TEST SingleDocumentWithTwoComments() {
			YAML::Node doc = YAML::Load(ex2_9);
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
			YAML::Node doc = YAML::Load(ex2_10);
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
			YAML::Node doc = YAML::Load(ex2_11);

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
			YAML::Node doc = YAML::Load(ex2_12);
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
			YAML::Node doc = YAML::Load(ex2_13);
			YAML_ASSERT(doc.as<std::string>() ==
						"\\//||\\/||\n"
						"// ||  ||__");
			return true;
		}
		
		// 2.14
		TEST InFoldedScalarsNewlinesBecomeSpaces() {
			YAML::Node doc = YAML::Load(ex2_14);
			YAML_ASSERT(doc.as<std::string>() == "Mark McGwire's year was crippled by a knee injury.");
			return true;
		}
		
		// 2.15
		TEST FoldedNewlinesArePreservedForMoreIndentedAndBlankLines() {
			YAML::Node doc = YAML::Load(ex2_15);
			YAML_ASSERT(doc.as<std::string>() ==
						"Sammy Sosa completed another fine season with great stats.\n\n"
						"  63 Home Runs\n"
						"  0.288 Batting Average\n\n"
						"What a year!");
			return true;
		}
		
		// 2.16
		TEST IndentationDeterminesScope() {
			YAML::Node doc = YAML::Load(ex2_16);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["name"].as<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc["accomplishment"].as<std::string>() == "Mark set a major league home run record in 1998.\n");
			YAML_ASSERT(doc["stats"].as<std::string>() == "65 Home Runs\n0.278 Batting Average\n");
			return true;
		}
		
		// 2.17
		TEST QuotedScalars() {
			YAML::Node doc = YAML::Load(ex2_17);
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
			YAML::Node doc = YAML::Load(ex2_18);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["plain"].as<std::string>() == "This unquoted scalar spans many lines.");
			YAML_ASSERT(doc["quoted"].as<std::string>() == "So does this quoted scalar.\n");
			return true;
		}
		
		// TODO: 2.19 - 2.22 schema tags
		
		// 2.23
		TEST VariousExplicitTags() {
			YAML::Node doc = YAML::Load(ex2_23);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["not-date"].Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc["not-date"].as<std::string>() == "2002-04-28");
			YAML_ASSERT(doc["picture"].Tag() == "tag:yaml.org,2002:binary");
			YAML_ASSERT(doc["picture"].as<std::string>() ==
						"R0lGODlhDAAMAIQAAP//9/X\n"
						"17unp5WZmZgAAAOfn515eXv\n"
						"Pz7Y6OjuDg4J+fn5OTk6enp\n"
						"56enmleECcgggoBADs=\n"
						);
			YAML_ASSERT(doc["application specific tag"].Tag() == "!something");
			YAML_ASSERT(doc["application specific tag"].as<std::string>() ==
						"The semantics of the tag\n"
						"above may be different for\n"
						"different documents."
						);
			return true;
		}
		
		// 2.24
		TEST GlobalTags() {
			YAML::Node doc = YAML::Load(ex2_24);
			YAML_ASSERT(doc.Tag() == "tag:clarkevans.com,2002:shape");
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].Tag() == "tag:clarkevans.com,2002:circle");
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0]["center"].size() == 2);
			YAML_ASSERT(doc[0]["center"]["x"].as<int>() == 73);
			YAML_ASSERT(doc[0]["center"]["y"].as<int>() == 129);
			YAML_ASSERT(doc[0]["radius"].as<int>() == 7);
			YAML_ASSERT(doc[1].Tag() == "tag:clarkevans.com,2002:line");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1]["start"].size() == 2);
			YAML_ASSERT(doc[1]["start"]["x"].as<int>() == 73);
			YAML_ASSERT(doc[1]["start"]["y"].as<int>() == 129);
			YAML_ASSERT(doc[1]["finish"].size() == 2);
			YAML_ASSERT(doc[1]["finish"]["x"].as<int>() == 89);
			YAML_ASSERT(doc[1]["finish"]["y"].as<int>() == 102);
			YAML_ASSERT(doc[2].Tag() == "tag:clarkevans.com,2002:label");
			YAML_ASSERT(doc[2].size() == 3);
			YAML_ASSERT(doc[2]["start"].size() == 2);
			YAML_ASSERT(doc[2]["start"]["x"].as<int>() == 73);
			YAML_ASSERT(doc[2]["start"]["y"].as<int>() == 129);
			YAML_ASSERT(doc[2]["color"].as<std::string>() == "0xFFEEBB");
			YAML_ASSERT(doc[2]["text"].as<std::string>() == "Pretty vector drawing.");
			return true;
		}
		
		// 2.25
		TEST UnorderedSets() {
			YAML::Node doc = YAML::Load(ex2_25);
			YAML_ASSERT(doc.Tag() == "tag:yaml.org,2002:set");
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["Mark McGwire"].IsNull());
			YAML_ASSERT(doc["Sammy Sosa"].IsNull());
			YAML_ASSERT(doc["Ken Griffey"].IsNull());
			return true;
		}
		
		// 2.26
		TEST OrderedMappings() {
			YAML::Node doc = YAML::Load(ex2_26);
			YAML_ASSERT(doc.Tag() == "tag:yaml.org,2002:omap");
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["Mark McGwire"].as<int>() == 65);
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1]["Sammy Sosa"].as<int>() == 63);
			YAML_ASSERT(doc[2].size() == 1);
			YAML_ASSERT(doc[2]["Ken Griffey"].as<int>() == 58);
			return true;
		}
		
		// 2.27
		TEST Invoice() {
			YAML::Node doc = YAML::Load(ex2_27);
			YAML_ASSERT(doc.Tag() == "tag:clarkevans.com,2002:invoice");
			YAML_ASSERT(doc.size() == 8);
			YAML_ASSERT(doc["invoice"].as<int>() == 34843);
			YAML_ASSERT(doc["date"].as<std::string>() == "2001-01-23");
			YAML_ASSERT(doc["bill-to"].size() == 3);
			YAML_ASSERT(doc["bill-to"]["given"].as<std::string>() == "Chris");
			YAML_ASSERT(doc["bill-to"]["family"].as<std::string>() == "Dumars");
			YAML_ASSERT(doc["bill-to"]["address"].size() == 4);
			YAML_ASSERT(doc["bill-to"]["address"]["lines"].as<std::string>() == "458 Walkman Dr.\nSuite #292\n");
			YAML_ASSERT(doc["bill-to"]["address"]["city"].as<std::string>() == "Royal Oak");
			YAML_ASSERT(doc["bill-to"]["address"]["state"].as<std::string>() == "MI");
			YAML_ASSERT(doc["bill-to"]["address"]["postal"].as<std::string>() == "48046");
			YAML_ASSERT(doc["ship-to"].size() == 3);
			YAML_ASSERT(doc["ship-to"]["given"].as<std::string>() == "Chris");
			YAML_ASSERT(doc["ship-to"]["family"].as<std::string>() == "Dumars");
			YAML_ASSERT(doc["ship-to"]["address"].size() == 4);
			YAML_ASSERT(doc["ship-to"]["address"]["lines"].as<std::string>() == "458 Walkman Dr.\nSuite #292\n");
			YAML_ASSERT(doc["ship-to"]["address"]["city"].as<std::string>() == "Royal Oak");
			YAML_ASSERT(doc["ship-to"]["address"]["state"].as<std::string>() == "MI");
			YAML_ASSERT(doc["ship-to"]["address"]["postal"].as<std::string>() == "48046");
			YAML_ASSERT(doc["product"].size() == 2);
			YAML_ASSERT(doc["product"][0].size() == 4);
			YAML_ASSERT(doc["product"][0]["sku"].as<std::string>() == "BL394D");
			YAML_ASSERT(doc["product"][0]["quantity"].as<int>() == 4);
			YAML_ASSERT(doc["product"][0]["description"].as<std::string>() == "Basketball");
			YAML_ASSERT(doc["product"][0]["price"].as<std::string>() == "450.00");
			YAML_ASSERT(doc["product"][1].size() == 4);
			YAML_ASSERT(doc["product"][1]["sku"].as<std::string>() == "BL4438H");
			YAML_ASSERT(doc["product"][1]["quantity"].as<int>() == 1);
			YAML_ASSERT(doc["product"][1]["description"].as<std::string>() == "Super Hoop");
			YAML_ASSERT(doc["product"][1]["price"].as<std::string>() == "2392.00");
			YAML_ASSERT(doc["tax"].as<std::string>() == "251.42");
			YAML_ASSERT(doc["total"].as<std::string>() == "4443.52");
			YAML_ASSERT(doc["comments"].as<std::string>() == "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.");
			return true;
		}
		
		// 2.28
		TEST LogFile() {
			std::vector<YAML::Node> docs = YAML::LoadAll(ex2_28);
			YAML_ASSERT(docs.size() == 3);

			{
				YAML::Node doc = docs[0];
				YAML_ASSERT(doc.size() == 3);
				YAML_ASSERT(doc["Time"].as<std::string>() == "2001-11-23 15:01:42 -5");
				YAML_ASSERT(doc["User"].as<std::string>() == "ed");
				YAML_ASSERT(doc["Warning"].as<std::string>() == "This is an error message for the log file");
			}

			{
				YAML::Node doc = docs[1];
				YAML_ASSERT(doc.size() == 3);
				YAML_ASSERT(doc["Time"].as<std::string>() == "2001-11-23 15:02:31 -5");
				YAML_ASSERT(doc["User"].as<std::string>() == "ed");
				YAML_ASSERT(doc["Warning"].as<std::string>() == "A slightly different error message.");
			}

			{
				YAML::Node doc = docs[2];
				YAML_ASSERT(doc.size() == 4);
				YAML_ASSERT(doc["Date"].as<std::string>() == "2001-11-23 15:03:17 -5");
				YAML_ASSERT(doc["User"].as<std::string>() == "ed");
				YAML_ASSERT(doc["Fatal"].as<std::string>() == "Unknown variable \"bar\"");
				YAML_ASSERT(doc["Stack"].size() == 2);
				YAML_ASSERT(doc["Stack"][0].size() == 3);
				YAML_ASSERT(doc["Stack"][0]["file"].as<std::string>() == "TopClass.py");
				YAML_ASSERT(doc["Stack"][0]["line"].as<std::string>() == "23");
				YAML_ASSERT(doc["Stack"][0]["code"].as<std::string>() == "x = MoreObject(\"345\\n\")\n");
				YAML_ASSERT(doc["Stack"][1].size() == 3);
				YAML_ASSERT(doc["Stack"][1]["file"].as<std::string>() == "MoreClass.py");
				YAML_ASSERT(doc["Stack"][1]["line"].as<std::string>() == "58");
				YAML_ASSERT(doc["Stack"][1]["code"].as<std::string>() == "foo = bar");
			}
			return true;
		}
		
		// TODO: 5.1 - 5.2 BOM
		
		// 5.3
		TEST BlockStructureIndicators() {
			YAML::Node doc = YAML::Load(ex5_3);
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
			YAML::Node doc = YAML::Load(ex5_4);
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
			YAML::Node doc = YAML::Load(ex5_5);
			YAML_ASSERT(doc.IsNull());
			return true;
		}
		
		// 5.6
		TEST NodePropertyIndicators() {
			YAML::Node doc = YAML::Load(ex5_6);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["anchored"].as<std::string>() == "value"); // TODO: assert tag
			YAML_ASSERT(doc["alias"].as<std::string>() == "value");
			return true;
		}
		
		// 5.7
		TEST BlockScalarIndicators() {
			YAML::Node doc = YAML::Load(ex5_7);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["literal"].as<std::string>() == "some\ntext\n");
			YAML_ASSERT(doc["folded"].as<std::string>() == "some text\n");
			return true;
		}
		
		// 5.8
		TEST QuotedScalarIndicators() {
			YAML::Node doc = YAML::Load(ex5_8);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["single"].as<std::string>() == "text");
			YAML_ASSERT(doc["double"].as<std::string>() == "text");
			return true;
		}
		
		// TODO: 5.9 directive
		// TODO: 5.10 reserved indicator
		
		// 5.11
		TEST LineBreakCharacters() {
			YAML::Node doc = YAML::Load(ex5_11);
			YAML_ASSERT(doc.as<std::string>() == "Line break (no glyph)\nLine break (glyphed)\n");
			return true;
		}
		
		// 5.12
		TEST TabsAndSpaces() {
			YAML::Node doc = YAML::Load(ex5_12);
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
			YAML::Node doc = YAML::Load(ex5_13);
			YAML_ASSERT(doc.as<std::string>() == "Fun with \x5C \x22 \x07 \x08 \x1B \x0C \x0A \x0D \x09 \x0B " + std::string("\x00", 1) + " \x20 \xA0 \x85 \xe2\x80\xa8 \xe2\x80\xa9 A A A");
			return true;
		}
		
		// 5.14
		TEST InvalidEscapedCharacters() {
			try {
				YAML::Load(ex5_14);
			} catch(const YAML::ParserException& e) {
				YAML_ASSERT(e.msg == std::string(YAML::ErrorMsg::INVALID_ESCAPE) + "c");
				return true;
			}
			
			return false;
		}
		
		// 6.1
		TEST IndentationSpaces() {
			YAML::Node doc = YAML::Load(ex6_1);
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
			YAML::Node doc = YAML::Load(ex6_2);
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
			YAML::Node doc = YAML::Load(ex6_3);
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
			YAML::Node doc = YAML::Load(ex6_4);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["plain"].as<std::string>() == "text lines");
			YAML_ASSERT(doc["quoted"].as<std::string>() == "text lines");
			YAML_ASSERT(doc["block"].as<std::string>() == "text\n \tlines\n");
			return true;
		}
		
		// 6.5
		TEST EmptyLines() {
			YAML::Node doc = YAML::Load(ex6_5);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["Folding"].as<std::string>() == "Empty line\nas a line feed");
			YAML_ASSERT(doc["Chomping"].as<std::string>() == "Clipped empty lines\n");
			return true;
		}
		
		// 6.6
		TEST LineFolding() {
			YAML::Node doc = YAML::Load(ex6_6);
			YAML_ASSERT(doc.as<std::string>() == "trimmed\n\n\nas space");
			return true;
		}
		
		// 6.7
		TEST BlockFolding() {
			YAML::Node doc = YAML::Load(ex6_7);
			YAML_ASSERT(doc.as<std::string>() == "foo \n\n\t bar\n\nbaz\n");
			return true;
		}
		
		// 6.8
		TEST FlowFolding() {
			YAML::Node doc = YAML::Load(ex6_8);
			YAML_ASSERT(doc.as<std::string>() == " foo\nbar\nbaz ");
			return true;
		}
		
		// 6.9
		TEST SeparatedComment() {
			YAML::Node doc = YAML::Load(ex6_9);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["key"].as<std::string>() == "value");
			return true;
		}
		
		// 6.10
		TEST CommentLines() {
			YAML::Node doc = YAML::Load(ex6_10);
			YAML_ASSERT(doc.IsNull());
			return true;
		}
		
		// 6.11
		TEST MultiLineComments() {
			YAML::Node doc = YAML::Load(ex6_11);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["key"].as<std::string>() == "value");
			return true;
		}
		
		// 6.12
		TEST SeparationSpacesII() {
			YAML::Node doc = YAML::Load(ex6_12);
			
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
			YAML::Node doc = YAML::Load(ex6_13);
			YAML_ASSERT(doc.as<std::string>() == "foo");
			return true;
		}
		
		// 6.14
		TEST YAMLDirective() {
			YAML::Node doc = YAML::Load(ex6_14);
			YAML_ASSERT(doc.as<std::string>() == "foo");
			return true;
		}
		
		// 6.15
		TEST InvalidRepeatedYAMLDirective() {
			try {
				YAML::Load(ex6_15);
			} catch(const YAML::ParserException& e) {
				YAML_ASSERT(e.msg == YAML::ErrorMsg::REPEATED_YAML_DIRECTIVE);
				return true;
			}
			
			return "  No exception was thrown";
		}
		
		// 6.16
		TEST TagDirective() {
			YAML::Node doc = YAML::Load(ex6_16);
			YAML_ASSERT(doc.Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc.as<std::string>() == "foo");
			return true;
		}
		
		// 6.17
		TEST InvalidRepeatedTagDirective() {
			try {
				YAML::Load(ex6_17);
			} catch(const YAML::ParserException& e) {
				if(e.msg == YAML::ErrorMsg::REPEATED_TAG_DIRECTIVE)
					return true;
				
				throw;
			}
			
			return "  No exception was thrown";
		}
		
		// 6.18
		TEST PrimaryTagHandle() {
			std::vector<YAML::Node> docs = YAML::LoadAll(ex6_18);
			YAML_ASSERT(docs.size() == 2);
			
			{
				YAML::Node doc = docs[0];
				YAML_ASSERT(doc.Tag() == "!foo");
				YAML_ASSERT(doc.as<std::string>() == "bar");
			}

			{
				YAML::Node doc = docs[1];
				YAML_ASSERT(doc.Tag() == "tag:example.com,2000:app/foo");
				YAML_ASSERT(doc.as<std::string>() == "bar");
			}
			return true;
		}
		
		// 6.19
		TEST SecondaryTagHandle() {
			YAML::Node doc = YAML::Load(ex6_19);
			YAML_ASSERT(doc.Tag() == "tag:example.com,2000:app/int");
			YAML_ASSERT(doc.as<std::string>() == "1 - 3");
			return true;
		}
		
		// 6.20
		TEST TagHandles() {
			YAML::Node doc = YAML::Load(ex6_20);
			YAML_ASSERT(doc.Tag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc.as<std::string>() == "bar");
			return true;
		}
		
		// 6.21
		TEST LocalTagPrefix() {
			std::vector<YAML::Node> docs = YAML::LoadAll(ex6_21);
			YAML_ASSERT(docs.size() == 2);
			
			{
				YAML::Node doc = docs[0];
				YAML_ASSERT(doc.Tag() == "!my-light");
				YAML_ASSERT(doc.as<std::string>() == "fluorescent");
			}

			{
				YAML::Node doc = docs[1];
				YAML_ASSERT(doc.Tag() == "!my-light");
				YAML_ASSERT(doc.as<std::string>() == "green");
			}
			return true;
		}
		
		// 6.22
		TEST GlobalTagPrefix() {
			YAML::Node doc = YAML::Load(ex6_22);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].Tag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc[0].as<std::string>() == "bar");
			return true;
		}
		
		// 6.23
		TEST NodeProperties() {
			YAML::Node doc = YAML::Load(ex6_23);
			YAML_ASSERT(doc.size() == 2);
			for(YAML::const_iterator it=doc.begin();it!=doc.end();++it) {
				if(it->first.as<std::string>() == "foo") {
					YAML_ASSERT(it->first.Tag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it->second.Tag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it->second.as<std::string>() == "bar");
				} else if(it->first.as<std::string>() == "baz") {
					YAML_ASSERT(it->second.as<std::string>() == "foo");
				} else
					return "  unknown key";
			}
			
			return true;
		}
		
		// 6.24
		TEST VerbatimTags() {
			YAML::Node doc = YAML::Load(ex6_24);
			YAML_ASSERT(doc.size() == 1);
			for(YAML::const_iterator it=doc.begin();it!=doc.end();++it) {
				YAML_ASSERT(it->first.Tag() == "tag:yaml.org,2002:str");
				YAML_ASSERT(it->first.as<std::string>() == "foo");
				YAML_ASSERT(it->second.Tag() == "!bar");
				YAML_ASSERT(it->second.as<std::string>() == "baz");
			}
			return true;
		}
		
		// 6.25
		TEST InvalidVerbatimTags() {
			YAML::Node doc = YAML::Load(ex6_25);
			return "  not implemented yet"; // TODO: check tags (but we probably will say these are valid, I think)
		}
		
		// 6.26
		TEST TagShorthands() {
			YAML::Node doc = YAML::Load(ex6_26);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].Tag() == "!local");
			YAML_ASSERT(doc[0].as<std::string>() == "foo");
			YAML_ASSERT(doc[1].Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc[1].as<std::string>() == "bar");
			YAML_ASSERT(doc[2].Tag() == "tag:example.com,2000:app/tag%21");
			YAML_ASSERT(doc[2].as<std::string>() == "baz");
			return true;
		}
		
		// 6.27
		TEST InvalidTagShorthands() {
			bool threw = false;
			try {
				YAML::Load(ex6_27a);
			} catch(const YAML::ParserException& e) {
				threw = true;
				if(e.msg != YAML::ErrorMsg::TAG_WITH_NO_SUFFIX)
					throw;
			}
			
			if(!threw)
				return "  No exception was thrown for a tag with no suffix";
			
			YAML::Load(ex6_27b); // TODO: should we reject this one (since !h! is not declared)?
			return "  not implemented yet";
		}
		
		// 6.28
		TEST NonSpecificTags() {
			YAML::Node doc = YAML::Load(ex6_28);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].as<std::string>() == "12"); // TODO: check tags. How?
			YAML_ASSERT(doc[1].as<int>() == 12);
			YAML_ASSERT(doc[2].as<std::string>() == "12");
			return true;
		}
		
		// 6.29
		TEST NodeAnchors() {
			YAML::Node doc = YAML::Load(ex6_29);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["First occurrence"].as<std::string>() == "Value");
			YAML_ASSERT(doc["Second occurrence"].as<std::string>() == "Value");
			return true;
		}
		
		// 7.1
		TEST AliasNodes() {
			YAML::Node doc = YAML::Load(ex7_1);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc["First occurrence"].as<std::string>() == "Foo");
			YAML_ASSERT(doc["Second occurrence"].as<std::string>() == "Foo");
			YAML_ASSERT(doc["Override anchor"].as<std::string>() == "Bar");
			YAML_ASSERT(doc["Reuse anchor"].as<std::string>() == "Bar");
			return true;
		}
		
		// 7.2
		TEST EmptyNodes() {
			YAML::Node doc = YAML::Load(ex7_2);
			YAML_ASSERT(doc.size() == 2);
			for(YAML::const_iterator it=doc.begin();it!=doc.end();++it) {
				if(it->first.as<std::string>() == "foo") {
					YAML_ASSERT(it->second.Tag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it->second.as<std::string>() == "");
				} else if(it->first.as<std::string>() == "") {
					YAML_ASSERT(it->first.Tag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it->second.as<std::string>() == "bar");
				} else
					return "  unexpected key";
			}
			return true;
		}
		
		// 7.3
		TEST CompletelyEmptyNodes() {
			YAML::Node doc = YAML::Load(ex7_3);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["foo"].IsNull());
			YAML_ASSERT(doc[YAML::Null].as<std::string>() == "bar");
			return true;
		}
		
		// 7.4
		TEST DoubleQuotedImplicitKeys() {
			YAML::Node doc = YAML::Load(ex7_4);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].as<std::string>() == "value");
			return true;
		}
		
		// 7.5
		TEST DoubleQuotedLineBreaks() {
			YAML::Node doc = YAML::Load(ex7_5);
			YAML_ASSERT(doc.as<std::string>() == "folded to a space,\nto a line feed, or \t \tnon-content");
			return true;
		}
		
		// 7.6
		TEST DoubleQuotedLines() {
			YAML::Node doc = YAML::Load(ex7_6);
			YAML_ASSERT(doc.as<std::string>() == " 1st non-empty\n2nd non-empty 3rd non-empty ");
			return true;
		}
		
		// 7.7
		TEST SingleQuotedCharacters() {
			YAML::Node doc = YAML::Load(ex7_7);
			YAML_ASSERT(doc.as<std::string>() == "here's to \"quotes\"");
			return true;
		}
		
		// 7.8
		TEST SingleQuotedImplicitKeys() {
			YAML::Node doc = YAML::Load(ex7_8);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].as<std::string>() == "value");
			return true;
		}
		
		// 7.9
		TEST SingleQuotedLines() {
			YAML::Node doc = YAML::Load(ex7_9);
			YAML_ASSERT(doc.as<std::string>() == " 1st non-empty\n2nd non-empty 3rd non-empty ");
			return true;
		}
		
		// 7.10
		TEST PlainCharacters() {
			YAML::Node doc = YAML::Load(ex7_10);
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
			YAML::Node doc = YAML::Load(ex7_11);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].as<std::string>() == "value");
			return true;
		}
		
		// 7.12
		TEST PlainLines() {
			YAML::Node doc = YAML::Load(ex7_12);
			YAML_ASSERT(doc.as<std::string>() == "1st non-empty\n2nd non-empty 3rd non-empty");
			return true;
		}
		
		// 7.13
		TEST FlowSequence() {
			YAML::Node doc = YAML::Load(ex7_13);
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
			YAML::Node doc = YAML::Load(ex7_14);
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
			YAML::Node doc = YAML::Load(ex7_15);
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
			YAML::Node doc = YAML::Load(ex7_16);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["explicit"].as<std::string>() == "entry");
			YAML_ASSERT(doc["implicit"].as<std::string>() == "entry");
			YAML_ASSERT(doc[YAML::Null].IsNull());
			return true;
		}
		
		// 7.17
		TEST FlowMappingSeparateValues() {
			YAML::Node doc = YAML::Load(ex7_17);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc["unquoted"].as<std::string>() == "separate");
			YAML_ASSERT(doc["http://foo.com"].IsNull());
			YAML_ASSERT(doc["omitted value"].IsNull());
			YAML_ASSERT(doc[YAML::Null].as<std::string>() == "omitted key");
			return true;
		}
		
		// 7.18
		TEST FlowMappingAdjacentValues() {
			YAML::Node doc = YAML::Load(ex7_18);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["adjacent"].as<std::string>() == "value");
			YAML_ASSERT(doc["readable"].as<std::string>() == "value");
			YAML_ASSERT(doc["empty"].IsNull());
			return true;
		}
		
		// 7.19
		TEST SinglePairFlowMappings() {
			YAML::Node doc = YAML::Load(ex7_19);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo"].as<std::string>() == "bar");
			return true;
		}
		
		// 7.20
		TEST SinglePairExplicitEntry() {
			YAML::Node doc = YAML::Load(ex7_20);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo bar"].as<std::string>() == "baz");
			return true;
		}
		
		// 7.21
		TEST SinglePairImplicitEntries() {
			YAML::Node doc = YAML::Load(ex7_21);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0][0].size() == 1);
			YAML_ASSERT(doc[0][0]["YAML"].as<std::string>() == "separate");
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1][0].size() == 1);
			YAML_ASSERT(doc[1][0][YAML::Null].as<std::string>() == "empty key entry");
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
				YAML::Load(ex7_22);
			} catch(const YAML::Exception& e) {
				if(e.msg == YAML::ErrorMsg::END_OF_SEQ_FLOW)
					return true;
				
				throw;
			}
			return "  no exception thrown";
		}
		
		// 7.23
		TEST FlowContent() {
			YAML::Node doc = YAML::Load(ex7_23);
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
		TEST FlowNodes() {
			YAML::Node doc = YAML::Load(ex7_24);
			YAML_ASSERT(doc.size() == 5);
			YAML_ASSERT(doc[0].Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc[0].as<std::string>() == "a");
			YAML_ASSERT(doc[1].as<char>() == 'b');
			YAML_ASSERT(doc[2].as<std::string>() == "c");
			YAML_ASSERT(doc[3].as<std::string>() == "c");
			YAML_ASSERT(doc[4].Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc[4].as<std::string>() == "");
			return true;
		}

		// 8.1
		TEST BlockScalarHeader() {
			YAML::Node doc = YAML::Load(ex8_1);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc[0].as<std::string>() == "literal\n");
			YAML_ASSERT(doc[1].as<std::string>() == " folded\n");
			YAML_ASSERT(doc[2].as<std::string>() == "keep\n\n");
			YAML_ASSERT(doc[3].as<std::string>() == " strip");
			return true;
		}
		
		// 8.2
		TEST BlockIndentationHeader() {
			YAML::Node doc = YAML::Load(ex8_2);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc[0].as<std::string>() == "detected\n");
			YAML_ASSERT(doc[1].as<std::string>() == "\n\n# detected\n");
			YAML_ASSERT(doc[2].as<std::string>() == " explicit\n");
			YAML_ASSERT(doc[3].as<std::string>() == "\t\ndetected\n");
			return true;
		}
		
		// 8.3
		TEST InvalidBlockScalarIndentationIndicators() {
			{
				bool threw = false;
				try {
					YAML::Load(ex8_3a);
				} catch(const YAML::Exception& e) {
					if(e.msg != YAML::ErrorMsg::END_OF_SEQ)
						throw;
					
					threw = true;
				}
				
				if(!threw)
					return "  no exception thrown for less indented auto-detecting indentation for a literal block scalar";
			}
			
			{
				bool threw = false;
				try {
					YAML::Load(ex8_3b);
				} catch(const YAML::Exception& e) {
					if(e.msg != YAML::ErrorMsg::END_OF_SEQ)
						throw;
					
					threw = true;
				}
				
				if(!threw)
					return "  no exception thrown for less indented auto-detecting indentation for a folded block scalar";
			}
			
			{
				bool threw = false;
				try {
					YAML::Load(ex8_3c);
				} catch(const YAML::Exception& e) {
					if(e.msg != YAML::ErrorMsg::END_OF_SEQ)
						throw;
					
					threw = true;
				}
				
				if(!threw)
					return "  no exception thrown for less indented explicit indentation for a literal block scalar";
			}
			
			return true;
		}
		
		// 8.4
		TEST ChompingFinalLineBreak() {
			YAML::Node doc = YAML::Load(ex8_4);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["strip"].as<std::string>() == "text");
			YAML_ASSERT(doc["clip"].as<std::string>() == "text\n");
			YAML_ASSERT(doc["keep"].as<std::string>() == "text\n");
			return true;
		}
		
		// 8.5
		TEST ChompingTrailingLines() {
			YAML::Node doc = YAML::Load(ex8_5);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["strip"].as<std::string>() == "# text");
			YAML_ASSERT(doc["clip"].as<std::string>() == "# text\n");
			YAML_ASSERT(doc["keep"].as<std::string>() == "# text\n"); // Note: I believe this is a bug in the YAML spec - it should be "# text\n\n"
			return true;
		}
		
		// 8.6
		TEST EmptyScalarChomping() {
			YAML::Node doc = YAML::Load(ex8_6);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["strip"].as<std::string>() == "");
			YAML_ASSERT(doc["clip"].as<std::string>() == "");
			YAML_ASSERT(doc["keep"].as<std::string>() == "\n");
			return true;
		}
		
		// 8.7
		TEST LiteralScalar() {
			YAML::Node doc = YAML::Load(ex8_7);
			YAML_ASSERT(doc.as<std::string>() == "literal\n\ttext\n");
			return true;
		}
		
		// 8.8
		TEST LiteralContent() {
			YAML::Node doc = YAML::Load(ex8_8);
			YAML_ASSERT(doc.as<std::string>() == "\n\nliteral\n \n\ntext\n");
			return true;
		}
		
		// 8.9
		TEST FoldedScalar() {
			YAML::Node doc = YAML::Load(ex8_9);
			YAML_ASSERT(doc.as<std::string>() == "folded text\n");
			return true;
		}
		
		// 8.10
		TEST FoldedLines() {
			YAML::Node doc = YAML::Load(ex8_10);
			YAML_ASSERT(doc.as<std::string>() == "\nfolded line\nnext line\n  * bullet\n\n  * list\n  * lines\n\nlast line\n");
			return true;
		}
		
		// 8.11
		TEST MoreIndentedLines() {
			YAML::Node doc = YAML::Load(ex8_11);
			YAML_ASSERT(doc.as<std::string>() == "\nfolded line\nnext line\n  * bullet\n\n  * list\n  * lines\n\nlast line\n");
			return true;
		}
		
		// 8.12
		TEST EmptySeparationLines() {
			YAML::Node doc = YAML::Load(ex8_12);
			YAML_ASSERT(doc.as<std::string>() == "\nfolded line\nnext line\n  * bullet\n\n  * list\n  * lines\n\nlast line\n");
			return true;
		}
		
		// 8.13
		TEST FinalEmptyLines() {
			YAML::Node doc = YAML::Load(ex8_13);
			YAML_ASSERT(doc.as<std::string>() == "\nfolded line\nnext line\n  * bullet\n\n  * list\n  * lines\n\nlast line\n");
			return true;
		}
		
		// 8.14
		TEST BlockSequence() {
			YAML::Node doc = YAML::Load(ex8_14);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["block sequence"].size() == 2);
			YAML_ASSERT(doc["block sequence"][0].as<std::string>() == "one");
			YAML_ASSERT(doc["block sequence"][1].size() == 1);
			YAML_ASSERT(doc["block sequence"][1]["two"].as<std::string>() == "three");
			return true;
		}
		
		// 8.15
		TEST BlockSequenceEntryTypes() {
			YAML::Node doc = YAML::Load(ex8_15);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc[0].IsNull());
			YAML_ASSERT(doc[1].as<std::string>() == "block node\n");
			YAML_ASSERT(doc[2].size() == 2);
			YAML_ASSERT(doc[2][0].as<std::string>() == "one");
			YAML_ASSERT(doc[2][1].as<std::string>() == "two");
			YAML_ASSERT(doc[3].size() == 1);
			YAML_ASSERT(doc[3]["one"].as<std::string>() == "two");
			return true;
		}
		
		// 8.16
		TEST BlockMappings() {
			YAML::Node doc = YAML::Load(ex8_16);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["block mapping"].size() == 1);
			YAML_ASSERT(doc["block mapping"]["key"].as<std::string>() == "value");
			return true;
		}
		
		// 8.17
		TEST ExplicitBlockMappingEntries() {
			YAML::Node doc = YAML::Load(ex8_17);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["explicit key"].IsNull());
			YAML_ASSERT(doc["block key\n"].size() == 2);
			YAML_ASSERT(doc["block key\n"][0].as<std::string>() == "one");
			YAML_ASSERT(doc["block key\n"][1].as<std::string>() == "two");
			return true;
		}
		
		// 8.18
		TEST ImplicitBlockMappingEntries() {
			YAML::Node doc = YAML::Load(ex8_18);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["plain key"].as<std::string>() == "in-line value");
			YAML_ASSERT(doc[YAML::Null].IsNull());
			YAML_ASSERT(doc["quoted key"].size() == 1);
			YAML_ASSERT(doc["quoted key"][0].as<std::string>() == "entry");
			return true;
		}
		
		// 8.19
		TEST CompactBlockMappings() {
			YAML::Node doc = YAML::Load(ex8_19);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["sun"].as<std::string>() == "yellow");
			YAML_ASSERT(doc[1].size() == 1);
			std::map<std::string, std::string> key;
			key["earth"] = "blue";
			YAML_ASSERT(doc[1][key].size() == 1);
			YAML_ASSERT(doc[1][key]["moon"].as<std::string>() == "white");
			return true;
		}
		
		// 8.20
		TEST BlockNodeTypes() {
			YAML::Node doc = YAML::Load(ex8_20);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].as<std::string>() == "flow in block");
			YAML_ASSERT(doc[1].as<std::string>() == "Block scalar\n");
			YAML_ASSERT(doc[2].size() == 1);
			YAML_ASSERT(doc[2]["foo"].as<std::string>() == "bar");
			return true;
		}
		
		// 8.21
		TEST BlockScalarNodes() {
			YAML::Node doc = YAML::Load(ex8_21);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["literal"].as<std::string>() == "value"); // Note: I believe this is a bug in the YAML spec - it should be "value\n"
			YAML_ASSERT(doc["folded"].as<std::string>() == "value");
			YAML_ASSERT(doc["folded"].Tag() == "!foo");
			return true;
		}
		
		// 8.22
		TEST BlockCollectionNodes() {
			YAML::Node doc = YAML::Load(ex8_22);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["sequence"].size() == 2);
			YAML_ASSERT(doc["sequence"][0].as<std::string>() == "entry");
			YAML_ASSERT(doc["sequence"][1].size() == 1);
			YAML_ASSERT(doc["sequence"][1][0].as<std::string>() == "nested");
			YAML_ASSERT(doc["mapping"].size() == 1);
			YAML_ASSERT(doc["mapping"]["foo"].as<std::string>() == "bar");
			return true;
		}
	}
}
