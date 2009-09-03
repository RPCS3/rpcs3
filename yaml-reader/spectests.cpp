#include "spectests.h"
#include "yaml.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

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
			
			if(doc.size() != 3)
				return false;
			std::string output;
			doc[0] >> output;
			if(output != "Mark McGwire")
				return false;
			doc[1] >> output;
			if(output != "Sammy Sosa")
				return false;
			doc[2] >> output;
			if(output != "Ken Griffey")
				return false;
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

			std::string output;
			doc["hr"] >> output;
			if(output != "65")
				return false;
			doc["avg"] >> output;
			if(output != "0.278")
				return false;
			doc["rbi"] >> output;
			if(output != "147")
				return false;
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

