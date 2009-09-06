#include "spectests.h"
#include "yaml.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#define YAML_ASSERT(cond) do { if(!(cond)) return false; } while(false)

namespace Test {
	namespace {
		void RunSpecTest(bool (*test)(), const std::string& index, const std::string& name, bool& passed) {
			std::string error;
			bool ok = true;
			try {
				ok = test();
			} catch(const YAML::Exception& e) {
				ok = false;
				error = e.msg;
			}
			if(ok) {
				std::cout << "Spec test " << index << " passed: " << name << "\n";
			} else {
				passed = false;
				std::cout << "Spec test " << index << " failed: " << name << "\n";
				if(error != "")
					std::cout << "Caught exception: " << error << "\n";
			}
		}
	}

	namespace Spec {
		bool SeqScalars() {
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
		
		bool MappingScalarsToScalars() {
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
	}

	bool RunSpecTests()
	{
		bool passed = true;
		RunSpecTest(&Spec::SeqScalars, "2.1", "Sequence of Scalars", passed);
		RunSpecTest(&Spec::MappingScalarsToScalars, "2.2", "Mapping Scalars to Scalars", passed);
		return passed;
	}
	
}

