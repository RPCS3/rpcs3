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
	}
}
