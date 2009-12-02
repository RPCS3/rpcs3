#include "tests.h"
#include "yaml.h"
#include <sstream>
#include <algorithm>

namespace Test
{
	namespace Parser {
		void SimpleScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar = "Hello, World!";
			desiredOutput = "Hello, World!";
		}

		void MultiLineScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar =
				"normal scalar, but\n"
				"over several lines";
			desiredOutput = "normal scalar, but over several lines";
		}

		void LiteralScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar =
				"|\n"
				" literal scalar - so we can draw ASCII:\n"
				" \n"
                "          -   -\n"
                "         |  -  |\n"
                "          -----\n";
			desiredOutput =
				"literal scalar - so we can draw ASCII:\n"
				"\n"
                "         -   -\n"
                "        |  -  |\n"
                "         -----\n";
		}

		void FoldedScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar =
				">\n"
				" and a folded scalar... so we\n"
				" can just keep writing various\n"
				" things. And if we want to keep indentation:\n"
				" \n"
				"    we just indent a little\n"
				"    see, this stays indented";
			desiredOutput =
				"and a folded scalar... so we"
				" can just keep writing various"
				" things. And if we want to keep indentation:\n"
				"\n"
				"   we just indent a little\n"
				"   see, this stays indented";
		}

		void ChompedFoldedScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar =
				">-\n"
				"  Here's a folded scalar\n"
				"  that gets chomped.";
			desiredOutput =
				"Here's a folded scalar"
				" that gets chomped.";
		}

		void ChompedLiteralScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar =
				"|-\n"
				"  Here's a literal scalar\n"
				"  that gets chomped.";
			desiredOutput =
				"Here's a literal scalar\n"
				"that gets chomped.";
		}

		void FoldedScalarWithIndent(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar =
				">2\n"
				"       Here's a folded scalar\n"
				"  that starts with some indentation.";
			desiredOutput =
				"     Here's a folded scalar\n"
				"that starts with some indentation.";
		}

		void ColonScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar = "::vector";
			desiredOutput = "::vector";
		}

		void QuotedScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar = "\": - ()\"";
			desiredOutput = ": - ()";
		}

		void CommaScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar = "Up, up, and away!";
			desiredOutput = "Up, up, and away!";
		}

		void DashScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar = "-123";
			desiredOutput = "-123";
		}

		void URLScalar(std::string& inputScalar, std::string& desiredOutput)
		{
			inputScalar = "http://example.com/foo#bar";
			desiredOutput = "http://example.com/foo#bar";
		}
		
		bool SimpleSeq()
		{
			std::string input =
				"- eggs\n"
				"- bread\n"
				"- milk";

			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);

			std::string output;
			doc[0] >> output;
			if(output != "eggs")
				return false;
			doc[1] >> output;
			if(output != "bread")
				return false;
			doc[2] >> output;
			if(output != "milk")
				return false;

			return true;
		}

		bool SimpleMap()
		{
			std::string input =
				"name: Prince Fielder\n"
				"position: 1B\n"
				"bats: L";

			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);

			std::string output;
			doc["name"] >> output;
			if(output != "Prince Fielder")
				return false;
			doc["position"] >> output;
			if(output != "1B")
				return false;
			doc["bats"] >> output;
			if(output != "L")
				return false;

			return true;
		}

		bool FlowSeq()
		{
			std::string input = "[ 2 , 3, 5  ,  7,   11]";

			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);

			int output;
			doc[0] >> output;
			if(output != 2)
				return false;
			doc[1] >> output;
			if(output != 3)
				return false;
			doc[2] >> output;
			if(output != 5)
				return false;
			doc[3] >> output;
			if(output != 7)
				return false;
			doc[4] >> output;
			if(output != 11)
				return false;

			return true;
		}

		bool FlowMap()
		{
			std::string input = "{hr: 65, avg: 0.278}";

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

			return true;
		}

		bool FlowMapWithOmittedKey()
		{
			std::string input = "{: omitted key}";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			std::string output;
			doc[YAML::Null] >> output;
			if(output != "omitted key")
				return false;
			
			return true;
		}
		
		bool FlowMapWithOmittedValue()
		{
			std::string input = "{a: b, c:, d:}";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);

			std::string output;
			doc["a"] >> output;
			if(output != "b")
				return false;
			if(!IsNull(doc["c"]))
				return false;
			if(!IsNull(doc["d"]))
				return false;

			return true;
		}

		bool FlowMapWithSoloEntry()
		{
			std::string input = "{a: b, c, d: e}";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			std::string output;
			doc["a"] >> output;
			if(output != "b")
				return false;
			if(!IsNull(doc["c"]))
				return false;
			doc["d"] >> output;
			if(output != "e")
				return false;
			
			return true;
		}
		
		bool FlowMapEndingWithSoloEntry()
		{
			std::string input = "{a: b, c}";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			std::string output;
			doc["a"] >> output;
			if(output != "b")
				return false;
			if(!IsNull(doc["c"]))
				return false;
			
			return true;
		}
		
		bool QuotedSimpleKeys()
		{
			std::string KeyValue[3] = { "\"double\": double\n", "'single': single\n", "plain: plain\n" };
			
			int perm[3] = { 0, 1, 2 };
			do {
				std::string input = KeyValue[perm[0]] + KeyValue[perm[1]] + KeyValue[perm[2]];

				std::stringstream stream(input);
				YAML::Parser parser(stream);
				YAML::Node doc;
				parser.GetNextDocument(doc);
				
				std::string output;
				doc["double"] >> output;
				if(output != "double")
					return false;
				doc["single"] >> output;
				if(output != "single")
					return false;
				doc["plain"] >> output;
				if(output != "plain")
					return false;
			} while(std::next_permutation(perm, perm + 3));
				
			return true;
		}

		bool CompressedMapAndSeq()
		{
			std::string input = "key:\n- one\n- two";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			const YAML::Node& seq = doc["key"];
			if(seq.size() != 2)
				return false;
				
			std::string output;
			seq[0] >> output;
			if(output != "one")
				return false;
			seq[1] >> output;
			if(output != "two")
				return false;

			return true;
		}

		bool NullBlockSeqEntry()
		{
			std::string input = "- hello\n-\n- world";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			std::string output;
			doc[0] >> output;
			if(output != "hello")
				return false;
			if(!IsNull(doc[1]))
				return false;
			doc[2] >> output;
			if(output != "world")
				return false;
			
			return true;
		}
		
		bool NullBlockMapKey()
		{
			std::string input = ": empty key";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			std::string output;
			doc[YAML::Null] >> output;
			if(output != "empty key")
				return false;
			
			return true;
		}
		
		bool NullBlockMapValue()
		{
			std::string input = "empty value:";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			if(!IsNull(doc["empty value"]))
				return false;
			
			return true;
		}

		bool SimpleAlias()
		{
			std::string input = "- &alias test\n- *alias";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			std::string output;
			doc[0] >> output;
			if(output != "test")
				return false;
			
			doc[1] >> output;
			if(output != "test")
				return false;
			
			if(doc.size() != 2)
				return false;
			
			return true;
		}
		
		bool AliasWithNull()
		{
			std::string input = "- &alias\n- *alias";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			if(!IsNull(doc[0]))
				return false;
			
			if(!IsNull(doc[1]))
				return false;
			
			if(doc.size() != 2)
				return false;
			
			return true;
		}

		bool AnchorInSimpleKey()
		{
			std::string input = "- &a b: c\n- *a";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			if(doc.size() != 2)
				return false;
	
			std::string output;
			doc[0]["b"] >> output;
			if(output != "c")
				return false;
			
			doc[1] >> output;
			if(output != "b")
				return false;
			
			return true;
		}
		
		bool AliasAsSimpleKey()
		{
			std::string input = "- &a b\n- *a: c";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);

			if(doc.size() != 2)
				return false;
			
			std::string output;
			doc[0] >> output;
			if(output != "b")
				return false;
			
			doc[1]["b"] >> output;
			if(output != "c")
				return false;

			return true;
		}
		
		bool ExplicitDoc()
		{
			std::string input = "---\n- one\n- two";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			if(doc.size() != 2)
				return false;
			
			std::string output;
			doc[0] >> output;
			if(output != "one")
				return false;
			doc[1] >> output;
			if(output != "two")
				return false;
			
			return true;
		}

		bool MultipleDocs()
		{
			std::string input = "---\nname: doc1\n---\nname: doc2";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);

			std::string output;
			doc["name"] >> output;
			if(output != "doc1")
				return false;
			
			if(!parser)
				return false;
			
			parser.GetNextDocument(doc);
			doc["name"] >> output;
			if(output != "doc2")
				return false;
			
			return true;
		}
		
		bool ExplicitEndDoc()
		{
			std::string input = "- one\n- two\n...\n...";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			if(doc.size() != 2)
				return false;
			
			std::string output;
			doc[0] >> output;
			if(output != "one")
				return false;
			doc[1] >> output;
			if(output != "two")
				return false;
			
			return true;
		}
		
		bool MultipleDocsWithSomeExplicitIndicators()
		{
			std::string input =
				"- one\n- two\n...\n"
				"---\nkey: value\n...\n...\n"
				"- three\n- four\n"
				"---\nkey: value";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			std::string output;

			parser.GetNextDocument(doc);
			if(doc.size() != 2)
				return false;
			doc[0] >> output;
			if(output != "one")
				return false;
			doc[1] >> output;
			if(output != "two")
				return false;
			
			parser.GetNextDocument(doc);
			doc["key"] >> output;
			if(output != "value")
				return false;
			
			parser.GetNextDocument(doc);
			if(doc.size() != 2)
				return false;
			doc[0] >> output;
			if(output != "three")
				return false;
			doc[1] >> output;
			if(output != "four")
				return false;
			
			parser.GetNextDocument(doc);
			doc["key"] >> output;
			if(output != "value")
				return false;
			
			return true;
		}

		bool BlockKeyWithNullValue()
		{
			std::string input =
				"key:\n"
				"just a key: value";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			
			parser.GetNextDocument(doc);
			if(doc.size() != 2)
				return false;
			if(!IsNull(doc["key"]))
			   return false;
			if(doc["just a key"] != "value")
				return false;
			
			return true;
		}
		
		bool Bases()
		{
			std::string input =
				"- 15\n"
				"- 0x10\n"
				"- 030\n"
				"- 0xffffffff\n";
			
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			
			parser.GetNextDocument(doc);
			if(doc.size() != 4)
				return false;
			if(doc[0] != 15)
				return false;
			if(doc[1] != 0x10)
				return false;
			if(doc[2] != 030)
				return false;
			if(doc[3] != 0xffffffff)
				return false;
			return true;
		}
		
		bool KeyNotFound()
		{
			std::string input = "key: value";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			try {
				doc["bad key"];
			} catch(const YAML::Exception& e) {
				if(e.msg != YAML::ErrorMsg::KEY_NOT_FOUND + ": bad key")
					throw;
			}

			try {
				doc[5];
			} catch(const YAML::Exception& e) {
				if(e.msg != YAML::ErrorMsg::KEY_NOT_FOUND + ": 5")
					throw;
			}

			try {
				doc[2.5];
			} catch(const YAML::Exception& e) {
				if(e.msg != YAML::ErrorMsg::KEY_NOT_FOUND + ": 2.5")
					throw;
			}

			return true;
		}
		
		bool DuplicateKey()
		{
			std::string input = "{a: 1, b: 2, c: 3, a: 4}";
			std::stringstream stream(input);
			YAML::Parser parser(stream);
			YAML::Node doc;
			parser.GetNextDocument(doc);
			
			if(doc["a"] != 1)
				return false;
			if(doc["b"] != 2)
				return false;
			if(doc["c"] != 3)
				return false;
			return true;
		}
	}
	
	namespace {
		void RunScalarParserTest(void (*test)(std::string&, std::string&), const std::string& name, int& passed, int& total) {
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
				error = e.what();
			}
			if(ok && output == desiredOutput) {
				passed++;
			} else {
				std::cout << "Parser test failed: " << name << "\n";
				if(error != "")
					std::cout << "Caught exception: " << error << "\n";
				else {
					std::cout << "Output:\n" << output << "<<<\n";
					std::cout << "Desired output:\n" << desiredOutput << "<<<\n";
				}
			}
			total++;
		}

		void RunParserTest(bool (*test)(), const std::string& name, int& passed, int& total) {
			std::string error;
			bool ok = true;
			try {
				ok = test();
			} catch(const YAML::Exception& e) {
				ok = false;
				error = e.msg;
			}
			if(ok) {
				passed++;
			} else {
				std::cout << "Parser test failed: " << name << "\n";
				if(error != "")
					std::cout << "Caught exception: " << error << "\n";
			}
			total++;
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

		void RunEncodingTest(EncodingFn encoding, bool declareEncoding, const std::string& name, int& passed, int& total)
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
				passed++;
			} else {
				std::cout << "Parser test failed: " << name << "\n";
				if(error != "")
					std::cout << "Caught exception: " << error << "\n";
			}
			total++;
		}
	}

	bool RunParserTests()
	{
		int passed = 0;
		int total = 0;
		RunScalarParserTest(&Parser::SimpleScalar, "simple scalar", passed, total);
		RunScalarParserTest(&Parser::MultiLineScalar, "multi-line scalar", passed, total);
		RunScalarParserTest(&Parser::LiteralScalar, "literal scalar", passed, total);
		RunScalarParserTest(&Parser::FoldedScalar, "folded scalar", passed, total);
		RunScalarParserTest(&Parser::ChompedFoldedScalar, "chomped folded scalar", passed, total);
		RunScalarParserTest(&Parser::ChompedLiteralScalar, "chomped literal scalar", passed, total);
		RunScalarParserTest(&Parser::FoldedScalarWithIndent, "folded scalar with indent", passed, total);
		RunScalarParserTest(&Parser::ColonScalar, "colon scalar", passed, total);
		RunScalarParserTest(&Parser::QuotedScalar, "quoted scalar", passed, total);
		RunScalarParserTest(&Parser::CommaScalar, "comma scalar", passed, total);
		RunScalarParserTest(&Parser::DashScalar, "dash scalar", passed, total);
		RunScalarParserTest(&Parser::URLScalar, "url scalar", passed, total);

		RunParserTest(&Parser::SimpleSeq, "simple seq", passed, total);
		RunParserTest(&Parser::SimpleMap, "simple map", passed, total);
		RunParserTest(&Parser::FlowSeq, "flow seq", passed, total);
		RunParserTest(&Parser::FlowMap, "flow map", passed, total);
		RunParserTest(&Parser::FlowMapWithOmittedKey, "flow map with omitted key", passed, total);
		RunParserTest(&Parser::FlowMapWithOmittedValue, "flow map with omitted value", passed, total);
		RunParserTest(&Parser::FlowMapWithSoloEntry, "flow map with solo entry", passed, total);
		RunParserTest(&Parser::FlowMapEndingWithSoloEntry, "flow map ending with solo entry", passed, total);
		RunParserTest(&Parser::QuotedSimpleKeys, "quoted simple keys", passed, total);
		RunParserTest(&Parser::CompressedMapAndSeq, "compressed map and seq", passed, total);
		RunParserTest(&Parser::NullBlockSeqEntry, "null block seq entry", passed, total);
		RunParserTest(&Parser::NullBlockMapKey, "null block map key", passed, total);
		RunParserTest(&Parser::NullBlockMapValue, "null block map value", passed, total);
		RunParserTest(&Parser::SimpleAlias, "simple alias", passed, total);
		RunParserTest(&Parser::AliasWithNull, "alias with null", passed, total);
		RunParserTest(&Parser::AnchorInSimpleKey, "anchor in simple key", passed, total);
		RunParserTest(&Parser::AliasAsSimpleKey, "alias as simple key", passed, total);
		RunParserTest(&Parser::ExplicitDoc, "explicit doc", passed, total);
		RunParserTest(&Parser::MultipleDocs, "multiple docs", passed, total);
		RunParserTest(&Parser::ExplicitEndDoc, "explicit end doc", passed, total);
		RunParserTest(&Parser::MultipleDocsWithSomeExplicitIndicators, "multiple docs with some explicit indicators", passed, total);
		RunParserTest(&Parser::BlockKeyWithNullValue, "block key with null value", passed, total);
		RunParserTest(&Parser::Bases, "bases", passed, total);
		RunParserTest(&Parser::KeyNotFound, "key not found", passed, total);
		RunParserTest(&Parser::DuplicateKey, "duplicate key", passed, total);
		
		RunEncodingTest(&EncodeToUtf8, false, "UTF-8, no BOM", passed, total);
		RunEncodingTest(&EncodeToUtf8, true, "UTF-8 with BOM", passed, total);
		RunEncodingTest(&EncodeToUtf16LE, false, "UTF-16LE, no BOM", passed, total);
		RunEncodingTest(&EncodeToUtf16LE, true, "UTF-16LE with BOM", passed, total);
		RunEncodingTest(&EncodeToUtf16BE, false, "UTF-16BE, no BOM", passed, total);
		RunEncodingTest(&EncodeToUtf16BE, true, "UTF-16BE with BOM", passed, total);
		RunEncodingTest(&EncodeToUtf32LE, false, "UTF-32LE, no BOM", passed, total);
		RunEncodingTest(&EncodeToUtf32LE, true, "UTF-32LE with BOM", passed, total);
		RunEncodingTest(&EncodeToUtf32BE, false, "UTF-32BE, no BOM", passed, total);
		RunEncodingTest(&EncodeToUtf32BE, true, "UTF-32BE with BOM", passed, total);

		std::cout << "Parser tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
}

