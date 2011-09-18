#include "spectests.h"
#include "specexamples.h"
#include "yaml-cpp/yaml.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#define YAML_ASSERT(cond) do { if(!(cond)) return "  Assert failed: " #cond; } while(false)
#define PARSE(doc, input) \
	std::stringstream stream(input);\
	YAML::Parser parser(stream);\
	YAML::Node doc;\
	parser.GetNextDocument(doc)
#define PARSE_NEXT(doc) parser.GetNextDocument(doc)

namespace Test {
	namespace Spec {
		// 2.1
		TEST SeqScalars() {
			PARSE(doc, ex2_1);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].to<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc[1].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc[2].to<std::string>() == "Ken Griffey");
			return true;
		}
		
		// 2.2
		TEST MappingScalarsToScalars() {
			PARSE(doc, ex2_2);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["hr"].to<std::string>() == "65");
			YAML_ASSERT(doc["avg"].to<std::string>() == "0.278");
			YAML_ASSERT(doc["rbi"].to<std::string>() == "147");
			return true;
		}
		
		// 2.3
		TEST MappingScalarsToSequences() {
			PARSE(doc, ex2_3);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["american"].size() == 3);
			YAML_ASSERT(doc["american"][0].to<std::string>() == "Boston Red Sox");
			YAML_ASSERT(doc["american"][1].to<std::string>() == "Detroit Tigers");
			YAML_ASSERT(doc["american"][2].to<std::string>() == "New York Yankees");
			YAML_ASSERT(doc["national"].size() == 3);
			YAML_ASSERT(doc["national"][0].to<std::string>() == "New York Mets");
			YAML_ASSERT(doc["national"][1].to<std::string>() == "Chicago Cubs");
			YAML_ASSERT(doc["national"][2].to<std::string>() == "Atlanta Braves");
			return true;
		}
		
		// 2.4
		TEST SequenceOfMappings()
		{
			PARSE(doc, ex2_4);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 3);
			YAML_ASSERT(doc[0]["name"].to<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc[0]["hr"].to<std::string>() == "65");
			YAML_ASSERT(doc[0]["avg"].to<std::string>() == "0.278");
			YAML_ASSERT(doc[1].size() == 3);
			YAML_ASSERT(doc[1]["name"].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc[1]["hr"].to<std::string>() == "63");
			YAML_ASSERT(doc[1]["avg"].to<std::string>() == "0.288");
			return true;
		}
		
		// 2.5
		TEST SequenceOfSequences()
		{
			PARSE(doc, ex2_5);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 3);
			YAML_ASSERT(doc[0][0].to<std::string>() == "name");
			YAML_ASSERT(doc[0][1].to<std::string>() == "hr");
			YAML_ASSERT(doc[0][2].to<std::string>() == "avg");
			YAML_ASSERT(doc[1].size() == 3);
			YAML_ASSERT(doc[1][0].to<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc[1][1].to<std::string>() == "65");
			YAML_ASSERT(doc[1][2].to<std::string>() == "0.278");
			YAML_ASSERT(doc[2].size() == 3);
			YAML_ASSERT(doc[2][0].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc[2][1].to<std::string>() == "63");
			YAML_ASSERT(doc[2][2].to<std::string>() == "0.288");
			return true;
		}
		
		// 2.6
		TEST MappingOfMappings()
		{
			PARSE(doc, ex2_6);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["Mark McGwire"].size() == 2);
			YAML_ASSERT(doc["Mark McGwire"]["hr"].to<std::string>() == "65");
			YAML_ASSERT(doc["Mark McGwire"]["avg"].to<std::string>() == "0.278");
			YAML_ASSERT(doc["Sammy Sosa"].size() == 2);
			YAML_ASSERT(doc["Sammy Sosa"]["hr"].to<std::string>() == "63");
			YAML_ASSERT(doc["Sammy Sosa"]["avg"].to<std::string>() == "0.288");
			return true;
		}
		
		// 2.7
		TEST TwoDocumentsInAStream()
		{
			PARSE(doc, ex2_7);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].to<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc[1].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc[2].to<std::string>() == "Ken Griffey");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].to<std::string>() == "Chicago Cubs");
			YAML_ASSERT(doc[1].to<std::string>() == "St Louis Cardinals");
			return true;
		}
		
		// 2.8
		TEST PlayByPlayFeed()
		{
			PARSE(doc, ex2_8);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["time"].to<std::string>() == "20:03:20");
			YAML_ASSERT(doc["player"].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["action"].to<std::string>() == "strike (miss)");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["time"].to<std::string>() == "20:03:47");
			YAML_ASSERT(doc["player"].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["action"].to<std::string>() == "grand slam");
			return true;
		}
		
		// 2.9
		TEST SingleDocumentWithTwoComments()
		{
			PARSE(doc, ex2_9);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["hr"].size() == 2);
			YAML_ASSERT(doc["hr"][0].to<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc["hr"][1].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"].size() == 2);
			YAML_ASSERT(doc["rbi"][0].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"][1].to<std::string>() == "Ken Griffey");
			return true;
		}
		
		// 2.10
		TEST SimpleAnchor()
		{
			PARSE(doc, ex2_10);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["hr"].size() == 2);
			YAML_ASSERT(doc["hr"][0].to<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc["hr"][1].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"].size() == 2);
			YAML_ASSERT(doc["rbi"][0].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"][1].to<std::string>() == "Ken Griffey");
			return true;
		}
		
		struct Pair {
			Pair() {}
			Pair(const std::string& f, const std::string& s): first(f), second(s) {}
			std::string first, second;
		};
		
		bool operator == (const Pair& p, const Pair& q) {
			return p.first == q.first && p.second == q.second;
		}
		
		void operator >> (const YAML::Node& node, Pair& p) {
			node[0] >> p.first;
			node[1] >> p.second;
		}
		
		// 2.11
		TEST MappingBetweenSequences()
		{
			PARSE(doc, ex2_11);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[Pair("Detroit Tigers", "Chicago cubs")].size() == 1);
			YAML_ASSERT(doc[Pair("Detroit Tigers", "Chicago cubs")][0].to<std::string>() == "2001-07-23");
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")].size() == 3);
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")][0].to<std::string>() == "2001-07-02");
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")][1].to<std::string>() == "2001-08-12");
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")][2].to<std::string>() == "2001-08-14");
			return true;
		}
		
		// 2.12
		TEST CompactNestedMapping()
		{
			PARSE(doc, ex2_12);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0]["item"].to<std::string>() == "Super Hoop");
			YAML_ASSERT(doc[0]["quantity"].to<int>() == 1);
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1]["item"].to<std::string>() == "Basketball");
			YAML_ASSERT(doc[1]["quantity"].to<int>() == 4);
			YAML_ASSERT(doc[2].size() == 2);
			YAML_ASSERT(doc[2]["item"].to<std::string>() == "Big Shoes");
			YAML_ASSERT(doc[2]["quantity"].to<int>() == 1);
			return true;
		}
		
		// 2.13
		TEST InLiteralsNewlinesArePreserved()
		{
			PARSE(doc, ex2_13);
			YAML_ASSERT(doc.to<std::string>() ==
						"\\//||\\/||\n"
						"// ||  ||__");
			return true;
		}
		
		// 2.14
		TEST InFoldedScalarsNewlinesBecomeSpaces()
		{
			PARSE(doc, ex2_14);
			YAML_ASSERT(doc.to<std::string>() == "Mark McGwire's year was crippled by a knee injury.");
			return true;
		}
		
		// 2.15
		TEST FoldedNewlinesArePreservedForMoreIndentedAndBlankLines()
		{
			PARSE(doc, ex2_15);
			YAML_ASSERT(doc.to<std::string>() ==
						"Sammy Sosa completed another fine season with great stats.\n\n"
						"  63 Home Runs\n"
						"  0.288 Batting Average\n\n"
						"What a year!");
			return true;
		}
		
		// 2.16
		TEST IndentationDeterminesScope()
		{
			PARSE(doc, ex2_16);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["name"].to<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc["accomplishment"].to<std::string>() == "Mark set a major league home run record in 1998.\n");
			YAML_ASSERT(doc["stats"].to<std::string>() == "65 Home Runs\n0.278 Batting Average\n");
			return true;
		}
		
		// 2.17
		TEST QuotedScalars()
		{
			PARSE(doc, ex2_17);
			YAML_ASSERT(doc.size() == 6);
			YAML_ASSERT(doc["unicode"].to<std::string>() == "Sosa did fine.\xe2\x98\xba");
			YAML_ASSERT(doc["control"].to<std::string>() == "\b1998\t1999\t2000\n");
			YAML_ASSERT(doc["hex esc"].to<std::string>() == "\x0d\x0a is \r\n");
			YAML_ASSERT(doc["single"].to<std::string>() == "\"Howdy!\" he cried.");
			YAML_ASSERT(doc["quoted"].to<std::string>() == " # Not a 'comment'.");
			YAML_ASSERT(doc["tie-fighter"].to<std::string>() == "|\\-*-/|");
			return true;
		}
		
		// 2.18
		TEST MultiLineFlowScalars()
		{
			PARSE(doc, ex2_18);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["plain"].to<std::string>() == "This unquoted scalar spans many lines.");
			YAML_ASSERT(doc["quoted"].to<std::string>() == "So does this quoted scalar.\n");
			return true;
		}
		
		// TODO: 2.19 - 2.22 schema tags
		
		// 2.23
		TEST VariousExplicitTags()
		{
			PARSE(doc, ex2_23);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["not-date"].Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc["not-date"].to<std::string>() == "2002-04-28");
			YAML_ASSERT(doc["picture"].Tag() == "tag:yaml.org,2002:binary");
			YAML_ASSERT(doc["picture"].to<std::string>() ==
				"R0lGODlhDAAMAIQAAP//9/X\n"
				"17unp5WZmZgAAAOfn515eXv\n"
				"Pz7Y6OjuDg4J+fn5OTk6enp\n"
				"56enmleECcgggoBADs=\n"
			);
			YAML_ASSERT(doc["application specific tag"].Tag() == "!something");
			YAML_ASSERT(doc["application specific tag"].to<std::string>() ==
				"The semantics of the tag\n"
				"above may be different for\n"
				"different documents."
			);
			return true;
		}
		
		// 2.24
		TEST GlobalTags()
		{
			PARSE(doc, ex2_24);
			YAML_ASSERT(doc.Tag() == "tag:clarkevans.com,2002:shape");
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].Tag() == "tag:clarkevans.com,2002:circle");
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0]["center"].size() == 2);
			YAML_ASSERT(doc[0]["center"]["x"].to<int>() == 73);
			YAML_ASSERT(doc[0]["center"]["y"].to<int>() == 129);
			YAML_ASSERT(doc[0]["radius"].to<int>() == 7);
			YAML_ASSERT(doc[1].Tag() == "tag:clarkevans.com,2002:line");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1]["start"].size() == 2);
			YAML_ASSERT(doc[1]["start"]["x"].to<int>() == 73);
			YAML_ASSERT(doc[1]["start"]["y"].to<int>() == 129);
			YAML_ASSERT(doc[1]["finish"].size() == 2);
			YAML_ASSERT(doc[1]["finish"]["x"].to<int>() == 89);
			YAML_ASSERT(doc[1]["finish"]["y"].to<int>() == 102);
			YAML_ASSERT(doc[2].Tag() == "tag:clarkevans.com,2002:label");
			YAML_ASSERT(doc[2].size() == 3);
			YAML_ASSERT(doc[2]["start"].size() == 2);
			YAML_ASSERT(doc[2]["start"]["x"].to<int>() == 73);
			YAML_ASSERT(doc[2]["start"]["y"].to<int>() == 129);
			YAML_ASSERT(doc[2]["color"].to<std::string>() == "0xFFEEBB");
			YAML_ASSERT(doc[2]["text"].to<std::string>() == "Pretty vector drawing.");
			return true;
		}
		
		// 2.25
		TEST UnorderedSets()
		{
			PARSE(doc, ex2_25);
			YAML_ASSERT(doc.Tag() == "tag:yaml.org,2002:set");
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(IsNull(doc["Mark McGwire"]));
			YAML_ASSERT(IsNull(doc["Sammy Sosa"]));
			YAML_ASSERT(IsNull(doc["Ken Griffey"]));
			return true;
		}
		
		// 2.26
		TEST OrderedMappings()
		{
			PARSE(doc, ex2_26);
			YAML_ASSERT(doc.Tag() == "tag:yaml.org,2002:omap");
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["Mark McGwire"].to<int>() == 65);
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1]["Sammy Sosa"].to<int>() == 63);
			YAML_ASSERT(doc[2].size() == 1);
			YAML_ASSERT(doc[2]["Ken Griffey"].to<int>() == 58);
			return true;
		}
		
		// 2.27
		TEST Invoice()
		{
			PARSE(doc, ex2_27);
			YAML_ASSERT(doc.Tag() == "tag:clarkevans.com,2002:invoice");
			YAML_ASSERT(doc.size() == 8);
			YAML_ASSERT(doc["invoice"].to<int>() == 34843);
			YAML_ASSERT(doc["date"].to<std::string>() == "2001-01-23");
			YAML_ASSERT(doc["bill-to"].size() == 3);
			YAML_ASSERT(doc["bill-to"]["given"].to<std::string>() == "Chris");
			YAML_ASSERT(doc["bill-to"]["family"].to<std::string>() == "Dumars");
			YAML_ASSERT(doc["bill-to"]["address"].size() == 4);
			YAML_ASSERT(doc["bill-to"]["address"]["lines"].to<std::string>() == "458 Walkman Dr.\nSuite #292\n");
			YAML_ASSERT(doc["bill-to"]["address"]["city"].to<std::string>() == "Royal Oak");
			YAML_ASSERT(doc["bill-to"]["address"]["state"].to<std::string>() == "MI");
			YAML_ASSERT(doc["bill-to"]["address"]["postal"].to<std::string>() == "48046");
			YAML_ASSERT(doc["ship-to"].size() == 3);
			YAML_ASSERT(doc["ship-to"]["given"].to<std::string>() == "Chris");
			YAML_ASSERT(doc["ship-to"]["family"].to<std::string>() == "Dumars");
			YAML_ASSERT(doc["ship-to"]["address"].size() == 4);
			YAML_ASSERT(doc["ship-to"]["address"]["lines"].to<std::string>() == "458 Walkman Dr.\nSuite #292\n");
			YAML_ASSERT(doc["ship-to"]["address"]["city"].to<std::string>() == "Royal Oak");
			YAML_ASSERT(doc["ship-to"]["address"]["state"].to<std::string>() == "MI");
			YAML_ASSERT(doc["ship-to"]["address"]["postal"].to<std::string>() == "48046");
			YAML_ASSERT(doc["product"].size() == 2);
			YAML_ASSERT(doc["product"][0].size() == 4);
			YAML_ASSERT(doc["product"][0]["sku"].to<std::string>() == "BL394D");
			YAML_ASSERT(doc["product"][0]["quantity"].to<int>() == 4);
			YAML_ASSERT(doc["product"][0]["description"].to<std::string>() == "Basketball");
			YAML_ASSERT(doc["product"][0]["price"].to<std::string>() == "450.00");
			YAML_ASSERT(doc["product"][1].size() == 4);
			YAML_ASSERT(doc["product"][1]["sku"].to<std::string>() == "BL4438H");
			YAML_ASSERT(doc["product"][1]["quantity"].to<int>() == 1);
			YAML_ASSERT(doc["product"][1]["description"].to<std::string>() == "Super Hoop");
			YAML_ASSERT(doc["product"][1]["price"].to<std::string>() == "2392.00");
			YAML_ASSERT(doc["tax"].to<std::string>() == "251.42");
			YAML_ASSERT(doc["total"].to<std::string>() == "4443.52");
			YAML_ASSERT(doc["comments"].to<std::string>() == "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.");
			return true;
		}
		
		// 2.28
		TEST LogFile()
		{
			PARSE(doc, ex2_28);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["Time"].to<std::string>() == "2001-11-23 15:01:42 -5");
			YAML_ASSERT(doc["User"].to<std::string>() == "ed");
			YAML_ASSERT(doc["Warning"].to<std::string>() == "This is an error message for the log file");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["Time"].to<std::string>() == "2001-11-23 15:02:31 -5");
			YAML_ASSERT(doc["User"].to<std::string>() == "ed");
			YAML_ASSERT(doc["Warning"].to<std::string>() == "A slightly different error message.");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc["Date"].to<std::string>() == "2001-11-23 15:03:17 -5");
			YAML_ASSERT(doc["User"].to<std::string>() == "ed");
			YAML_ASSERT(doc["Fatal"].to<std::string>() == "Unknown variable \"bar\"");
			YAML_ASSERT(doc["Stack"].size() == 2);
			YAML_ASSERT(doc["Stack"][0].size() == 3);
			YAML_ASSERT(doc["Stack"][0]["file"].to<std::string>() == "TopClass.py");
			YAML_ASSERT(doc["Stack"][0]["line"].to<std::string>() == "23");
			YAML_ASSERT(doc["Stack"][0]["code"].to<std::string>() == "x = MoreObject(\"345\\n\")\n");
			YAML_ASSERT(doc["Stack"][1].size() == 3);
			YAML_ASSERT(doc["Stack"][1]["file"].to<std::string>() == "MoreClass.py");
			YAML_ASSERT(doc["Stack"][1]["line"].to<std::string>() == "58");
			YAML_ASSERT(doc["Stack"][1]["code"].to<std::string>() == "foo = bar");
			return true;
		}
		
		// TODO: 5.1 - 5.2 BOM
		
		// 5.3
		TEST BlockStructureIndicators()
		{
			PARSE(doc, ex5_3);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["sequence"].size() == 2);
			YAML_ASSERT(doc["sequence"][0].to<std::string>() == "one");
			YAML_ASSERT(doc["sequence"][1].to<std::string>() == "two");
			YAML_ASSERT(doc["mapping"].size() == 2);
			YAML_ASSERT(doc["mapping"]["sky"].to<std::string>() == "blue");
			YAML_ASSERT(doc["mapping"]["sea"].to<std::string>() == "green");
			return true;
		}
		
		// 5.4
		TEST FlowStructureIndicators()
		{
			PARSE(doc, ex5_4);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["sequence"].size() == 2);
			YAML_ASSERT(doc["sequence"][0].to<std::string>() == "one");
			YAML_ASSERT(doc["sequence"][1].to<std::string>() == "two");
			YAML_ASSERT(doc["mapping"].size() == 2);
			YAML_ASSERT(doc["mapping"]["sky"].to<std::string>() == "blue");
			YAML_ASSERT(doc["mapping"]["sea"].to<std::string>() == "green");
			return true;
		}
		
		// 5.5
		TEST CommentIndicator()
		{
			PARSE(doc, ex5_5);
			YAML_ASSERT(doc.size() == 0);
			return true;
		}
		
		// 5.6
		TEST NodePropertyIndicators()
		{
			PARSE(doc, ex5_6);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["anchored"].to<std::string>() == "value"); // TODO: assert tag
			YAML_ASSERT(doc["alias"].to<std::string>() == "value");
			return true;
		}
		
		// 5.7
		TEST BlockScalarIndicators()
		{
			PARSE(doc, ex5_7);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["literal"].to<std::string>() == "some\ntext\n");
			YAML_ASSERT(doc["folded"].to<std::string>() == "some text\n");
			return true;
		}
		
		// 5.8
		TEST QuotedScalarIndicators()
		{
			PARSE(doc, ex5_8);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["single"].to<std::string>() == "text");
			YAML_ASSERT(doc["double"].to<std::string>() == "text");
			return true;
		}
		
		// TODO: 5.9 directive
		// TODO: 5.10 reserved indicator
		
		// 5.11
		TEST LineBreakCharacters()
		{
			PARSE(doc, ex5_11);
			YAML_ASSERT(doc.to<std::string>() == "Line break (no glyph)\nLine break (glyphed)\n");
			return true;
		}
		
		// 5.12
		TEST TabsAndSpaces()
		{
			PARSE(doc, ex5_12);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["quoted"].to<std::string>() == "Quoted\t");
			YAML_ASSERT(doc["block"].to<std::string>() ==
						"void main() {\n"
						"\tprintf(\"Hello, world!\\n\");\n"
						"}");
			return true;
		}
		
		// 5.13
		TEST EscapedCharacters()
		{
			PARSE(doc, ex5_13);
			YAML_ASSERT(doc.to<std::string>() == "Fun with \x5C \x22 \x07 \x08 \x1B \x0C \x0A \x0D \x09 \x0B " + std::string("\x00", 1) + " \x20 \xA0 \x85 \xe2\x80\xa8 \xe2\x80\xa9 A A A");
			return true;
		}
		
		// 5.14
		TEST InvalidEscapedCharacters()
		{
			std::stringstream stream(ex5_14);
			try {
				YAML::Parser parser(stream);
				YAML::Node doc;
				parser.GetNextDocument(doc);
			} catch(const YAML::ParserException& e) {
				YAML_ASSERT(e.msg == std::string(YAML::ErrorMsg::INVALID_ESCAPE) + "c");
				return true;
			}
			
			return false;
		}
		
		// 6.1
		TEST IndentationSpaces()
		{
			PARSE(doc, ex6_1);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["Not indented"].size() == 2);
			YAML_ASSERT(doc["Not indented"]["By one space"].to<std::string>() == "By four\n  spaces\n");
			YAML_ASSERT(doc["Not indented"]["Flow style"].size() == 3);
			YAML_ASSERT(doc["Not indented"]["Flow style"][0].to<std::string>() == "By two");
			YAML_ASSERT(doc["Not indented"]["Flow style"][1].to<std::string>() == "Also by two");
			YAML_ASSERT(doc["Not indented"]["Flow style"][2].to<std::string>() == "Still by two");
			return true;
		}
		
		// 6.2
		TEST IndentationIndicators()
		{
			PARSE(doc, ex6_2);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["a"].size() == 2);
			YAML_ASSERT(doc["a"][0].to<std::string>() == "b");
			YAML_ASSERT(doc["a"][1].size() == 2);
			YAML_ASSERT(doc["a"][1][0].to<std::string>() == "c");
			YAML_ASSERT(doc["a"][1][1].to<std::string>() == "d");
			return true;
		}
		
		// 6.3
		TEST SeparationSpaces()
		{
			PARSE(doc, ex6_3);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo"].to<std::string>() == "bar");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1][0].to<std::string>() == "baz");
			YAML_ASSERT(doc[1][1].to<std::string>() == "baz");
			return true;
		}
		
		// 6.4
		TEST LinePrefixes()
		{
			PARSE(doc, ex6_4);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["plain"].to<std::string>() == "text lines");
			YAML_ASSERT(doc["quoted"].to<std::string>() == "text lines");
			YAML_ASSERT(doc["block"].to<std::string>() == "text\n \tlines\n");
			return true;
		}
		
		// 6.5
		TEST EmptyLines()
		{
			PARSE(doc, ex6_5);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["Folding"].to<std::string>() == "Empty line\nas a line feed");
			YAML_ASSERT(doc["Chomping"].to<std::string>() == "Clipped empty lines\n");
			return true;
		}
		
		// 6.6
		TEST LineFolding()
		{
			PARSE(doc, ex6_6);
			YAML_ASSERT(doc.to<std::string>() == "trimmed\n\n\nas space");
			return true;
		}
		
		// 6.7
		TEST BlockFolding()
		{
			PARSE(doc, ex6_7);
			YAML_ASSERT(doc.to<std::string>() == "foo \n\n\t bar\n\nbaz\n");
			return true;
		}
		
		// 6.8
		TEST FlowFolding()
		{
			PARSE(doc, ex6_8);			
			YAML_ASSERT(doc.to<std::string>() == " foo\nbar\nbaz ");
			return true;
		}
		
		// 6.9
		TEST SeparatedComment()
		{
			PARSE(doc, ex6_9);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["key"].to<std::string>() == "value");
			return true;
		}
		
		// 6.10
		TEST CommentLines()
		{
			PARSE(doc, ex6_10);
			YAML_ASSERT(doc.size() == 0);
			return true;
		}
		
		// 6.11
		TEST MultiLineComments()
		{
			PARSE(doc, ex6_11);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["key"].to<std::string>() == "value");
			return true;
		}

		struct StringMap {
			typedef std::map<std::string, std::string> Map;
			Map _;
		};
		
		bool operator == (const StringMap& m, const StringMap& n) {
			return m._ == n._;
		}
		
		void operator >> (const YAML::Node& node, StringMap& m) {
			m._.clear();
			for(YAML::Iterator it=node.begin();it!=node.end();++it) {
				std::string key = it.first().to<std::string>();
				std::string value = it.second().to<std::string>();
				m._[key] = value;
			}
		}

		
		// 6.12
		TEST SeparationSpacesII()
		{
			PARSE(doc, ex6_12);
			std::map<std::string, std::string> key;
			key["first"] = "Sammy";
			key["last"] = "Sosa";
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[key].size() == 2);
			YAML_ASSERT(doc[key]["hr"].to<int>() == 65);
			YAML_ASSERT(doc[key]["avg"].to<std::string>() == "0.278");
			return true;
		}
		
		// 6.13
		TEST ReservedDirectives()
		{
			PARSE(doc, ex6_13);
			return true;
		}
		
		// 6.14
		TEST YAMLDirective()
		{
			PARSE(doc, ex6_14);
			return true;
		}
		
		// 6.15
		TEST InvalidRepeatedYAMLDirective()
		{
			try {
				PARSE(doc, ex6_15);
			} catch(const YAML::ParserException& e) {
				if(e.msg == YAML::ErrorMsg::REPEATED_YAML_DIRECTIVE)
					return true;

				throw;
			}
			
			return "  No exception was thrown";
		}
		
		// 6.16
		TEST TagDirective()
		{
			PARSE(doc, ex6_16);
			YAML_ASSERT(doc.Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc.to<std::string>() == "foo");
			return true;
		}
		
		// 6.17
		TEST InvalidRepeatedTagDirective()
		{
			try {
				PARSE(doc, ex6_17);
			} catch(const YAML::ParserException& e) {
				if(e.msg == YAML::ErrorMsg::REPEATED_TAG_DIRECTIVE)
					return true;
				
				throw;
			}
	
			return "  No exception was thrown";
		}

		// 6.18
		TEST PrimaryTagHandle()
		{
			PARSE(doc, ex6_18);
			YAML_ASSERT(doc.Tag() == "!foo");
			YAML_ASSERT(doc.to<std::string>() == "bar");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.Tag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc.to<std::string>() == "bar");
			return true;
		}
		
		// 6.19
		TEST SecondaryTagHandle()
		{
			PARSE(doc, ex6_19);
			YAML_ASSERT(doc.Tag() == "tag:example.com,2000:app/int");
			YAML_ASSERT(doc.to<std::string>() == "1 - 3");
			return true;
		}
		
		// 6.20
		TEST TagHandles()
		{
			PARSE(doc, ex6_20);
			YAML_ASSERT(doc.Tag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc.to<std::string>() == "bar");
			return true;
		}
		
		// 6.21
		TEST LocalTagPrefix()
		{
			PARSE(doc, ex6_21);
			YAML_ASSERT(doc.Tag() == "!my-light");
			YAML_ASSERT(doc.to<std::string>() == "fluorescent");
			
			PARSE_NEXT(doc);
			YAML_ASSERT(doc.Tag() == "!my-light");
			YAML_ASSERT(doc.to<std::string>() == "green");
			return true;
		}
		
		// 6.22
		TEST GlobalTagPrefix()
		{
			PARSE(doc, ex6_22);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].Tag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc[0].to<std::string>() == "bar");
			return true;
		}
		
		// 6.23
		TEST NodeProperties()
		{
			PARSE(doc, ex6_23);
			YAML_ASSERT(doc.size() == 2);
			for(YAML::Iterator it=doc.begin();it!=doc.end();++it) {
				if(it.first().to<std::string>() == "foo") {
					YAML_ASSERT(it.first().Tag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it.second().Tag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it.second().to<std::string>() == "bar");
				} else if(it.first().to<std::string>() == "baz") {
					YAML_ASSERT(it.second().to<std::string>() == "foo");
				} else
					return "  unknown key";
			}
			
			return true;
		}
		
		// 6.24
		TEST VerbatimTags()
		{
			PARSE(doc, ex6_24);
			YAML_ASSERT(doc.size() == 1);
			for(YAML::Iterator it=doc.begin();it!=doc.end();++it) {
				YAML_ASSERT(it.first().Tag() == "tag:yaml.org,2002:str");
				YAML_ASSERT(it.first().to<std::string>() == "foo");
				YAML_ASSERT(it.second().Tag() == "!bar");
				YAML_ASSERT(it.second().to<std::string>() == "baz");
			}
			return true;
		}
		
		// 6.25
		TEST InvalidVerbatimTags()
		{
			PARSE(doc, ex6_25);
			return "  not implemented yet"; // TODO: check tags (but we probably will say these are valid, I think)
		}
		
		// 6.26
		TEST TagShorthands()
		{
			PARSE(doc, ex6_26);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].Tag() == "!local");
			YAML_ASSERT(doc[0].to<std::string>() == "foo");
			YAML_ASSERT(doc[1].Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc[1].to<std::string>() == "bar");
			YAML_ASSERT(doc[2].Tag() == "tag:example.com,2000:app/tag%21");
			YAML_ASSERT(doc[2].to<std::string>() == "baz");
			return true;
		}
		
		// 6.27
		TEST InvalidTagShorthands()
		{
			bool threw = false;
			try {
				PARSE(doc, ex6_27a);
			} catch(const YAML::ParserException& e) {
				threw = true;
				if(e.msg != YAML::ErrorMsg::TAG_WITH_NO_SUFFIX)
					throw;
			}
			
			if(!threw)
				return "  No exception was thrown for a tag with no suffix";

			PARSE(doc, ex6_27b); // TODO: should we reject this one (since !h! is not declared)?
			return "  not implemented yet";
		}
		
		// 6.28
		TEST NonSpecificTags()
		{
			PARSE(doc, ex6_28);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].to<std::string>() == "12"); // TODO: check tags. How?
			YAML_ASSERT(doc[1].to<int>() == 12);
			YAML_ASSERT(doc[2].to<std::string>() == "12");
			return true;
		}

		// 6.29
		TEST NodeAnchors()
		{
			PARSE(doc, ex6_29);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["First occurrence"].to<std::string>() == "Value");
			YAML_ASSERT(doc["Second occurrence"].to<std::string>() == "Value");
			return true;
		}
		
		// 7.1
		TEST AliasNodes()
		{
			PARSE(doc, ex7_1);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc["First occurrence"].to<std::string>() == "Foo");
			YAML_ASSERT(doc["Second occurrence"].to<std::string>() == "Foo");
			YAML_ASSERT(doc["Override anchor"].to<std::string>() == "Bar");
			YAML_ASSERT(doc["Reuse anchor"].to<std::string>() == "Bar");
			return true;
		}
		
		// 7.2
		TEST EmptyNodes()
		{
			PARSE(doc, ex7_2);
			YAML_ASSERT(doc.size() == 2);
			for(YAML::Iterator it=doc.begin();it!=doc.end();++it) {
				if(it.first().to<std::string>() == "foo") {
					YAML_ASSERT(it.second().Tag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it.second().to<std::string>() == "");
				} else if(it.first().to<std::string>() == "") {
					YAML_ASSERT(it.first().Tag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it.second().to<std::string>() == "bar");
				} else
					return "  unexpected key";
			}
			return true;
		}
		
		// 7.3
		TEST CompletelyEmptyNodes()
		{
			PARSE(doc, ex7_3);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(IsNull(doc["foo"]));
			YAML_ASSERT(doc[YAML::Null].to<std::string>() == "bar");
			return true;
		}
		
		// 7.4
		TEST DoubleQuotedImplicitKeys()
		{
			PARSE(doc, ex7_4);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].to<std::string>() == "value");
			return true;
		}
		
		// 7.5
		TEST DoubleQuotedLineBreaks()
		{
			PARSE(doc, ex7_5);
			YAML_ASSERT(doc.to<std::string>() == "folded to a space,\nto a line feed, or \t \tnon-content");
			return true;
		}
		
		// 7.6
		TEST DoubleQuotedLines()
		{
			PARSE(doc, ex7_6);
			YAML_ASSERT(doc.to<std::string>() == " 1st non-empty\n2nd non-empty 3rd non-empty ");
			return true;
		}
		
		// 7.7
		TEST SingleQuotedCharacters()
		{
			PARSE(doc, ex7_7);
			YAML_ASSERT(doc.to<std::string>() == "here's to \"quotes\"");
			return true;
		}
		
		// 7.8
		TEST SingleQuotedImplicitKeys()
		{
			PARSE(doc, ex7_8);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].to<std::string>() == "value");
			return true;
		}
		
		// 7.9
		TEST SingleQuotedLines()
		{
			PARSE(doc, ex7_9);
			YAML_ASSERT(doc.to<std::string>() == " 1st non-empty\n2nd non-empty 3rd non-empty ");
			return true;
		}
		
		// 7.10
		TEST PlainCharacters()
		{
			PARSE(doc, ex7_10);
			YAML_ASSERT(doc.size() == 6);
			YAML_ASSERT(doc[0].to<std::string>() == "::vector");
			YAML_ASSERT(doc[1].to<std::string>() == ": - ()");
			YAML_ASSERT(doc[2].to<std::string>() == "Up, up, and away!");
			YAML_ASSERT(doc[3].to<int>() == -123);
			YAML_ASSERT(doc[4].to<std::string>() == "http://example.com/foo#bar");
			YAML_ASSERT(doc[5].size() == 5);
			YAML_ASSERT(doc[5][0].to<std::string>() == "::vector");
			YAML_ASSERT(doc[5][1].to<std::string>() == ": - ()");
			YAML_ASSERT(doc[5][2].to<std::string>() == "Up, up, and away!");
			YAML_ASSERT(doc[5][3].to<int>() == -123);
			YAML_ASSERT(doc[5][4].to<std::string>() == "http://example.com/foo#bar");
			return true;
		}
		
		// 7.11
		TEST PlainImplicitKeys()
		{
			PARSE(doc, ex7_11);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].to<std::string>() == "value");
			return true;
		}
		
		// 7.12
		TEST PlainLines()
		{
			PARSE(doc, ex7_12);
			YAML_ASSERT(doc.to<std::string>() == "1st non-empty\n2nd non-empty 3rd non-empty");
			return true;
		}
		
		// 7.13
		TEST FlowSequence()
		{
			PARSE(doc, ex7_13);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0][0].to<std::string>() == "one");
			YAML_ASSERT(doc[0][1].to<std::string>() == "two");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1][0].to<std::string>() == "three");
			YAML_ASSERT(doc[1][1].to<std::string>() == "four");
			return true;
		}
		
		// 7.14
		TEST FlowSequenceEntries()
		{
			PARSE(doc, ex7_14);
			YAML_ASSERT(doc.size() == 5);
			YAML_ASSERT(doc[0].to<std::string>() == "double quoted");
			YAML_ASSERT(doc[1].to<std::string>() == "single quoted");
			YAML_ASSERT(doc[2].to<std::string>() == "plain text");
			YAML_ASSERT(doc[3].size() == 1);
			YAML_ASSERT(doc[3][0].to<std::string>() == "nested");
			YAML_ASSERT(doc[4].size() == 1);
			YAML_ASSERT(doc[4]["single"].to<std::string>() == "pair");
			return true;
		}
		
		// 7.15
		TEST FlowMappings()
		{
			PARSE(doc, ex7_15);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0]["one"].to<std::string>() == "two");
			YAML_ASSERT(doc[0]["three"].to<std::string>() == "four");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1]["five"].to<std::string>() == "six");
			YAML_ASSERT(doc[1]["seven"].to<std::string>() == "eight");
			return true;
		}
		
		// 7.16
		TEST FlowMappingEntries()
		{
			PARSE(doc, ex7_16);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["explicit"].to<std::string>() == "entry");
			YAML_ASSERT(doc["implicit"].to<std::string>() == "entry");
			YAML_ASSERT(IsNull(doc[YAML::Null]));
			return true;
		}
		
		// 7.17
		TEST FlowMappingSeparateValues()
		{
			PARSE(doc, ex7_17);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc["unquoted"].to<std::string>() == "separate");
			YAML_ASSERT(IsNull(doc["http://foo.com"]));
			YAML_ASSERT(IsNull(doc["omitted value"]));
			YAML_ASSERT(doc[YAML::Null].to<std::string>() == "omitted key");
			return true;
		}
		
		// 7.18
		TEST FlowMappingAdjacentValues()
		{
			PARSE(doc, ex7_18);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["adjacent"].to<std::string>() == "value");
			YAML_ASSERT(doc["readable"].to<std::string>() == "value");
			YAML_ASSERT(IsNull(doc["empty"]));
			return true;
		}
		
		// 7.19
		TEST SinglePairFlowMappings()
		{
			PARSE(doc, ex7_19);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo"].to<std::string>() == "bar");
			return true;
		}
		
		// 7.20
		TEST SinglePairExplicitEntry()
		{
			PARSE(doc, ex7_20);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo bar"].to<std::string>() == "baz");
			return true;
		}
		
		// 7.21
		TEST SinglePairImplicitEntries()
		{
			PARSE(doc, ex7_21);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0][0].size() == 1);
			YAML_ASSERT(doc[0][0]["YAML"].to<std::string>() == "separate");
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1][0].size() == 1);
			YAML_ASSERT(doc[1][0][YAML::Null].to<std::string>() == "empty key entry");
			YAML_ASSERT(doc[2].size() == 1);
			YAML_ASSERT(doc[2][0].size() == 1);
			StringMap key;
			key._["JSON"] = "like";
			YAML_ASSERT(doc[2][0][key].to<std::string>() == "adjacent");
			return true;
		}
		
		// 7.22
		TEST InvalidImplicitKeys()
		{
			try {
				PARSE(doc, ex7_22);
			} catch(const YAML::Exception& e) {
				if(e.msg == YAML::ErrorMsg::END_OF_SEQ_FLOW)
					return true;
				
				throw;
			}
			return "  no exception thrown";
		}
		
		// 7.23
		TEST FlowContent()
		{
			PARSE(doc, ex7_23);
			YAML_ASSERT(doc.size() == 5);
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0][0].to<std::string>() == "a");
			YAML_ASSERT(doc[0][1].to<std::string>() == "b");
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1]["a"].to<std::string>() == "b");
			YAML_ASSERT(doc[2].to<std::string>() == "a");
			YAML_ASSERT(doc[3].to<char>() == 'b');
			YAML_ASSERT(doc[4].to<std::string>() == "c");
			return true;
		}
		
		// 7.24
		TEST FlowNodes()
		{
			PARSE(doc, ex7_24);
			YAML_ASSERT(doc.size() == 5);
			YAML_ASSERT(doc[0].Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc[0].to<std::string>() == "a");
			YAML_ASSERT(doc[1].to<char>() == 'b');
			YAML_ASSERT(doc[2].to<std::string>() == "c");
			YAML_ASSERT(doc[3].to<std::string>() == "c");
			YAML_ASSERT(doc[4].Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc[4].to<std::string>() == "");
			return true;
		}
		
		// 8.1
		TEST BlockScalarHeader()
		{
			PARSE(doc, ex8_1);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc[0].to<std::string>() == "literal\n");
			YAML_ASSERT(doc[1].to<std::string>() == " folded\n");
			YAML_ASSERT(doc[2].to<std::string>() == "keep\n\n");
			YAML_ASSERT(doc[3].to<std::string>() == " strip");
			return true;
		}
		
		// 8.2
		TEST BlockIndentationHeader()
		{
			PARSE(doc, ex8_2);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc[0].to<std::string>() == "detected\n");
			YAML_ASSERT(doc[1].to<std::string>() == "\n\n# detected\n");
			YAML_ASSERT(doc[2].to<std::string>() == " explicit\n");
			YAML_ASSERT(doc[3].to<std::string>() == "\t\ndetected\n");
			return true;
		}
		
		// 8.3
		TEST InvalidBlockScalarIndentationIndicators()
		{
			{
				bool threw = false;
				try {
					PARSE(doc, ex8_3a);
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
					PARSE(doc, ex8_3b);
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
					PARSE(doc, ex8_3c);
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
		TEST ChompingFinalLineBreak()
		{
			PARSE(doc, ex8_4);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["strip"].to<std::string>() == "text");
			YAML_ASSERT(doc["clip"].to<std::string>() == "text\n");
			YAML_ASSERT(doc["keep"].to<std::string>() == "text\n");
			return true;
		}
		
		// 8.5
		TEST ChompingTrailingLines()
		{
			PARSE(doc, ex8_5);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["strip"].to<std::string>() == "# text");
			YAML_ASSERT(doc["clip"].to<std::string>() == "# text\n");
			YAML_ASSERT(doc["keep"].to<std::string>() == "# text\n"); // Note: I believe this is a bug in the YAML spec - it should be "# text\n\n"
			return true;
		}
		
		// 8.6
		TEST EmptyScalarChomping()
		{
			PARSE(doc, ex8_6);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["strip"].to<std::string>() == "");
			YAML_ASSERT(doc["clip"].to<std::string>() == "");
			YAML_ASSERT(doc["keep"].to<std::string>() == "\n");
			return true;
		}
		
		// 8.7
		TEST LiteralScalar()
		{
			PARSE(doc, ex8_7);
			YAML_ASSERT(doc.to<std::string>() == "literal\n\ttext\n");
			return true;
		}
		
		// 8.8
		TEST LiteralContent()
		{
			PARSE(doc, ex8_8);
			YAML_ASSERT(doc.to<std::string>() == "\n\nliteral\n \n\ntext\n");
			return true;
		}
		
		// 8.9
		TEST FoldedScalar()
		{
			PARSE(doc, ex8_9);
			YAML_ASSERT(doc.to<std::string>() == "folded text\n");
			return true;
		}
		
		// 8.10
		TEST FoldedLines()
		{
			PARSE(doc, ex8_10);
			YAML_ASSERT(doc.to<std::string>() == "\nfolded line\nnext line\n  * bullet\n\n  * list\n  * lines\n\nlast line\n");
			return true;
		}
		
		// 8.11
		TEST MoreIndentedLines()
		{
			return true; // same as 8.10
		}
		
		// 8.12
		TEST EmptySeparationLines()
		{
			return true; // same as 8.10
		}
		
		// 8.13
		TEST FinalEmptyLines()
		{
			return true; // same as 8.10
		}
		
		// 8.14
		TEST BlockSequence()
		{
			PARSE(doc, ex8_14);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["block sequence"].size() == 2);
			YAML_ASSERT(doc["block sequence"][0].to<std::string>() == "one");
			YAML_ASSERT(doc["block sequence"][1].size() == 1);
			YAML_ASSERT(doc["block sequence"][1]["two"].to<std::string>() == "three");
			return true;
		}
		
		// 8.15
		TEST BlockSequenceEntryTypes()
		{
			PARSE(doc, ex8_15);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(YAML::IsNull(doc[0]));
			YAML_ASSERT(doc[1].to<std::string>() == "block node\n");
			YAML_ASSERT(doc[2].size() == 2);
			YAML_ASSERT(doc[2][0].to<std::string>() == "one");
			YAML_ASSERT(doc[2][1].to<std::string>() == "two");
			YAML_ASSERT(doc[3].size() == 1);
			YAML_ASSERT(doc[3]["one"].to<std::string>() == "two");
			return true;
		}
		
		// 8.16
		TEST BlockMappings()
		{
			PARSE(doc, ex8_16);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["block mapping"].size() == 1);
			YAML_ASSERT(doc["block mapping"]["key"].to<std::string>() == "value");
			return true;
		}
		
		// 8.17
		TEST ExplicitBlockMappingEntries()
		{
			PARSE(doc, ex8_17);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(IsNull(doc["explicit key"]));
			YAML_ASSERT(doc["block key\n"].size() == 2);
			YAML_ASSERT(doc["block key\n"][0].to<std::string>() == "one");
			YAML_ASSERT(doc["block key\n"][1].to<std::string>() == "two");
			return true;
		}
		
		// 8.18
		TEST ImplicitBlockMappingEntries()
		{
			PARSE(doc, ex8_18);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["plain key"].to<std::string>() == "in-line value");
			YAML_ASSERT(IsNull(doc[YAML::Null]));
			YAML_ASSERT(doc["quoted key"].size() == 1);
			YAML_ASSERT(doc["quoted key"][0].to<std::string>() == "entry");
			return true;
		}
		
		// 8.19
		TEST CompactBlockMappings()
		{
			PARSE(doc, ex8_19);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["sun"].to<std::string>() == "yellow");
			YAML_ASSERT(doc[1].size() == 1);
			std::map<std::string, std::string> key;
			key["earth"] = "blue";
			YAML_ASSERT(doc[1][key].size() == 1);
			YAML_ASSERT(doc[1][key]["moon"].to<std::string>() == "white");
			return true;
		}
		
		// 8.20
		TEST BlockNodeTypes()
		{
			PARSE(doc, ex8_20);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].to<std::string>() == "flow in block");
			YAML_ASSERT(doc[1].to<std::string>() == "Block scalar\n");
			YAML_ASSERT(doc[2].size() == 1);
			YAML_ASSERT(doc[2]["foo"].to<std::string>() == "bar");
			return true;
		}
		
		// 8.21
		TEST BlockScalarNodes()
		{
			PARSE(doc, ex8_21);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["literal"].to<std::string>() == "value"); // Note: I believe this is a bug in the YAML spec - it should be "value\n"
			YAML_ASSERT(doc["folded"].to<std::string>() == "value");
			YAML_ASSERT(doc["folded"].Tag() == "!foo");
			return true;
		}
		
		// 8.22
		TEST BlockCollectionNodes()
		{
			PARSE(doc, ex8_22);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["sequence"].size() == 2);
			YAML_ASSERT(doc["sequence"][0].to<std::string>() == "entry");
			YAML_ASSERT(doc["sequence"][1].size() == 1);
			YAML_ASSERT(doc["sequence"][1][0].to<std::string>() == "nested");
			YAML_ASSERT(doc["mapping"].size() == 1);
			YAML_ASSERT(doc["mapping"]["foo"].to<std::string>() == "bar");
			return true;
		}
	}
}
