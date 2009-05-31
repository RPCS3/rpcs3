#include "yaml.h"
#include "tests.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

namespace Test
{
	void RunAll()
	{
		bool passed = true;
		if(!RunParserTests())
			passed = false;
		
		if(!RunEmitterTests())
			passed = false;

		if(passed)
			std::cout << "All tests passed!\n";
	}
	
	////////////////////////////////////////////////////////////////////////////////////////
	// Parser tests

	namespace {
		void RunScalarParserTest(void (*test)(std::string&, std::string&), const std::string& name, bool& passed) {
			std::string error;
			std::string inputScalar, desiredOutput;
			std::string output;
			bool ok = true;
			try {
				test(inputScalar, desiredOutput);
				std::stringstream stream(inputScalar);
				YAML::Parser parser(stream);
				YAML::Node doc;
				parser.GetNextDocument(doc);
				doc >> output;
			} catch(const YAML::Exception& e) {
				ok = false;
				error = e.msg;
			}
			if(ok && output == desiredOutput) {
				std::cout << "Parser test passed: " << name << "\n";
			} else {
				passed = false;
				std::cout << "Parser test failed: " << name << "\n";
				if(error != "")
					std::cout << "Caught exception: " << error << "\n";
				else {
					std::cout << "Output:\n" << output << "<<<\n";
					std::cout << "Desired output:\n" << desiredOutput << "<<<\n";
				}
			}
		}

		void RunParserTest(bool (*test)(), const std::string& name, bool& passed) {
			std::string error;
			bool ok = true;
			try {
				ok = test();
			} catch(const YAML::Exception& e) {
				ok = false;
				error = e.msg;
			}
			if(ok) {
				std::cout << "Parser test passed: " << name << "\n";
			} else {
				passed = false;
				std::cout << "Parser test failed: " << name << "\n";
				if(error != "")
					std::cout << "Caught exception: " << error << "\n";
			}
		}
	}

