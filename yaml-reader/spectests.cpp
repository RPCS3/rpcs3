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

#define YAML_ASSERT(cond) do { if(!(cond)) return "Assert failed: " #cond; } while(false)

namespace Test {
	namespace {
		void RunSpecTest(TEST (*test)(), const std::string& index, const std::string& name, bool& passed) {
			TEST ret;
			try {
				ret = test();
			} catch(const YAML::Exception& e) {
				ret.ok = false;
				ret.error = e.msg;
			}
			
			if(ret.ok) {
				std::cout << "Spec test " << index << " passed: " << name << "\n";
			} else {
				passed = false;
				std::cout << "Spec test " << index << " failed: " << name << "\n";
				std::cout << ret.error << "\n";
			}
		}
	}

	namespace Spec {
		TEST SeqScalars() {
			std::string input =
				"- Mark McGwire\n"
				"- Sammy Sosa\n"
				"- Ken Griffey";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0] == "Mark McGwire");
			YAML_ASSERT(doc[1] == "Sammy Sosa");
			YAML_ASSERT(doc[2] == "Ken Griffey");
			return true;
		}
		
		TEST MappingScalarsToScalars() {
			std::string input =
				"hr:  65    # Home runs\n"
				"avg: 0.278 # Batting average\n"
				"rbi: 147   # Runs Batted In";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);

			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["hr"] == "65");
			YAML_ASSERT(doc["avg"] == "0.278");
			YAML_ASSERT(doc["rbi"] == "147");
			return true;
		}
		
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
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
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
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
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
		
		TEST SequenceOfSequences()
		{
			std::string input =
				"- [name        , hr, avg  ]\n"
				"- [Mark McGwire, 65, 0.278]\n"
				"- [Sammy Sosa  , 63, 0.288]";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
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
		
		TEST MappingOfMappings()
		{
			std::string input =
				"Mark McGwire: {hr: 65, avg: 0.278}\n"
				"Sammy Sosa: {\n"
				"    hr: 63,\n"
				"    avg: 0.288\n"
				"  }";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["Mark McGwire"].size() == 2);
			YAML_ASSERT(doc["Mark McGwire"]["hr"] == "65");
			YAML_ASSERT(doc["Mark McGwire"]["avg"] == "0.278");
			YAML_ASSERT(doc["Sammy Sosa"].size() == 2);
			YAML_ASSERT(doc["Sammy Sosa"]["hr"] == "63");
			YAML_ASSERT(doc["Sammy Sosa"]["avg"] == "0.288");
			return true;
		}
		
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
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc[0] == "Mark McGwire");
			YAML_ASSERT(doc[1] == "Sammy Sosa");
			YAML_ASSERT(doc[2] == "Ken Griffey");
			
			parser.GetNextDocument(doc);
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[0] == "Chicago Cubs");
			YAML_ASSERT(doc[1] == "St Louis Cardinals");
			return true;
		}
		
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
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["time"] == "20:03:20");
			YAML_ASSERT(doc["player"] == "Sammy Sosa");
			YAML_ASSERT(doc["action"] == "strike (miss)");
			
			parser.GetNextDocument(doc);
			YAML_ASSERT(doc.size() == 3);
			YAML_ASSERT(doc["time"] == "20:03:47");
			YAML_ASSERT(doc["player"] == "Sammy Sosa");
			YAML_ASSERT(doc["action"] == "grand slam");
			return true;
		}
		
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
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc["hr"].size() == 2);
			YAML_ASSERT(doc["hr"][0] == "Mark McGwire");
			YAML_ASSERT(doc["hr"][1] == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"].size() == 2);
			YAML_ASSERT(doc["rbi"][0] == "Sammy Sosa");
			YAML_ASSERT(doc["rbi"][1] == "Ken Griffey");
			return true;
		}
		
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
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
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
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);

			YAML_ASSERT(doc.size() == 2);
			YAML_ASSERT(doc[Pair("Detroit Tigers", "Chicago Cubs")].size() == 1);
			YAML_ASSERT(doc[Pair("Detroit Tigers", "Chicago Cubs")][0] == "2001-07-23");
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")].size() == 3);
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")][0] == "2001-07-02");
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")][1] == "2001-08-12");
			YAML_ASSERT(doc[Pair("New York Yankees", "Atlanta Braves")][2] == "2001-08-14");
			return true;
		}
	}

	bool RunSpecTests()
	{
		bool passed = true;
		RunSpecTest(&Spec::SeqScalars, "2.1", "Sequence of Scalars", passed);
		RunSpecTest(&Spec::MappingScalarsToScalars, "2.2", "Mapping Scalars to Scalars", passed);
		RunSpecTest(&Spec::MappingScalarsToSequences, "2.3", "Mapping Scalars to Sequences", passed);
		RunSpecTest(&Spec::SequenceOfMappings, "2.4", "Sequence of Mappings", passed);
		RunSpecTest(&Spec::SequenceOfSequences, "2.5", "Sequence of Sequences", passed);
		RunSpecTest(&Spec::MappingOfMappings, "2.6", "Mapping of Mappings", passed);
		RunSpecTest(&Spec::TwoDocumentsInAStream, "2.7", "Two Documents in a Stream", passed);
		RunSpecTest(&Spec::PlayByPlayFeed, "2.8", "Play by Play Feed from a Game", passed);
		RunSpecTest(&Spec::SingleDocumentWithTwoComments, "2.9", "Single Document with Two Comments", passed);
		RunSpecTest(&Spec::SimpleAnchor, "2.10", "Node for \"Sammy Sosa\" appears twice in this document", passed);
		RunSpecTest(&Spec::MappingBetweenSequences, "2.11", "Mapping between Sequences", passed);
		return passed;
	}
	
}

