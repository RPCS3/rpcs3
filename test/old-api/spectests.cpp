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
			std::string input =
				"- Mark McGwire\n"
				"- Sammy Sosa\n"
				"- Ken Griffey";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].to<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc[1].to<std::string>() == "Sammy Sosa");
			YAML_ASSERT(doc[2].to<std::string>() == "Ken Griffey");
			return true;
		}
		
		// 2.2
		TEST MappingScalarsToScalars() {
			std::string input =
				"hr:  65    # Home runs\n"
				"avg: 0.278 # Batting average\n"
				"rbi: 147   # Runs Batted In";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["hr"].to<std::string>() == "65");
			YAML_ASSERT(doc["avg"].to<std::string>() == "0.278");
			YAML_ASSERT(doc["rbi"].to<std::string>() == "147");
			return true;
		}
		
		// 2.3
		TEST MappingScalarsToSequences() {
			std::string input =
				"american:\n"
				"- Boston Red Sox\n"
				"- Detroit Tigers\n"
				"- New York Yankees\n"
				"national:\n"
				"- New York Mets\n"
				"- Chicago Cubs\n"
				"- Atlanta Braves";

			PARSE(doc, input);
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
			std::string input =
				"-\n"
				"  name: Mark McGwire\n"
				"  hr:   65\n"
				"  avg:  0.278\n"
				"-\n"
				"  name: Sammy Sosa\n"
				"  hr:   63\n"
				"  avg:  0.288";

			PARSE(doc, input);
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
			std::string input =
				"- [name        , hr, avg  ]\n"
				"- [Mark McGwire, 65, 0.278]\n"
				"- [Sammy Sosa  , 63, 0.288]";

			PARSE(doc, input);
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
			std::string input =
				"Mark McGwire: {hr: 65, avg: 0.278}\n"
				"Sammy Sosa: {\n"
				"    hr: 63,\n"
				"    avg: 0.288\n"
				"  }";

			PARSE(doc, input);
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
			std::string input =
				"# Ranking of 1998 home runs\n"
				"---\n"
				"- Mark McGwire\n"
				"- Sammy Sosa\n"
				"- Ken Griffey\n"
				"\n"
				"# Team ranking\n"
				"---\n"
				"- Chicago Cubs\n"
				"- St Louis Cardinals";

			PARSE(doc, input);
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
			std::string input =
				"---\n"
				"time: 20:03:20\n"
				"player: Sammy Sosa\n"
				"action: strike (miss)\n"
				"...\n"
				"---\n"
				"time: 20:03:47\n"
				"player: Sammy Sosa\n"
				"action: grand slam\n"
				"...";

			PARSE(doc, input);
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
			std::string input =
				"---\n"
				"hr: # 1998 hr ranking\n"
				"  - Mark McGwire\n"
				"  - Sammy Sosa\n"
				"rbi:\n"
				"  # 1998 rbi ranking\n"
				"  - Sammy Sosa\n"
				"  - Ken Griffey";

			PARSE(doc, input);
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
			std::string input =
				"---\n"
				"hr:\n"
				"  - Mark McGwire\n"
				"  # Following node labeled SS\n"
				"  - &SS Sammy Sosa\n"
				"rbi:\n"
				"  - *SS # Subsequent occurrence\n"
				"  - Ken Griffey";

			PARSE(doc, input);
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
			std::string input =
				"? - Detroit Tigers\n"
				"  - Chicago cubs\n"
				":\n"
				"  - 2001-07-23\n"
				"\n"
				"? [ New York Yankees,\n"
				"    Atlanta Braves ]\n"
				": [ 2001-07-02, 2001-08-12,\n"
				"    2001-08-14 ]";

			PARSE(doc, input);
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
			std::string input =
				"---\n"
				"# Products purchased\n"
				"- item    : Super Hoop\n"
				"  quantity: 1\n"
				"- item    : Basketball\n"
				"  quantity: 4\n"
				"- item    : Big Shoes\n"
				"  quantity: 1";

			PARSE(doc, input);
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
			std::string input =
				"# ASCII Art\n"
				"--- |\n"
				"  \\//||\\/||\n"
				"  // ||  ||__";

			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() ==
						"\\//||\\/||\n"
						"// ||  ||__");
			return true;
		}
		
		// 2.14
		TEST InFoldedScalarsNewlinesBecomeSpaces()
		{
			std::string input =
				"--- >\n"
				"  Mark McGwire's\n"
				"  year was crippled\n"
				"  by a knee injury.";

			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "Mark McGwire's year was crippled by a knee injury.");
			return true;
		}
		
		// 2.15
		TEST FoldedNewlinesArePreservedForMoreIndentedAndBlankLines()
		{
			std::string input =
				">\n"
				" Sammy Sosa completed another\n"
				" fine season with great stats.\n"
				" \n"
				"   63 Home Runs\n"
				"   0.288 Batting Average\n"
				" \n"
				" What a year!";

			PARSE(doc, input);
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
			std::string input =
				"name: Mark McGwire\n"
				"accomplishment: >\n"
				"  Mark set a major league\n"
				"  home run record in 1998.\n"
				"stats: |\n"
				"  65 Home Runs\n"
				"  0.278 Batting Average\n";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["name"].to<std::string>() == "Mark McGwire");
			YAML_ASSERT(doc["accomplishment"].to<std::string>() == "Mark set a major league home run record in 1998.\n");
			YAML_ASSERT(doc["stats"].to<std::string>() == "65 Home Runs\n0.278 Batting Average\n");
			return true;
		}
		
		// 2.17
		TEST QuotedScalars()
		{
			std::string input =
				"unicode: \"Sosa did fine.\\u263A\"\n"
				"control: \"\\b1998\\t1999\\t2000\\n\"\n"
				"hex esc: \"\\x0d\\x0a is \\r\\n\"\n"
				"\n"
				"single: '\"Howdy!\" he cried.'\n"
				"quoted: ' # Not a ''comment''.'\n"
				"tie-fighter: '|\\-*-/|'";

			PARSE(doc, input);
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
			std::string input =
				"plain:\n"
				"  This unquoted scalar\n"
				"  spans many lines.\n"
				"\n"
				"quoted: \"So does this\n"
				"  quoted scalar.\\n\"";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["plain"].to<std::string>() == "This unquoted scalar spans many lines.");
			YAML_ASSERT(doc["quoted"].to<std::string>() == "So does this quoted scalar.\n");
			return true;
		}
		
		// TODO: 2.19 - 2.22 schema tags
		
		// 2.23
		TEST VariousExplicitTags()
		{
			std::string input =
				"---\n"
				"not-date: !!str 2002-04-28\n"
				"\n"
				"picture: !!binary |\n"
				" R0lGODlhDAAMAIQAAP//9/X\n"
				" 17unp5WZmZgAAAOfn515eXv\n"
				" Pz7Y6OjuDg4J+fn5OTk6enp\n"
				" 56enmleECcgggoBADs=\n"
				"\n"
				"application specific tag: !something |\n"
				" The semantics of the tag\n"
				" above may be different for\n"
				" different documents.";
			
			PARSE(doc, input);
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
			std::string input =
				"%TAG ! tag:clarkevans.com,2002:\n"
				"--- !shape\n"
				"  # Use the ! handle for presenting\n"
				"  # tag:clarkevans.com,2002:circle\n"
				"- !circle\n"
				"  center: &ORIGIN {x: 73, y: 129}\n"
				"  radius: 7\n"
				"- !line\n"
				"  start: *ORIGIN\n"
				"  finish: { x: 89, y: 102 }\n"
				"- !label\n"
				"  start: *ORIGIN\n"
				"  color: 0xFFEEBB\n"
				"  text: Pretty vector drawing.";
			
			PARSE(doc, input);
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
			std::string input =
				"# Sets are represented as a\n"
				"# Mapping where each key is\n"
				"# associated with a null value\n"
				"--- !!set\n"
				"? Mark McGwire\n"
				"? Sammy Sosa\n"
				"? Ken Griffey";
			
			PARSE(doc, input);
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
			std::string input =
				"# Ordered maps are represented as\n"
				"# A sequence of mappings, with\n"
				"# each mapping having one key\n"
				"--- !!omap\n"
				"- Mark McGwire: 65\n"
				"- Sammy Sosa: 63\n"
				"- Ken Griffey: 58";
			
			PARSE(doc, input);
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
			std::string input =
				"--- !<tag:clarkevans.com,2002:invoice>\n"
				"invoice: 34843\n"
				"date   : 2001-01-23\n"
				"bill-to: &id001\n"
				"    given  : Chris\n"
				"    family : Dumars\n"
				"    address:\n"
				"        lines: |\n"
				"            458 Walkman Dr.\n"
				"            Suite #292\n"
				"        city    : Royal Oak\n"
				"        state   : MI\n"
				"        postal  : 48046\n"
				"ship-to: *id001\n"
				"product:\n"
				"    - sku         : BL394D\n"
				"      quantity    : 4\n"
				"      description : Basketball\n"
				"      price       : 450.00\n"
				"    - sku         : BL4438H\n"
				"      quantity    : 1\n"
				"      description : Super Hoop\n"
				"      price       : 2392.00\n"
				"tax  : 251.42\n"
				"total: 4443.52\n"
				"comments:\n"
				"    Late afternoon is best.\n"
				"    Backup contact is Nancy\n"
				"    Billsmer @ 338-4338.";

			PARSE(doc, input);
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
			std::string input =
				"---\n"
				"Time: 2001-11-23 15:01:42 -5\n"
				"User: ed\n"
				"Warning:\n"
				"  This is an error message\n"
				"  for the log file\n"
				"---\n"
				"Time: 2001-11-23 15:02:31 -5\n"
				"User: ed\n"
				"Warning:\n"
				"  A slightly different error\n"
				"  message.\n"
				"---\n"
				"Date: 2001-11-23 15:03:17 -5\n"
				"User: ed\n"
				"Fatal:\n"
				"  Unknown variable \"bar\"\n"
				"Stack:\n"
				"  - file: TopClass.py\n"
				"    line: 23\n"
				"    code: |\n"
				"      x = MoreObject(\"345\\n\")\n"
				"  - file: MoreClass.py\n"
				"    line: 58\n"
				"    code: |-\n"
				"      foo = bar";

			PARSE(doc, input);
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
			std::string input =
				"sequence:\n"
				"- one\n"
				"- two\n"
				"mapping:\n"
				"  ? sky\n"
				"  : blue\n"
				"  sea : green";

			PARSE(doc, input);
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
			std::string input =
				"sequence: [ one, two, ]\n"
				"mapping: { sky: blue, sea: green }";

			PARSE(doc, input);
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
			std::string input =
				"# Comment only.";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 0);
			return true;
		}
		
		// 5.6
		TEST NodePropertyIndicators()
		{
			std::string input =
				"anchored: !local &anchor value\n"
				"alias: *anchor";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["anchored"].to<std::string>() == "value"); // TODO: assert tag
			YAML_ASSERT(doc["alias"].to<std::string>() == "value");
			return true;
		}
		
		// 5.7
		TEST BlockScalarIndicators()
		{
			std::string input =
				"literal: |\n"
				"  some\n"
				"  text\n"
				"folded: >\n"
				"  some\n"
				"  text\n";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["literal"].to<std::string>() == "some\ntext\n");
			YAML_ASSERT(doc["folded"].to<std::string>() == "some text\n");
			return true;
		}
		
		// 5.8
		TEST QuotedScalarIndicators()
		{
			std::string input =
				"single: 'text'\n"
				"double: \"text\"";

			PARSE(doc, input);
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
			std::string input =
				"|\n"
				"  Line break (no glyph)\n"
				"  Line break (glyphed)\n";

			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "Line break (no glyph)\nLine break (glyphed)\n");
			return true;
		}
		
		// 5.12
		TEST TabsAndSpaces()
		{
			std::string input =
				"# Tabs and spaces\n"
				"quoted: \"Quoted\t\"\n"
				"block:	|\n"
				"  void main() {\n"
				"  \tprintf(\"Hello, world!\\n\");\n"
				"  }";

			PARSE(doc, input);
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
			std::string input =
				"\"Fun with \\\\\n"
				"\\\" \\a \\b \\e \\f \\\n"
				"\\n \\r \\t \\v \\0 \\\n"
				"\\  \\_ \\N \\L \\P \\\n"
				"\\x41 \\u0041 \\U00000041\"";

			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "Fun with \x5C \x22 \x07 \x08 \x1B \x0C \x0A \x0D \x09 \x0B " + std::string("\x00", 1) + " \x20 \xA0 \x85 \xe2\x80\xa8 \xe2\x80\xa9 A A A");
			return true;
		}
		
		// 5.14
		TEST InvalidEscapedCharacters()
		{
			std::string input =
				"Bad escapes:\n"
				"  \"\\c\n"
				"  \\xq-\"";
			
			std::stringstream stream(input);
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
			std::string input =
				"  # Leading comment line spaces are\n"
				"   # neither content nor indentation.\n"
				"    \n"
				"Not indented:\n"
				" By one space: |\n"
				"    By four\n"
				"      spaces\n"
				" Flow style: [    # Leading spaces\n"
				"   By two,        # in flow style\n"
				"  Also by two,    # are neither\n"
				"  \tStill by two   # content nor\n"
				"    ]             # indentation.";

			PARSE(doc, input);
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
			std::string input =
				"? a\n"
				": -\tb\n"
				"  -  -\tc\n"
				"     - d";

			PARSE(doc, input);
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
			std::string input =
				"- foo:\t bar\n"
				"- - baz\n"
				"  -\tbaz";

			PARSE(doc, input);
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
			std::string input =
				"plain: text\n"
				"  lines\n"
				"quoted: \"text\n"
				"  \tlines\"\n"
				"block: |\n"
				"  text\n"
				"   \tlines\n";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["plain"].to<std::string>() == "text lines");
			YAML_ASSERT(doc["quoted"].to<std::string>() == "text lines");
			YAML_ASSERT(doc["block"].to<std::string>() == "text\n \tlines\n");
			return true;
		}
		
		// 6.5
		TEST EmptyLines()
		{
			std::string input =
				"Folding:\n"
				"  \"Empty line\n"
				"   \t\n"
				"  as a line feed\"\n"
				"Chomping: |\n"
				"  Clipped empty lines\n"
				" ";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["Folding"].to<std::string>() == "Empty line\nas a line feed");
			YAML_ASSERT(doc["Chomping"].to<std::string>() == "Clipped empty lines\n");
			return true;
		}
		
		// 6.6
		TEST LineFolding()
		{
			std::string input =
				">-\n"
				"  trimmed\n"
				"  \n"
				" \n"
				"\n"
				"  as\n"
				"  space";

			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "trimmed\n\n\nas space");
			return true;
		}
		
		// 6.7
		TEST BlockFolding()
		{
			std::string input =
				">\n"
				"  foo \n"
				" \n"
				"  \t bar\n"
				"\n"
				"  baz\n";

			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "foo \n\n\t bar\n\nbaz\n");
			return true;
		}
		
		// 6.8
		TEST FlowFolding()
		{
			std::string input =
				"\"\n"
				"  foo \n"
				" \n"
				"  \t bar\n"
				"\n"
				"  baz\n"
				"\"";

			PARSE(doc, input);			
			YAML_ASSERT(doc.to<std::string>() == " foo\nbar\nbaz ");
			return true;
		}
		
		// 6.9
		TEST SeparatedComment()
		{
			std::string input =
				"key:    # Comment\n"
				"  value";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["key"].to<std::string>() == "value");
			return true;
		}
		
		// 6.10
		TEST CommentLines()
		{
			std::string input =
				"  # Comment\n"
				"   \n"
				"\n";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 0);
			return true;
		}
		
		// 6.11
		TEST MultiLineComments()
		{
			std::string input =
				"key:    # Comment\n"
				"        # lines\n"
				"  value\n"
				"\n";

			PARSE(doc, input);
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
			std::string input =
				"{ first: Sammy, last: Sosa }:\n"
				"# Statistics:\n"
				"  hr:  # Home runs\n"
				"     65\n"
				"  avg: # Average\n"
				"   0.278";

			PARSE(doc, input);
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
			std::string input =
				"%FOO  bar baz # Should be ignored\n"
				"               # with a warning.\n"
				"--- \"foo\"";

			PARSE(doc, input);
			return true;
		}
		
		// 6.14
		TEST YAMLDirective()
		{
			std::string input =
				"%YAML 1.3 # Attempt parsing\n"
				"           # with a warning\n"
				"---\n"
				"\"foo\"";

			PARSE(doc, input);
			return true;
		}
		
		// 6.15
		TEST InvalidRepeatedYAMLDirective()
		{
			std::string input =
				"%YAML 1.2\n"
				"%YAML 1.1\n"
				"foo";

			try {
				PARSE(doc, input);
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
			std::string input =
				"%TAG !yaml! tag:yaml.org,2002:\n"
				"---\n"
				"!yaml!str \"foo\"";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.Tag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc.to<std::string>() == "foo");
			return true;
		}
		
		// 6.17
		TEST InvalidRepeatedTagDirective()
		{
			std::string input =
				"%TAG ! !foo\n"
				"%TAG ! !foo\n"
				"bar";

			try {
				PARSE(doc, input);
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
			std::string input =
				"# Private\n"
				"!foo \"bar\"\n"
				"...\n"
				"# Global\n"
				"%TAG ! tag:example.com,2000:app/\n"
				"---\n"
				"!foo \"bar\"";

			PARSE(doc, input);
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
			std::string input =
				"%TAG !! tag:example.com,2000:app/\n"
				"---\n"
				"!!int 1 - 3 # Interval, not integer";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.Tag() == "tag:example.com,2000:app/int");
			YAML_ASSERT(doc.to<std::string>() == "1 - 3");
			return true;
		}
		
		// 6.20
		TEST TagHandles()
		{
			std::string input =
				"%TAG !e! tag:example.com,2000:app/\n"
				"---\n"
				"!e!foo \"bar\"";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.Tag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc.to<std::string>() == "bar");
			return true;
		}
		
		// 6.21
		TEST LocalTagPrefix()
		{
			std::string input =
				"%TAG !m! !my-\n"
				"--- # Bulb here\n"
				"!m!light fluorescent\n"
				"...\n"
				"%TAG !m! !my-\n"
				"--- # Color here\n"
				"!m!light green";
			
			PARSE(doc, input);
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
			std::string input =
				"%TAG !e! tag:example.com,2000:app/\n"
				"---\n"
				"- !e!foo \"bar\"";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].Tag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc[0].to<std::string>() == "bar");
			return true;
		}
		
		// 6.23
		TEST NodeProperties()
		{
			std::string input =
				"!!str &a1 \"foo\":\n"
				"  !!str bar\n"
				"&a2 baz : *a1";

			PARSE(doc, input);
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
			std::string input =
				"!<tag:yaml.org,2002:str> foo :\n"
				"  !<!bar> baz";
			
			PARSE(doc, input);
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
			std::string input =
				"- !<!> foo\n"
				"- !<$:?> bar\n";

			PARSE(doc, input);
			return "  not implemented yet"; // TODO: check tags (but we probably will say these are valid, I think)
		}
		
		// 6.26
		TEST TagShorthands()
		{
			std::string input =
				"%TAG !e! tag:example.com,2000:app/\n"
				"---\n"
				"- !local foo\n"
				"- !!str bar\n"
				"- !e!tag%21 baz\n";

			PARSE(doc, input);
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
			std::string input1 =
				"%TAG !e! tag:example,2000:app/\n"
				"---\n"
				"- !e! foo";

			bool threw = false;
			try {
				PARSE(doc, input1);
			} catch(const YAML::ParserException& e) {
				threw = true;
				if(e.msg != YAML::ErrorMsg::TAG_WITH_NO_SUFFIX)
					throw;
			}
			
			if(!threw)
				return "  No exception was thrown for a tag with no suffix";

			std::string input2 =
				"%TAG !e! tag:example,2000:app/\n"
				"---\n"
				"- !h!bar baz";

			PARSE(doc, input2); // TODO: should we reject this one (since !h! is not declared)?
			return "  not implemented yet";
		}
		
		// 6.28
		TEST NonSpecificTags()
		{
			std::string input =
				"# Assuming conventional resolution:\n"
				"- \"12\"\n"
				"- 12\n"
				"- ! 12";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].to<std::string>() == "12"); // TODO: check tags. How?
			YAML_ASSERT(doc[1].to<int>() == 12);
			YAML_ASSERT(doc[2].to<std::string>() == "12");
			return true;
		}

		// 6.29
		TEST NodeAnchors()
		{
			std::string input =
				"First occurrence: &anchor Value\n"
				"Second occurrence: *anchor";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["First occurrence"].to<std::string>() == "Value");
			YAML_ASSERT(doc["Second occurrence"].to<std::string>() == "Value");
			return true;
		}
		
		// 7.1
		TEST AliasNodes()
		{
			std::string input =
				"First occurrence: &anchor Foo\n"
				"Second occurrence: *anchor\n"
				"Override anchor: &anchor Bar\n"
				"Reuse anchor: *anchor";

			PARSE(doc, input);
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
			std::string input =
				"{\n"
				"  foo : !!str,\n"
				"  !!str : bar,\n"
				"}";

			PARSE(doc, input);
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
			std::string input =
				"{\n"
				"  ? foo :,\n"
				"  : bar,\n"
				"}\n";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(IsNull(doc["foo"]));
			YAML_ASSERT(doc[YAML::Null].to<std::string>() == "bar");
			return true;
		}
		
		// 7.4
		TEST DoubleQuotedImplicitKeys()
		{
			std::string input =
				"\"implicit block key\" : [\n"
				"  \"implicit flow key\" : value,\n"
				" ]";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].to<std::string>() == "value");
			return true;
		}
		
		// 7.5
		TEST DoubleQuotedLineBreaks()
		{
			std::string input =
				"\"folded \n"
				"to a space,\t\n"
				" \n"
				"to a line feed, or \t\\\n"
				" \\ \tnon-content\"";

			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "folded to a space,\nto a line feed, or \t \tnon-content");
			return true;
		}
		
		// 7.6
		TEST DoubleQuotedLines()
		{
			std::string input =
				"\" 1st non-empty\n"
				"\n"
				" 2nd non-empty \n"
				"\t3rd non-empty \"";

			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == " 1st non-empty\n2nd non-empty 3rd non-empty ");
			return true;
		}
		
		// 7.7
		TEST SingleQuotedCharacters()
		{
			std::string input = " 'here''s to \"quotes\"'";

			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "here's to \"quotes\"");
			return true;
		}
		
		// 7.8
		TEST SingleQuotedImplicitKeys()
		{
			std::string input =
				"'implicit block key' : [\n"
				"  'implicit flow key' : value,\n"
				" ]";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].to<std::string>() == "value");
			return true;
		}
		
		// 7.9
		TEST SingleQuotedLines()
		{
			std::string input =
				"' 1st non-empty\n"
				"\n"
				" 2nd non-empty \n"
				"\t3rd non-empty '";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == " 1st non-empty\n2nd non-empty 3rd non-empty ");
			return true;
		}
		
		// 7.10
		TEST PlainCharacters()
		{
			std::string input =
				"# Outside flow collection:\n"
				"- ::vector\n"
				"- \": - ()\"\n"
				"- Up, up, and away!\n"
				"- -123\n"
				"- http://example.com/foo#bar\n"
				"# Inside flow collection:\n"
				"- [ ::vector,\n"
				"  \": - ()\",\n"
				"  \"Up, up, and away!\",\n"
				"  -123,\n"
				"  http://example.com/foo#bar ]";
			
			PARSE(doc, input);
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
			std::string input =
				"implicit block key : [\n"
				"  implicit flow key : value,\n"
				" ]";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["implicit block key"].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0].size() == 1);
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"].to<std::string>() == "value");
			return true;
		}
		
		// 7.12
		TEST PlainLines()
		{
			std::string input =
				"1st non-empty\n"
				"\n"
				" 2nd non-empty \n"
				"\t3rd non-empty";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "1st non-empty\n2nd non-empty 3rd non-empty");
			return true;
		}
		
		// 7.13
		TEST FlowSequence()
		{
			std::string input =
				"- [ one, two, ]\n"
				"- [three ,four]";
			
			PARSE(doc, input);
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
			std::string input =
				"[\n"
				"\"double\n"
				" quoted\", 'single\n"
				"           quoted',\n"
				"plain\n"
				" text, [ nested ],\n"
				"single: pair,\n"
				"]";
			
			PARSE(doc, input);
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
			std::string input =
				"- { one : two , three: four , }\n"
				"- {five: six,seven : eight}";
			
			PARSE(doc, input);
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
			std::string input =
				"{\n"
				"? explicit: entry,\n"
				"implicit: entry,\n"
				"?\n"
				"}";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["explicit"].to<std::string>() == "entry");
			YAML_ASSERT(doc["implicit"].to<std::string>() == "entry");
			YAML_ASSERT(IsNull(doc[YAML::Null]));
			return true;
		}
		
		// 7.17
		TEST FlowMappingSeparateValues()
		{
			std::string input =
				"{\n"
				"unquoted : \"separate\",\n"
				"http://foo.com,\n"
				"omitted value:,\n"
				": omitted key,\n"
				"}";
			
			PARSE(doc, input);
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
			std::string input =
				"{\n"
				"\"adjacent\":value,\n"
				"\"readable\":value,\n"
				"\"empty\":\n"
				"}";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["adjacent"].to<std::string>() == "value");
			YAML_ASSERT(doc["readable"].to<std::string>() == "value");
			YAML_ASSERT(IsNull(doc["empty"]));
			return true;
		}
		
		// 7.19
		TEST SinglePairFlowMappings()
		{
			std::string input =
				"[\n"
				"foo: bar\n"
				"]";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo"].to<std::string>() == "bar");
			return true;
		}
		
		// 7.20
		TEST SinglePairExplicitEntry()
		{
			std::string input =
				"[\n"
				"? foo\n"
				" bar : baz\n"
				"]";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["foo bar"].to<std::string>() == "baz");
			return true;
		}
		
		// 7.21
		TEST SinglePairImplicitEntries()
		{
			std::string input =
				"- [ YAML : separate ]\n"
				"- [ : empty key entry ]\n"
				"- [ {JSON: like}:adjacent ]";
			
			PARSE(doc, input);
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
			std::string input =
				"[ foo\n"
				" bar: invalid,"; // Note: we don't check (on purpose) the >1K chars for an implicit key
			
			try {
				PARSE(doc, input);
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
			std::string input =
				"- [ a, b ]\n"
				"- { a: b }\n"
				"- \"a\"\n"
				"- 'b'\n"
				"- c";
			
			PARSE(doc, input);
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
			std::string input =
				"- !!str \"a\"\n"
				"- 'b'\n"
				"- &anchor \"c\"\n"
				"- *anchor\n"
				"- !!str";
			
			PARSE(doc, input);
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
			std::string input =
				"- | # Empty header\n"
				" literal\n"
				"- >1 # Indentation indicator\n"
				"  folded\n"
				"- |+ # Chomping indicator\n"
				" keep\n"
				"\n"
				"- >1- # Both indicators\n"
				"  strip\n";
			
			PARSE(doc, input);
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
			std::string input =
				"- |\n"
				" detected\n"
				"- >\n"
				" \n"
				"  \n"
				"  # detected\n"
				"- |1\n"
				"  explicit\n"
				"- >\n"
				" \t\n"
				" detected\n";
			
			PARSE(doc, input);
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
				std::string input =
					"- |\n"
					"  \n"
					" text";
			
				bool threw = false;
				try {
					PARSE(doc, input);
				} catch(const YAML::Exception& e) {
					if(e.msg != YAML::ErrorMsg::END_OF_SEQ)
						throw;
					
					threw = true;
				}
				
				if(!threw)
					return "  no exception thrown for less indented auto-detecting indentation for a literal block scalar";
			}
			
			{
				std::string input =
					"- >\n"
					"  text\n"
					" text";
			
				bool threw = false;
				try {
					PARSE(doc, input);
				} catch(const YAML::Exception& e) {
					if(e.msg != YAML::ErrorMsg::END_OF_SEQ)
						throw;
					
					threw = true;
				}
				
				if(!threw)
					return "  no exception thrown for less indented auto-detecting indentation for a folded block scalar";
			}
			
			{
				std::string input =
					"- |2\n"
					" text";
			
				bool threw = false;
				try {
					PARSE(doc, input);
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
			std::string input =
				"strip: |-\n"
				"  text\n"
				"clip: |\n"
				"  text\n"
				"keep: |+\n"
				"  text\n";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["strip"].to<std::string>() == "text");
			YAML_ASSERT(doc["clip"].to<std::string>() == "text\n");
			YAML_ASSERT(doc["keep"].to<std::string>() == "text\n");
			return true;
		}
		
		// 8.5
		TEST ChompingTrailingLines()
		{
			std::string input =
				" # Strip\n"
				"  # Comments:\n"
				"strip: |-\n"
				"  # text\n"
				"  \n"
				" # Clip\n"
				"  # comments:\n"
				"\n"
				"clip: |\n"
				"  # text\n"
				" \n"
				" # Keep\n"
				"  # comments:\n"
				"\n"
				"keep: |+\n"
				"  # text\n"
				"\n"
				" # Trail\n"
				"  # Comments\n";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["strip"].to<std::string>() == "# text");
			YAML_ASSERT(doc["clip"].to<std::string>() == "# text\n");
			YAML_ASSERT(doc["keep"].to<std::string>() == "# text\n"); // Note: I believe this is a bug in the YAML spec - it should be "# text\n\n"
			return true;
		}
		
		// 8.6
		TEST EmptyScalarChomping()
		{
			std::string input =
				"strip: >-\n"
				"\n"
				"clip: >\n"
				"\n"
				"keep: |+\n"
				"\n";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["strip"].to<std::string>() == "");
			YAML_ASSERT(doc["clip"].to<std::string>() == "");
			YAML_ASSERT(doc["keep"].to<std::string>() == "\n");
			return true;
		}
		
		// 8.7
		TEST LiteralScalar()
		{
			std::string input =
				"|\n"
				" literal\n"
				" \ttext\n"
				"\n";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "literal\n\ttext\n");
			return true;
		}
		
		// 8.8
		TEST LiteralContent()
		{
			std::string input =
				"|\n"
				" \n"
				"  \n"
				"  literal\n"
				"   \n"
				"  \n"
				"  text\n"
				"\n"
				" # Comment\n";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "\n\nliteral\n \n\ntext\n");
			return true;
		}
		
		// 8.9
		TEST FoldedScalar()
		{
			std::string input =
				">\n"
				" folded\n"
				" text\n"
				"\n";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.to<std::string>() == "folded text\n");
			return true;
		}
		
		// 8.10
		TEST FoldedLines()
		{
			std::string input =
				">\n"
				"\n"
				" folded\n"
				" line\n"
				"\n"
				" next\n"
				" line\n"
				"   * bullet\n"
				"\n"
				"   * list\n"
				"   * lines\n"
				"\n"
				" last\n"
				" line\n"
				"\n"
				"# Comment\n";
			
			PARSE(doc, input);
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
			std::string input =
				"block sequence:\n"
				"  - one\n"
				"  - two : three\n";
			
			PARSE(doc, input);
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
			std::string input =
				"- # Empty\n"
				"- |\n"
				" block node\n"
				"- - one # Compact\n"
				"  - two # sequence\n"
				"- one: two # Compact mapping\n";
			
			PARSE(doc, input);
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
			std::string input =
				"block mapping:\n"
				" key: value\n";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 1);
			YAML_ASSERT(doc["block mapping"].size() == 1);
			YAML_ASSERT(doc["block mapping"]["key"].to<std::string>() == "value");
			return true;
		}
		
		// 8.17
		TEST ExplicitBlockMappingEntries()
		{
			std::string input =
				"? explicit key # Empty value\n"
				"? |\n"
				"  block key\n"
				": - one # Explicit compact\n"
				"  - two # block value\n";
			
			PARSE(doc, input);
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
			std::string input =
				"plain key: in-line value\n"
				":  # Both empty\n"
				"\"quoted key\":\n"
				"- entry\n";
			
			PARSE(doc, input);
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
			std::string input =
				"- sun: yellow\n"
				"- ? earth: blue\n"
				"  : moon: white\n";
			
			PARSE(doc, input);
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
			std::string input =
				"-\n"
				"  \"flow in block\"\n"
				"- >\n"
				" Block scalar\n"
				"- !!map # Block collection\n"
				"  foo : bar\n";
			
			PARSE(doc, input);
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
			std::string input =
				"literal: |2\n"
				"  value\n"
				"folded:\n"
				"   !foo\n"
				"  >1\n"
				" value\n";
			
			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["literal"].to<std::string>() == "value"); // Note: I believe this is a bug in the YAML spec - it should be "value\n"
			YAML_ASSERT(doc["folded"].to<std::string>() == "value");
			YAML_ASSERT(doc["folded"].Tag() == "!foo");
			return true;
		}
		
		// 8.22
		TEST BlockCollectionNodes()
		{
			std::string input =
				"sequence: !!seq\n"
				"- entry\n"
				"- !!seq\n"
				" - nested\n"
				"mapping: !!map\n"
				" foo: bar\n";
			
			PARSE(doc, input);
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
