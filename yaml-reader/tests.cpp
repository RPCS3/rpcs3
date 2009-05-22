#include "yaml.h"
#include "tests.h"
#include "parser.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

namespace Test
{
	// runs all the tests on all data we have
	void RunAll(bool verbose)
	{
		std::vector <std::string> files;
		files.push_back("tests/simple.yaml");
		files.push_back("tests/mixed.yaml");
		files.push_back("tests/scalars.yaml");
		files.push_back("tests/directives.yaml");
		files.push_back("tests/aliased.yaml");

		bool passed = true;
		for(unsigned i=0;i<files.size();i++) {
			if(!Inout(files[i], verbose)) {
				std::cout << "Inout test failed on " << files[i] << "\n";
				passed = false;
			} else
				std::cout << "Inout test passed: " << files[i] << "\n";
		}

		if(passed)
			std::cout << "All tests passed!\n";
	}

	// loads the given YAML file, outputs it, and then loads the outputted file,
	// outputs again, and makes sure that the two outputs are the same
	bool Inout(const std::string& file, bool verbose)
	{
		std::ifstream fin(file.c_str());

		try {
			// read and output
			YAML::Parser parser(fin);
			if(!parser)
				return false;

			YAML::Node doc;
			parser.GetNextDocument(doc);

			std::stringstream out;
			out << doc;
			// and save
			std::string firstTry = out.str();

			// and now again
			parser.Load(out);
			if(!parser)
				return false;

			parser.GetNextDocument(doc);
			std::stringstream out2;
			out2 << doc;
			// and save
			std::string secondTry = out2.str();

			// now compare
			if(firstTry == secondTry)
				return true;

			std::ofstream fout("tests/out.yaml");
			fout << "---\n";
			fout << firstTry << std::endl;
			fout << "---\n";
			fout << secondTry << std::endl;
		} catch(YAML::ParserException& e) {
			std::cout << file << " (line " << e.line + 1 << ", col " << e.column + 1 << "): " << e.msg << std::endl;
			
			if(verbose) {
				std::cout << "Token queue:\n";
				std::ifstream f(file.c_str());
				YAML::Parser p(f);
				p.PrintTokens(std::cout);
			}
			
			return false;
		}

		return true;
	}
}
