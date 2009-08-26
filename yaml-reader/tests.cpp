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

		typedef void (*EncodingFn)(std::ostream&, int);

		inline char Byte(int ch)
		{
			return static_cast<char>(static_cast<unsigned char>(static_cast<unsigned int>(ch)));
		}

		void EncodeToUtf8(std::ostream& stream, int ch)
		{
			if (ch <= 0x7F)
			{
				stream << Byte(ch);
			}
			else if (ch <= 0x7FF)
			{
				stream << Byte(0xC0 | (ch >> 6));
				stream << Byte(0x80 | (ch & 0x3F));
			}
			else if (ch <= 0xFFFF)
			{
				stream << Byte(0xE0 | (ch >> 12));
				stream << Byte(0x80 | ((ch >> 6) & 0x3F));
				stream << Byte(0x80 | (ch & 0x3F));
			}
			else if (ch <= 0x1FFFFF)
			{
				stream << Byte(0xF0 | (ch >> 18));
				stream << Byte(0x80 | ((ch >> 12) & 0x3F));
				stream << Byte(0x80 | ((ch >> 6) & 0x3F));
				stream << Byte(0x80 | (ch & 0x3F));
			}
		}

		bool SplitUtf16HighChar(std::ostream& stream, EncodingFn encoding, int ch)
		{
			int biasedValue = ch - 0x10000;
			if (biasedValue < 0)
			{
				return false;
			}
			int high = 0xD800 | (biasedValue >> 10);
			int low  = 0xDC00 | (biasedValue & 0x3FF);
			encoding(stream, high);
			encoding(stream, low);
			return true;
		}

		void EncodeToUtf16LE(std::ostream& stream, int ch)
		{
			if (!SplitUtf16HighChar(stream, &EncodeToUtf16LE, ch))
			{
				stream << Byte(ch & 0xFF) << Byte(ch >> 8);
			}
		}

		void EncodeToUtf16BE(std::ostream& stream, int ch)
		{
			if (!SplitUtf16HighChar(stream, &EncodeToUtf16BE, ch))
			{
				stream << Byte(ch >> 8) << Byte(ch & 0xFF);
			}
		}

		void EncodeToUtf32LE(std::ostream& stream, int ch)
		{
			stream << Byte(ch & 0xFF) << Byte((ch >> 8) & 0xFF) 
				<< Byte((ch >> 16) & 0xFF) << Byte((ch >> 24) & 0xFF);
		}

		void EncodeToUtf32BE(std::ostream& stream, int ch)
		{
			stream << Byte((ch >> 24) & 0xFF) << Byte((ch >> 16) & 0xFF)
				<< Byte((ch >> 8) & 0xFF) << Byte(ch & 0xFF);
		}

		class EncodingTester
		{
		public:
			EncodingTester(EncodingFn encoding, bool declareEncoding)
			{
				if (declareEncoding)
				{
					encoding(m_yaml, 0xFEFF);
				}

				AddEntry(encoding, 0x0021, 0x007E); // Basic Latin
				AddEntry(encoding, 0x00A1, 0x00FF); // Latin-1 Supplement
				AddEntry(encoding, 0x0660, 0x06FF); // Arabic (largest contiguous block)

				// CJK unified ideographs (multiple lines)
				AddEntry(encoding, 0x4E00, 0x4EFF);
				AddEntry(encoding, 0x4F00, 0x4FFF);
				AddEntry(encoding, 0x5000, 0x51FF); // 512 character line
				AddEntry(encoding, 0x5200, 0x54FF); // 768 character line
				AddEntry(encoding, 0x5500, 0x58FF); // 1024 character line

				AddEntry(encoding, 0x103A0, 0x103C3); // Old Persian

				m_yaml.seekg(0, std::ios::beg);
			}

			std::istream& stream() {return m_yaml;}
			const std::vector<std::string>& entries() {return m_entries;}

		private:
			std::stringstream m_yaml;
			std::vector<std::string> m_entries;

			void AddEntry(EncodingFn encoding, int startCh, int endCh)
			{
				encoding(m_yaml, '-');
				encoding(m_yaml, ' ');
				encoding(m_yaml, '|');
				encoding(m_yaml, '\n');
				encoding(m_yaml, ' ');
				encoding(m_yaml, ' ');

				std::stringstream entry;
				for (int ch = startCh; ch <= endCh; ++ch)
				{
					encoding(m_yaml, ch);
					EncodeToUtf8(entry, ch);
				}
				encoding(m_yaml, '\n');

				m_entries.push_back(entry.str());
			}
		};

		void RunEncodingTest(EncodingFn encoding, bool declareEncoding, const std::string& name, bool& passed)
		{
			EncodingTester tester(encoding, declareEncoding);
			std::string error;
			bool ok = true;
			try {
				YAML::Parser parser(tester.stream());
				YAML::Node doc;
				parser.GetNextDocument(doc);

				YAML::Iterator itNode = doc.begin();
				std::vector<std::string>::const_iterator itEntry = tester.entries().begin();
				for (; (itNode != doc.end()) && (itEntry != tester.entries().end()); ++itNode, ++itEntry)
				{
					std::string stScalarValue;
					if (!itNode->GetScalar(stScalarValue) && (stScalarValue == *itEntry))
					{
						break;
					}
				}

				if ((itNode != doc.end()) || (itEntry != tester.entries().end()))
				{
					ok = false;
				}
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
		RunParserTest(&Parser::QuotedSimpleKeys, "quoted simple keys", passed);
		RunParserTest(&Parser::CompressedMapAndSeq, "compressed map and seq", passed);
		RunParserTest(&Parser::NullBlockSeqEntry, "null block seq entry", passed);
		RunParserTest(&Parser::NullBlockMapKey, "null block map key", passed);
		RunParserTest(&Parser::NullBlockMapValue, "null block map value", passed);
		RunParserTest(&Parser::SimpleAlias, "simple alias", passed);
		RunParserTest(&Parser::AliasWithNull, "alias with null", passed);
		RunParserTest(&Parser::ExplicitDoc, "explicit doc", passed);
		RunParserTest(&Parser::MultipleDocs, "multiple docs", passed);
		RunParserTest(&Parser::ExplicitEndDoc, "explicit end doc", passed);
		RunParserTest(&Parser::MultipleDocsWithSomeExplicitIndicators, "multiple docs with some explicit indicators", passed);
		
		RunEncodingTest(&EncodeToUtf8, false, "UTF-8, no BOM", passed);
		RunEncodingTest(&EncodeToUtf8, true, "UTF-8 with BOM", passed);
		RunEncodingTest(&EncodeToUtf16LE, false, "UTF-16LE, no BOM", passed);
		RunEncodingTest(&EncodeToUtf16LE, true, "UTF-16LE with BOM", passed);
		RunEncodingTest(&EncodeToUtf16BE, false, "UTF-16BE, no BOM", passed);
		RunEncodingTest(&EncodeToUtf16BE, true, "UTF-16BE with BOM", passed);
		RunEncodingTest(&EncodeToUtf32LE, false, "UTF-32LE, no BOM", passed);
		RunEncodingTest(&EncodeToUtf32LE, true, "UTF-32LE with BOM", passed);
		RunEncodingTest(&EncodeToUtf32BE, false, "UTF-32BE, no BOM", passed);
		RunEncodingTest(&EncodeToUtf32BE, true, "UTF-32BE with BOM", passed);
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
		RunEmitterTest(&Emitter::AliasAndAnchorWithNull, "alias and anchor with null", passed);
		RunEmitterTest(&Emitter::ComplexDoc, "complex doc", passed);
		RunEmitterTest(&Emitter::STLContainers, "STL containers", passed);
		RunEmitterTest(&Emitter::SimpleComment, "simple comment", passed);
		RunEmitterTest(&Emitter::MultiLineComment, "multi-line comment", passed);
		RunEmitterTest(&Emitter::ComplexComments, "complex comments", passed);
		RunEmitterTest(&Emitter::Indentation, "indentation", passed);
		RunEmitterTest(&Emitter::SimpleGlobalSettings, "simple global settings", passed);
		RunEmitterTest(&Emitter::ComplexGlobalSettings, "complex global settings", passed);
		RunEmitterTest(&Emitter::Null, "null", passed);
		
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