	bool RunParserTests()
	{
		bool passed = true;
		RunScalarParserTest(&Parser::SimpleScalar, "simple scalar", passed);
		RunScalarParserTest(&Parser::MultiLineScalar, "multi-line scalar", passed);
		RunScalarParserTest(&Parser::LiteralScalar, "literal scalar", passed);
		RunScalarParserTest(&Parser::FoldedScalar, "folded scalar", passed);
		RunScalarParserTest(&Parser::ChompedFoldedScalar, "chomped folded scalar", passed);
		RunScalarParserTest(&Parser::ChompedLiteralScalar, "chomped literal scalar", passed);
		RunScalarParserTest(&Parser::FoldedScalarWithIndent, "folded scalar with indent", passed);
		RunScalarParserTest(&Parser::ColonScalar, "colon scalar", passed);
		RunScalarParserTest(&Parser::QuotedScalar, "quoted scalar", passed);
		RunScalarParserTest(&Parser::CommaScalar, "comma scalar", passed);
		RunScalarParserTest(&Parser::DashScalar, "dash scalar", passed);
		RunScalarParserTest(&Parser::URLScalar, "url scalar", passed);

		RunParserTest(&Parser::SimpleSeq, "simple seq", passed);
		RunParserTest(&Parser::SimpleMap, "simple map", passed);
		RunParserTest(&Parser::FlowSeq, "flow seq", passed);
		RunParserTest(&Parser::FlowMap, "flow map", passed);
		return passed;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// Emitter tests
	
	namespace {
		void RunEmitterTest(void (*test)(YAML::Emitter&, std::string&), const std::string& name, bool& passed) {
			YAML::Emitter out;
			std::string desiredOutput;
			test(out, desiredOutput);
			std::string output = out.c_str();

			if(output == desiredOutput) {
				std::cout << "Emitter test passed: " << name << "\n";
			} else {
				passed = false;
				std::cout << "Emitter test failed: " << name << "\n";
				std::cout << "Output:\n";
				std::cout << output << "<<<\n";
				std::cout << "Desired output:\n";
				std::cout << desiredOutput << "<<<\n";				
			}
		}
		
		void RunEmitterErrorTest(void (*test)(YAML::Emitter&, std::string&), const std::string& name, bool& passed) {
			YAML::Emitter out;
			std::string desiredError;
			test(out, desiredError);
			std::string lastError = out.GetLastError();
			if(!out.good() && lastError == desiredError) {
				std::cout << "Emitter test passed: " << name << "\n";
			} else {
				passed = false;
				std::cout << "Emitter test failed: " << name << "\n";
				if(out.good())
					std::cout << "No error detected\n";
				else
					std::cout << "Detected error: " << lastError << "\n";
				std::cout << "Expected error: " << desiredError << "\n";
			}
		}
	}
	
	bool RunEmitterTests()
	{
		bool passed = true;
		RunEmitterTest(&Emitter::SimpleScalar, "simple scalar", passed);
		RunEmitterTest(&Emitter::SimpleSeq, "simple seq", passed);
		RunEmitterTest(&Emitter::SimpleFlowSeq, "simple flow seq", passed);
		RunEmitterTest(&Emitter::EmptyFlowSeq, "empty flow seq", passed);
		RunEmitterTest(&Emitter::NestedBlockSeq, "nested block seq", passed);
		RunEmitterTest(&Emitter::NestedFlowSeq, "nested flow seq", passed);
		RunEmitterTest(&Emitter::SimpleMap, "simple map", passed);
		RunEmitterTest(&Emitter::SimpleFlowMap, "simple flow map", passed);
		RunEmitterTest(&Emitter::MapAndList, "map and list", passed);
		RunEmitterTest(&Emitter::ListAndMap, "list and map", passed);
		RunEmitterTest(&Emitter::NestedBlockMap, "nested block map", passed);
		RunEmitterTest(&Emitter::NestedFlowMap, "nested flow map", passed);
		RunEmitterTest(&Emitter::MapListMix, "map list mix", passed);
		RunEmitterTest(&Emitter::SimpleLongKey, "simple long key", passed);
		RunEmitterTest(&Emitter::SingleLongKey, "single long key", passed);
		RunEmitterTest(&Emitter::ComplexLongKey, "complex long key", passed);
		RunEmitterTest(&Emitter::AutoLongKey, "auto long key", passed);
		RunEmitterTest(&Emitter::ScalarFormat, "scalar format", passed);
		RunEmitterTest(&Emitter::AutoLongKeyScalar, "auto long key scalar", passed);
		RunEmitterTest(&Emitter::LongKeyFlowMap, "long key flow map", passed);
		RunEmitterTest(&Emitter::BlockMapAsKey, "block map as key", passed);
		RunEmitterTest(&Emitter::AliasAndAnchor, "alias and anchor", passed);
		RunEmitterTest(&Emitter::ComplexDoc, "complex doc", passed);
		RunEmitterTest(&Emitter::STLContainers, "STL containers", passed);
		RunEmitterTest(&Emitter::SimpleComment, "simple comment", passed);
		RunEmitterTest(&Emitter::MultiLineComment, "multi-line comment", passed);
		RunEmitterTest(&Emitter::ComplexComments, "complex comments", passed);
		RunEmitterTest(&Emitter::Indentation, "indentation", passed);
		RunEmitterTest(&Emitter::SimpleGlobalSettings, "simple global settings", passed);
		RunEmitterTest(&Emitter::ComplexGlobalSettings, "complex global settings", passed);
		
		RunEmitterErrorTest(&Emitter::ExtraEndSeq, "extra EndSeq", passed);
		RunEmitterErrorTest(&Emitter::ExtraEndMap, "extra EndMap", passed);
		RunEmitterErrorTest(&Emitter::BadSingleQuoted, "bad single quoted string", passed);
		RunEmitterErrorTest(&Emitter::InvalidAnchor, "invalid anchor", passed);
		RunEmitterErrorTest(&Emitter::InvalidAlias, "invalid alias", passed);
		RunEmitterErrorTest(&Emitter::MissingKey, "missing key", passed);
		RunEmitterErrorTest(&Emitter::MissingValue, "missing value", passed);
		RunEmitterErrorTest(&Emitter::UnexpectedKey, "unexpected key", passed);
		RunEmitterErrorTest(&Emitter::UnexpectedValue, "unexpected value", passed);
		return passed;
	}
	
}
