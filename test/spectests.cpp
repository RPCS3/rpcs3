#include "spectests.h"
#include "yaml.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

namespace {
	struct TEST {
		TEST(): ok(false) {}
		TEST(bool ok_): ok(ok_) {}
		TEST(const char *error_): ok(false), error(error_) {}
		
		bool ok;
		std::string error;
	};
}

#define YAML_ASSERT(cond) do { if(!(cond)) return "  Assert failed: " #cond; } while(false)
#define PARSE(doc, input) \
	std::stringstream stream(input);\
	YAML::Parser parser(stream);\
	YAML::Node doc;\
	parser.GetNextDocument(doc)
#define PARSE_NEXT(doc) parser.GetNextDocument(doc)

namespace Test {
	namespace {
		void RunSpecTest(TEST (*test)(), const std::string& index, const std::string& name, int& passed, int& total) {
			TEST ret;
			try {
				ret = test();
			} catch(const YAML::Exception& e) {
				ret.ok = false;
				ret.error = std::string("  Exception caught: ") + e.what();
			}
			
			if(!ret.ok) {
				std::cout << "Spec test " << index << " failed: " << name << "\n";
				std::cout << ret.error << "\n";
			}
			
			if(ret.ok)
				passed++;
			total++;
		}
	}

	namespace Spec {
		// 2.1
		TEST SeqScalars() {
			std::string input =
				"- Mark McGwire\n"
				"- Sammy Sosa\n"
				"- Ken Griffey";

			PARSE(doc, input);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0] == "Mark McGwire");
			YAML_ASSERT(doc[1] == "Sammy Sosa");
			YAML_ASSERT(doc[2] == "Ken Griffey");
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
			YAML_ASSERT(doc["hr"] == "65");
			YAML_ASSERT(doc["avg"] == "0.278");
			YAML_ASSERT(doc["rbi"] == "147");
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
			YAML_ASSERT(doc["american"][0] == "Boston Red Sox");
			YAML_ASSERT(doc["american"][1] == "Detroit Tigers");
			YAML_ASSERT(doc["american"][2] == "New York Yankees");
			YAML_ASSERT(doc["national"].size() == 3);
			YAML_ASSERT(doc["national"][0] == "New York Mets");
			YAML_ASSERT(doc["national"][1] == "Chicago Cubs");
			YAML_ASSERT(doc["national"][2] == "Atlanta Braves");
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
			YAML_ASSERT(doc[0]["name"] == "Mark McGwire");
			YAML_ASSERT(doc[0]["hr"] == "65");
			YAML_ASSERT(doc[0]["avg"] == "0.278");
			YAML_ASSERT(doc[1].size() == 3);
			YAML_ASSERT(doc[1]["name"] == "Sammy Sosa");
			YAML_ASSERT(doc[1]["hr"] == "63");
			YAML_ASSERT(doc[1]["avg"] == "0.288");
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
			YAML_ASSERT(doc[0][0] == "name");
			YAML_ASSERT(doc[0][1] == "hr");
			YAML_ASSERT(doc[0][2] == "avg");
			YAML_ASSERT(doc[1].size() == 3);
			YAML_ASSERT(doc[1][0] == "Mark McGwire");
			YAML_ASSERT(doc[1][1] == "65");
			YAML_ASSERT(doc[1][2] == "0.278");
			YAML_ASSERT(doc[2].size() == 3);
			YAML_ASSERT(doc[2][0] == "Sammy Sosa");
			YAML_ASSERT(doc[2][1] == "63");
			YAML_ASSERT(doc[2][2] == "0.288");
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
			YAML_ASSERT(doc["Mark McGwire"]["hr"] == "65");
			YAML_ASSERT(doc["Mark McGwire"]["avg"] == "0.278");
			YAML_ASSERT(doc["Sammy Sosa"].size() == 2);
			YAML_ASSERT(doc["Sammy Sosa"]["hr"] == "63");
			YAML_ASSERT(doc["Sammy Sosa"]["avg"] == "0.288");
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
			YAML_ASSERT(doc[0] == "Mark McGwire");
			YAML_ASSERT(doc[1] == "Sammy Sosa");
			YAML_ASSERT(doc[2] == "Ken Griffey");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0] == "Chicago Cubs");
			YAML_ASSERT(doc[1] == "St Louis Cardinals");
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
			YAML_ASSERT(doc["time"] == "20:03:20");
			YAML_ASSERT(doc["player"] == "Sammy Sosa");
			YAML_ASSERT(doc["action"] == "strike (miss)");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["time"] == "20:03:47");
			YAML_ASSERT(doc["player"] == "Sammy Sosa");
			YAML_ASSERT(doc["action"] == "grand slam");
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
			YAML_ASSERT(doc["hr"][0] == "Mark McGwire");
			YAML_ASSERT(doc["hr"][1] == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"].size() == 2);
			YAML_ASSERT(doc["rbi"][0] == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"][1] == "Ken Griffey");
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
			YAML_ASSERT(doc["hr"][0] == "Mark McGwire");
			YAML_ASSERT(doc["hr"][1] == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"].size() == 2);
			YAML_ASSERT(doc["rbi"][0] == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"][1] == "Ken Griffey");
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
			YAML_ASSERT(doc[Pair("Detroit Tigers", "Chicago cubs")][0] == "2001-07-23");
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")].size() == 3);
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")][0] == "2001-07-02");
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")][1] == "2001-08-12");
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")][2] == "2001-08-14");
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
			YAML_ASSERT(doc[0]["item"] == "Super Hoop");
			YAML_ASSERT(doc[0]["quantity"] == 1);
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1]["item"] == "Basketball");
			YAML_ASSERT(doc[1]["quantity"] == 4);
			YAML_ASSERT(doc[2].size() == 2);
			YAML_ASSERT(doc[2]["item"] == "Big Shoes");
			YAML_ASSERT(doc[2]["quantity"] == 1);
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
			YAML_ASSERT(doc ==
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
			YAML_ASSERT(doc == "Mark McGwire's year was crippled by a knee injury.");
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
			YAML_ASSERT(doc ==
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
			YAML_ASSERT(doc["name"] == "Mark McGwire");
			YAML_ASSERT(doc["accomplishment"] == "Mark set a major league home run record in 1998.\n");
			YAML_ASSERT(doc["stats"] == "65 Home Runs\n0.278 Batting Average\n");
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
			YAML_ASSERT(doc["unicode"] == "Sosa did fine.\xe2\x98\xba");
			YAML_ASSERT(doc["control"] == "\b1998\t1999\t2000\n");
			YAML_ASSERT(doc["hex esc"] == "\x0d\x0a is \r\n");
			YAML_ASSERT(doc["single"] == "\"Howdy!\" he cried.");
			YAML_ASSERT(doc["quoted"] == " # Not a 'comment'.");
			YAML_ASSERT(doc["tie-fighter"] == "|\\-*-/|");
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
			YAML_ASSERT(doc["plain"] == "This unquoted scalar spans many lines.");
			YAML_ASSERT(doc["quoted"] == "So does this quoted scalar.\n");
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
			YAML_ASSERT(doc["not-date"].GetTag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc["not-date"] == "2002-04-28");
			YAML_ASSERT(doc["picture"].GetTag() == "tag:yaml.org,2002:binary");
			YAML_ASSERT(doc["picture"] ==
				"R0lGODlhDAAMAIQAAP//9/X\n"
				"17unp5WZmZgAAAOfn515eXv\n"
				"Pz7Y6OjuDg4J+fn5OTk6enp\n"
				"56enmleECcgggoBADs=\n"
			);
			YAML_ASSERT(doc["application specific tag"].GetTag() == "!something");
			YAML_ASSERT(doc["application specific tag"] ==
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
			YAML_ASSERT(doc.GetTag() == "tag:clarkevans.com,2002:shape");
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].GetTag() == "tag:clarkevans.com,2002:circle");
			YAML_ASSERT(doc[0].size() == 2);
			YAML_ASSERT(doc[0]["center"].size() == 2);
			YAML_ASSERT(doc[0]["center"]["x"] == 73);
			YAML_ASSERT(doc[0]["center"]["y"] == 129);
			YAML_ASSERT(doc[0]["radius"] == 7);
			YAML_ASSERT(doc[1].GetTag() == "tag:clarkevans.com,2002:line");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1]["start"].size() == 2);
			YAML_ASSERT(doc[1]["start"]["x"] == 73);
			YAML_ASSERT(doc[1]["start"]["y"] == 129);
			YAML_ASSERT(doc[1]["finish"].size() == 2);
			YAML_ASSERT(doc[1]["finish"]["x"] == 89);
			YAML_ASSERT(doc[1]["finish"]["y"] == 102);
			YAML_ASSERT(doc[2].GetTag() == "tag:clarkevans.com,2002:label");
			YAML_ASSERT(doc[2].size() == 3);
			YAML_ASSERT(doc[2]["start"].size() == 2);
			YAML_ASSERT(doc[2]["start"]["x"] == 73);
			YAML_ASSERT(doc[2]["start"]["y"] == 129);
			YAML_ASSERT(doc[2]["color"] == "0xFFEEBB");
			YAML_ASSERT(doc[2]["text"] == "Pretty vector drawing.");
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
			YAML_ASSERT(doc.GetTag() == "tag:yaml.org,2002:set");
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
			YAML_ASSERT(doc.GetTag() == "tag:yaml.org,2002:omap");
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0].size() == 1);
			YAML_ASSERT(doc[0]["Mark McGwire"] == 65);
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1]["Sammy Sosa"] == 63);
			YAML_ASSERT(doc[2].size() == 1);
			YAML_ASSERT(doc[2]["Ken Griffey"] == 58);
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
			YAML_ASSERT(doc.GetTag() == "tag:clarkevans.com,2002:invoice");
			YAML_ASSERT(doc.size() == 8);
			YAML_ASSERT(doc["invoice"] == 34843);
			YAML_ASSERT(doc["date"] == "2001-01-23");
			YAML_ASSERT(doc["bill-to"].size() == 3);
			YAML_ASSERT(doc["bill-to"]["given"] == "Chris");
			YAML_ASSERT(doc["bill-to"]["family"] == "Dumars");
			YAML_ASSERT(doc["bill-to"]["address"].size() == 4);
			YAML_ASSERT(doc["bill-to"]["address"]["lines"] == "458 Walkman Dr.\nSuite #292\n");
			YAML_ASSERT(doc["bill-to"]["address"]["city"] == "Royal Oak");
			YAML_ASSERT(doc["bill-to"]["address"]["state"] == "MI");
			YAML_ASSERT(doc["bill-to"]["address"]["postal"] == "48046");
			YAML_ASSERT(doc["ship-to"].size() == 3);
			YAML_ASSERT(doc["ship-to"]["given"] == "Chris");
			YAML_ASSERT(doc["ship-to"]["family"] == "Dumars");
			YAML_ASSERT(doc["ship-to"]["address"].size() == 4);
			YAML_ASSERT(doc["ship-to"]["address"]["lines"] == "458 Walkman Dr.\nSuite #292\n");
			YAML_ASSERT(doc["ship-to"]["address"]["city"] == "Royal Oak");
			YAML_ASSERT(doc["ship-to"]["address"]["state"] == "MI");
			YAML_ASSERT(doc["ship-to"]["address"]["postal"] == "48046");
			YAML_ASSERT(doc["product"].size() == 2);
			YAML_ASSERT(doc["product"][0].size() == 4);
			YAML_ASSERT(doc["product"][0]["sku"] == "BL394D");
			YAML_ASSERT(doc["product"][0]["quantity"] == 4);
			YAML_ASSERT(doc["product"][0]["description"] == "Basketball");
			YAML_ASSERT(doc["product"][0]["price"] == "450.00");
			YAML_ASSERT(doc["product"][1].size() == 4);
			YAML_ASSERT(doc["product"][1]["sku"] == "BL4438H");
			YAML_ASSERT(doc["product"][1]["quantity"] == 1);
			YAML_ASSERT(doc["product"][1]["description"] == "Super Hoop");
			YAML_ASSERT(doc["product"][1]["price"] == "2392.00");
			YAML_ASSERT(doc["tax"] == "251.42");
			YAML_ASSERT(doc["total"] == "4443.52");
			YAML_ASSERT(doc["comments"] == "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.");
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
			YAML_ASSERT(doc["Time"] == "2001-11-23 15:01:42 -5");
			YAML_ASSERT(doc["User"] == "ed");
			YAML_ASSERT(doc["Warning"] == "This is an error message for the log file");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["Time"] == "2001-11-23 15:02:31 -5");
			YAML_ASSERT(doc["User"] == "ed");
			YAML_ASSERT(doc["Warning"] == "A slightly different error message.");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.size() == 4);
			YAML_ASSERT(doc["Date"] == "2001-11-23 15:03:17 -5");
			YAML_ASSERT(doc["User"] == "ed");
			YAML_ASSERT(doc["Fatal"] == "Unknown variable \"bar\"");
			YAML_ASSERT(doc["Stack"].size() == 2);
			YAML_ASSERT(doc["Stack"][0].size() == 3);
			YAML_ASSERT(doc["Stack"][0]["file"] == "TopClass.py");
			YAML_ASSERT(doc["Stack"][0]["line"] == "23");
			YAML_ASSERT(doc["Stack"][0]["code"] == "x = MoreObject(\"345\\n\")\n");
			YAML_ASSERT(doc["Stack"][1].size() == 3);
			YAML_ASSERT(doc["Stack"][1]["file"] == "MoreClass.py");
			YAML_ASSERT(doc["Stack"][1]["line"] == "58");
			YAML_ASSERT(doc["Stack"][1]["code"] == "foo = bar");
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
			YAML_ASSERT(doc["sequence"][0] == "one");
			YAML_ASSERT(doc["sequence"][1] == "two");
			YAML_ASSERT(doc["mapping"].size() == 2);
			YAML_ASSERT(doc["mapping"]["sky"] == "blue");
			YAML_ASSERT(doc["mapping"]["sea"] == "green");
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
			YAML_ASSERT(doc["sequence"][0] == "one");
			YAML_ASSERT(doc["sequence"][1] == "two");
			YAML_ASSERT(doc["mapping"].size() == 2);
			YAML_ASSERT(doc["mapping"]["sky"] == "blue");
			YAML_ASSERT(doc["mapping"]["sea"] == "green");
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
			YAML_ASSERT(doc["anchored"] == "value"); // TODO: assert tag
			YAML_ASSERT(doc["alias"] == "value");
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
			YAML_ASSERT(doc["literal"] == "some\ntext\n");
			YAML_ASSERT(doc["folded"] == "some text\n");
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
			YAML_ASSERT(doc["single"] == "text");
			YAML_ASSERT(doc["double"] == "text");
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
			YAML_ASSERT(doc == "Line break (no glyph)\nLine break (glyphed)\n");
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
			YAML_ASSERT(doc["quoted"] == "Quoted\t");
			YAML_ASSERT(doc["block"] ==
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
			YAML_ASSERT(doc == "Fun with \x5C \x22 \x07 \x08 \x1B \x0C \x0A \x0D \x09 \x0B " + std::string("\x00", 1) + " \x20 \xA0 \x85 \xe2\x80\xa8 \xe2\x80\xa9 A A A");
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
				YAML_ASSERT(e.msg == YAML::ErrorMsg::INVALID_ESCAPE + "c");
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
			YAML_ASSERT(doc["Not indented"]["By one space"] == "By four\n  spaces\n");
			YAML_ASSERT(doc["Not indented"]["Flow style"].size() == 3);
			YAML_ASSERT(doc["Not indented"]["Flow style"][0] == "By two");
			YAML_ASSERT(doc["Not indented"]["Flow style"][1] == "Also by two");
			YAML_ASSERT(doc["Not indented"]["Flow style"][2] == "Still by two");
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
			YAML_ASSERT(doc["a"][0] == "b");
			YAML_ASSERT(doc["a"][1].size() == 2);
			YAML_ASSERT(doc["a"][1][0] == "c");
			YAML_ASSERT(doc["a"][1][1] == "d");
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
			YAML_ASSERT(doc[0]["foo"] == "bar");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1][0] == "baz");
			YAML_ASSERT(doc[1][1] == "baz");
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
			YAML_ASSERT(doc["plain"] == "text lines");
			YAML_ASSERT(doc["quoted"] == "text lines");
			YAML_ASSERT(doc["block"] == "text\n \tlines\n");
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
			YAML_ASSERT(doc["Folding"] == "Empty line\nas a line feed");
			YAML_ASSERT(doc["Chomping"] == "Clipped empty lines\n");
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
			YAML_ASSERT(doc == "trimmed\n\n\nas space");
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
			YAML_ASSERT(doc == "foo \n\n\t bar\n\nbaz\n");
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
			YAML_ASSERT(doc == " foo\nbar\nbaz ");
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
			YAML_ASSERT(doc["key"] == "value");
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
			YAML_ASSERT(doc["key"] == "value");
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
				std::string key = it.first();
				std::string value = it.second();
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
			YAML_ASSERT(doc[key]["hr"] == 65);
			YAML_ASSERT(doc[key]["avg"] == "0.278");
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
			YAML_ASSERT(doc.GetTag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc == "foo");
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
			YAML_ASSERT(doc.GetTag() == "!foo");
			YAML_ASSERT(doc == "bar");

			PARSE_NEXT(doc);
			YAML_ASSERT(doc.GetTag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc == "bar");
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
			YAML_ASSERT(doc.GetTag() == "tag:example.com,2000:app/int");
			YAML_ASSERT(doc == "1 - 3");
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
			YAML_ASSERT(doc.GetTag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc == "bar");
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
			YAML_ASSERT(doc.GetTag() == "!my-light");
			YAML_ASSERT(doc == "fluorescent");
			
			PARSE_NEXT(doc);
			YAML_ASSERT(doc.GetTag() == "!my-light");
			YAML_ASSERT(doc == "green");
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
			YAML_ASSERT(doc[0].GetTag() == "tag:example.com,2000:app/foo");
			YAML_ASSERT(doc[0] == "bar");
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
				if(it.first() == "foo") {
					YAML_ASSERT(it.first().GetTag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it.second().GetTag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it.second() == "bar");
				} else if(it.first() == "baz") {
					YAML_ASSERT(it.second() == "foo");
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
				YAML_ASSERT(it.first().GetTag() == "tag:yaml.org,2002:str");
				YAML_ASSERT(it.first() == "foo");
				YAML_ASSERT(it.second().GetTag() == "!bar");
				YAML_ASSERT(it.second() == "baz");
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
			YAML_ASSERT(doc[0].GetTag() == "!local");
			YAML_ASSERT(doc[0] == "foo");
			YAML_ASSERT(doc[1].GetTag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc[1] == "bar");
			YAML_ASSERT(doc[2].GetTag() == "tag:example.com,2000:app/tag%21");
			YAML_ASSERT(doc[2] == "baz");
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
			YAML_ASSERT(doc[0] == "12"); // TODO: check tags. How?
			YAML_ASSERT(doc[1] == 12);
			YAML_ASSERT(doc[2] == "12");
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
			YAML_ASSERT(doc["First occurrence"] == "Value");
			YAML_ASSERT(doc["Second occurrence"] == "Value");
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
			YAML_ASSERT(doc["First occurrence"] == "Foo");
			YAML_ASSERT(doc["Second occurrence"] == "Foo");
			YAML_ASSERT(doc["Override anchor"] == "Bar");
			YAML_ASSERT(doc["Reuse anchor"] == "Bar");
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
				if(it.first() == "foo") {
					YAML_ASSERT(it.second().GetTag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it.second() == "");
				} else if(it.first() == "") {
					YAML_ASSERT(it.first().GetTag() == "tag:yaml.org,2002:str");
					YAML_ASSERT(it.second() == "bar");
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
			YAML_ASSERT(doc[YAML::Null] == "bar");
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
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"] == "value");
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
			YAML_ASSERT(doc == "folded to a space,\nto a line feed, or \t \tnon-content");
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
			YAML_ASSERT(doc == " 1st non-empty\n2nd non-empty 3rd non-empty ");
			return true;
		}
		
		// 7.7
		TEST SingleQuotedCharacters()
		{
			std::string input = " 'here''s to \"quotes\"'";

			PARSE(doc, input);
			YAML_ASSERT(doc == "here's to \"quotes\"");
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
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"] == "value");
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
			YAML_ASSERT(doc == " 1st non-empty\n2nd non-empty 3rd non-empty ");
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
			YAML_ASSERT(doc[0] == "::vector");
			YAML_ASSERT(doc[1] == ": - ()");
			YAML_ASSERT(doc[2] == "Up, up, and away!");
			YAML_ASSERT(doc[3] == -123);
			YAML_ASSERT(doc[4] == "http://example.com/foo#bar");
			YAML_ASSERT(doc[5].size() == 5);
			YAML_ASSERT(doc[5][0] == "::vector");
			YAML_ASSERT(doc[5][1] == ": - ()");
			YAML_ASSERT(doc[5][2] == "Up, up, and away!");
			YAML_ASSERT(doc[5][3] == -123);
			YAML_ASSERT(doc[5][4] == "http://example.com/foo#bar");
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
			YAML_ASSERT(doc["implicit block key"][0]["implicit flow key"] == "value");
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
			YAML_ASSERT(doc == "1st non-empty\n2nd non-empty 3rd non-empty");
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
			YAML_ASSERT(doc[0][0] == "one");
			YAML_ASSERT(doc[0][1] == "two");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1][0] == "three");
			YAML_ASSERT(doc[1][1] == "four");
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
			YAML_ASSERT(doc[0] == "double quoted");
			YAML_ASSERT(doc[1] == "single quoted");
			YAML_ASSERT(doc[2] == "plain text");
			YAML_ASSERT(doc[3].size() == 1);
			YAML_ASSERT(doc[3][0] == "nested");
			YAML_ASSERT(doc[4].size() == 1);
			YAML_ASSERT(doc[4]["single"] == "pair");
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
			YAML_ASSERT(doc[0]["one"] == "two");
			YAML_ASSERT(doc[0]["three"] == "four");
			YAML_ASSERT(doc[1].size() == 2);
			YAML_ASSERT(doc[1]["five"] == "six");
			YAML_ASSERT(doc[1]["seven"] == "eight");
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
			YAML_ASSERT(doc["explicit"] == "entry");
			YAML_ASSERT(doc["implicit"] == "entry");
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
			YAML_ASSERT(doc["unquoted"] == "separate");
			YAML_ASSERT(IsNull(doc["http://foo.com"]));
			YAML_ASSERT(IsNull(doc["omitted value"]));
			YAML_ASSERT(doc[YAML::Null] == "omitted key");
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
			YAML_ASSERT(doc["adjacent"] == "value");
			YAML_ASSERT(doc["readable"] == "value");
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
			YAML_ASSERT(doc[0]["foo"] == "bar");
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
			YAML_ASSERT(doc[0]["foo bar"] == "baz");
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
			YAML_ASSERT(doc[0][0]["YAML"] == "separate");
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1][0].size() == 1);
			YAML_ASSERT(doc[1][0][YAML::Null] == "empty key entry");
			YAML_ASSERT(doc[2].size() == 1);
			YAML_ASSERT(doc[2][0].size() == 1);
			StringMap key;
			key._["JSON"] = "like";
			YAML_ASSERT(doc[2][0][key] == "adjacent");
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
			YAML_ASSERT(doc[0][0] == "a");
			YAML_ASSERT(doc[0][1] == "b");
			YAML_ASSERT(doc[1].size() == 1);
			YAML_ASSERT(doc[1]["a"] == "b");
			YAML_ASSERT(doc[2] == "a");
			YAML_ASSERT(doc[3] == 'b');
			YAML_ASSERT(doc[4] == "c");
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
			YAML_ASSERT(doc[0].GetTag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc[0] == "a");
			YAML_ASSERT(doc[1] == 'b');
			YAML_ASSERT(doc[2] == "c");
			YAML_ASSERT(doc[3] == "c");
			YAML_ASSERT(doc[4].GetTag() == "tag:yaml.org,2002:str");
			YAML_ASSERT(doc[4] == "");
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
			YAML_ASSERT(doc[0] == "literal\n");
			YAML_ASSERT(doc[1] == " folded\n");
			YAML_ASSERT(doc[2] == "keep\n\n");
			YAML_ASSERT(doc[3] == " strip");
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
			YAML_ASSERT(doc[0] == "detected\n");
			YAML_ASSERT(doc[1] == "\n\n# detected\n");
			YAML_ASSERT(doc[2] == " explicit\n");
			YAML_ASSERT(doc[3] == "\t detected\n");
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
			YAML_ASSERT(doc["strip"] == "text");
			YAML_ASSERT(doc["clip"] == "text\n");
			YAML_ASSERT(doc["keep"] == "text\n");
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
			YAML_ASSERT(doc["strip"] == "# text");
			YAML_ASSERT(doc["clip"] == "# text\n");
			YAML_ASSERT(doc["keep"] == "# text\n");
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
			YAML_ASSERT(doc["strip"] == "");
			YAML_ASSERT(doc["clip"] == "");
			YAML_ASSERT(doc["keep"] == "\n");
			return true;
		}
	}

	bool RunSpecTests()
	{
		int passed = 0;
		int total = 0;
		RunSpecTest(&Spec::SeqScalars, "2.1", "Sequence of Scalars", passed, total);
		RunSpecTest(&Spec::MappingScalarsToScalars, "2.2", "Mapping Scalars to Scalars", passed, total);
		RunSpecTest(&Spec::MappingScalarsToSequences, "2.3", "Mapping Scalars to Sequences", passed, total);
		RunSpecTest(&Spec::SequenceOfMappings, "2.4", "Sequence of Mappings", passed, total);
		RunSpecTest(&Spec::SequenceOfSequences, "2.5", "Sequence of Sequences", passed, total);
		RunSpecTest(&Spec::MappingOfMappings, "2.6", "Mapping of Mappings", passed, total);
		RunSpecTest(&Spec::TwoDocumentsInAStream, "2.7", "Two Documents in a Stream", passed, total);
		RunSpecTest(&Spec::PlayByPlayFeed, "2.8", "Play by Play Feed from a Game", passed, total);
		RunSpecTest(&Spec::SingleDocumentWithTwoComments, "2.9", "Single Document with Two Comments", passed, total);
		RunSpecTest(&Spec::SimpleAnchor, "2.10", "Node for \"Sammy Sosa\" appears twice in this document", passed, total);
		RunSpecTest(&Spec::MappingBetweenSequences, "2.11", "Mapping between Sequences", passed, total);
		RunSpecTest(&Spec::CompactNestedMapping, "2.12", "Compact Nested Mapping", passed, total);
		RunSpecTest(&Spec::InLiteralsNewlinesArePreserved, "2.13", "In literals, newlines are preserved", passed, total);
		RunSpecTest(&Spec::InFoldedScalarsNewlinesBecomeSpaces, "2.14", "In folded scalars, newlines become spaces", passed, total);
		RunSpecTest(&Spec::FoldedNewlinesArePreservedForMoreIndentedAndBlankLines, "2.15", "Folded newlines are preserved for \"more indented\" and blank lines", passed, total);
		RunSpecTest(&Spec::IndentationDeterminesScope, "2.16", "Indentation determines scope", passed, total);
		RunSpecTest(&Spec::QuotedScalars, "2.17", "Quoted scalars", passed, total);
		RunSpecTest(&Spec::MultiLineFlowScalars, "2.18", "Multi-line flow scalars", passed, total);
		
		RunSpecTest(&Spec::VariousExplicitTags, "2.23", "Various Explicit Tags", passed, total);
		RunSpecTest(&Spec::GlobalTags, "2.24", "Global Tags", passed, total);
		RunSpecTest(&Spec::UnorderedSets, "2.25", "Unordered Sets", passed, total);
		RunSpecTest(&Spec::OrderedMappings, "2.26", "Ordered Mappings", passed, total);
		RunSpecTest(&Spec::Invoice, "2.27", "Invoice", passed, total);
		RunSpecTest(&Spec::LogFile, "2.28", "Log File", passed, total);
		
		RunSpecTest(&Spec::BlockStructureIndicators, "5.3", "Block Structure Indicators", passed, total);
		RunSpecTest(&Spec::FlowStructureIndicators, "5.4", "Flow Structure Indicators", passed, total);
		RunSpecTest(&Spec::NodePropertyIndicators, "5.6", "Node Property Indicators", passed, total);
		RunSpecTest(&Spec::BlockScalarIndicators, "5.7", "Block Scalar Indicators", passed, total);
		RunSpecTest(&Spec::QuotedScalarIndicators, "5.8", "Quoted Scalar Indicators", passed, total);
		RunSpecTest(&Spec::LineBreakCharacters, "5.11", "Line Break Characters", passed, total);
		RunSpecTest(&Spec::TabsAndSpaces, "5.12", "Tabs and Spaces", passed, total);
		RunSpecTest(&Spec::EscapedCharacters, "5.13", "Escaped Characters", passed, total);
		RunSpecTest(&Spec::InvalidEscapedCharacters, "5.14", "Invalid Escaped Characters", passed, total);
		
		RunSpecTest(&Spec::IndentationSpaces, "6.1", "Indentation Spaces", passed, total);
		RunSpecTest(&Spec::IndentationIndicators, "6.2", "Indentation Indicators", passed, total);
		RunSpecTest(&Spec::SeparationSpaces, "6.3", "Separation Spaces", passed, total);
		RunSpecTest(&Spec::LinePrefixes, "6.4", "Line Prefixes", passed, total);
		RunSpecTest(&Spec::EmptyLines, "6.5", "Empty Lines", passed, total);
		RunSpecTest(&Spec::LineFolding, "6.6", "Line Folding", passed, total);
		RunSpecTest(&Spec::BlockFolding, "6.7", "Block Folding", passed, total);
		RunSpecTest(&Spec::FlowFolding, "6.8", "Flow Folding", passed, total);
		RunSpecTest(&Spec::SeparatedComment, "6.9", "Separated Comment", passed, total);
		RunSpecTest(&Spec::CommentLines, "6.10", "Comment Lines", passed, total);
		RunSpecTest(&Spec::MultiLineComments, "6.11", "Multi-Line Comments", passed, total);
		RunSpecTest(&Spec::SeparationSpacesII, "6.12", "Separation Spaces", passed, total);
		RunSpecTest(&Spec::ReservedDirectives, "6.13", "Reserved Directives", passed, total);
		RunSpecTest(&Spec::YAMLDirective, "6.14", "YAML Directive", passed, total);
		RunSpecTest(&Spec::InvalidRepeatedYAMLDirective, "6.15", "Invalid Repeated YAML Directive", passed, total);
		RunSpecTest(&Spec::TagDirective, "6.16", "Tag Directive", passed, total);
		RunSpecTest(&Spec::InvalidRepeatedTagDirective, "6.17", "Invalid Repeated Tag Directive", passed, total);
		RunSpecTest(&Spec::PrimaryTagHandle, "6.18", "Primary Tag Handle", passed, total);
		RunSpecTest(&Spec::SecondaryTagHandle, "6.19", "SecondaryTagHandle", passed, total);
		RunSpecTest(&Spec::TagHandles, "6.20", "TagHandles", passed, total);
		RunSpecTest(&Spec::LocalTagPrefix, "6.21", "LocalTagPrefix", passed, total);
		RunSpecTest(&Spec::GlobalTagPrefix, "6.22", "GlobalTagPrefix", passed, total);
		RunSpecTest(&Spec::NodeProperties, "6.23", "NodeProperties", passed, total);
		RunSpecTest(&Spec::VerbatimTags, "6.24", "Verbatim Tags", passed, total);
		RunSpecTest(&Spec::InvalidVerbatimTags, "6.25", "Invalid Verbatim Tags", passed, total);
		RunSpecTest(&Spec::TagShorthands, "6.26", "Tag Shorthands", passed, total);
		RunSpecTest(&Spec::InvalidTagShorthands, "6.27", "Invalid Tag Shorthands", passed, total);
		RunSpecTest(&Spec::NonSpecificTags, "6.28", "Non Specific Tags", passed, total);
		RunSpecTest(&Spec::NodeAnchors, "6.29", "Node Anchors", passed, total);
		
		RunSpecTest(&Spec::AliasNodes, "7.1", "Alias Nodes", passed, total);
		RunSpecTest(&Spec::EmptyNodes, "7.2", "Empty Nodes", passed, total);
		RunSpecTest(&Spec::CompletelyEmptyNodes, "7.3", "Completely Empty Nodes", passed, total);
		RunSpecTest(&Spec::DoubleQuotedImplicitKeys, "7.4", "Double Quoted Implicit Keys", passed, total);
		RunSpecTest(&Spec::DoubleQuotedLineBreaks, "7.5", "Double Quoted Line Breaks", passed, total);
		RunSpecTest(&Spec::DoubleQuotedLines, "7.6", "Double Quoted Lines", passed, total);
		RunSpecTest(&Spec::SingleQuotedCharacters, "7.7", "Single Quoted Characters", passed, total);
		RunSpecTest(&Spec::SingleQuotedImplicitKeys, "7.8", "Single Quoted Implicit Keys", passed, total);
		RunSpecTest(&Spec::SingleQuotedLines, "7.9", "Single Quoted Lines", passed, total);
		RunSpecTest(&Spec::PlainCharacters, "7.10", "Plain Characters", passed, total);
		RunSpecTest(&Spec::PlainImplicitKeys, "7.11", "Plain Implicit Keys", passed, total);
		RunSpecTest(&Spec::PlainLines, "7.12", "Plain Lines", passed, total);
		RunSpecTest(&Spec::FlowSequence, "7.13", "Flow Sequence", passed, total);
		RunSpecTest(&Spec::FlowSequenceEntries, "7.14", "Flow Sequence Entries", passed, total);
		RunSpecTest(&Spec::FlowMappings, "7.15", "Flow Mappings", passed, total);
		RunSpecTest(&Spec::FlowMappingEntries, "7.16", "Flow Mapping Entries", passed, total);
		RunSpecTest(&Spec::FlowMappingSeparateValues, "7.17", "Flow Mapping Separate Values", passed, total);
		RunSpecTest(&Spec::FlowMappingAdjacentValues, "7.18", "Flow Mapping Adjacent Values", passed, total);
		RunSpecTest(&Spec::SinglePairFlowMappings, "7.19", "Single Pair Flow Mappings", passed, total);
		RunSpecTest(&Spec::SinglePairExplicitEntry, "7.20", "Single Pair Explicit Entry", passed, total);
		RunSpecTest(&Spec::SinglePairImplicitEntries, "7.21", "Single Pair Implicit Entries", passed, total);
		RunSpecTest(&Spec::InvalidImplicitKeys, "7.22", "Invalid Implicit Keys", passed, total);
		RunSpecTest(&Spec::FlowContent, "7.23", "Flow Content", passed, total);
		RunSpecTest(&Spec::FlowNodes, "7.24", "FlowNodes", passed, total);
		
		RunSpecTest(&Spec::BlockScalarHeader, "8.1", "Block Scalar Header", passed, total);
		RunSpecTest(&Spec::BlockIndentationHeader, "8.2", "Block Indentation Header", passed, total);
		RunSpecTest(&Spec::InvalidBlockScalarIndentationIndicators, "8.3", "Invalid Block Scalar Indentation Indicators", passed, total);
		RunSpecTest(&Spec::ChompingFinalLineBreak, "8.4", "Chomping Final Line Break", passed, total);
		RunSpecTest(&Spec::ChompingTrailingLines, "8.4", "Chomping Trailing Lines", passed, total);
		RunSpecTest(&Spec::EmptyScalarChomping, "8.4", "Empty Scalar Chomping", passed, total);

		std::cout << "Spec tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
	
}

