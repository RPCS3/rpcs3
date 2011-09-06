#ifndef TESTS_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define TESTS_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <string>

namespace Test {
	void RunAll();

	namespace Parser {
		// scalar tests
		void SimpleScalar(std::string& inputScalar, std::string& desiredOutput);
		void MultiLineScalar(std::string& inputScalar, std::string& desiredOutput);
		void LiteralScalar(std::string& inputScalar, std::string& desiredOutput);
		void FoldedScalar(std::string& inputScalar, std::string& desiredOutput);
		void ChompedFoldedScalar(std::string& inputScalar, std::string& desiredOutput);
		void ChompedLiteralScalar(std::string& inputScalar, std::string& desiredOutput);
		void FoldedScalarWithIndent(std::string& inputScalar, std::string& desiredOutput);
		void ColonScalar(std::string& inputScalar, std::string& desiredOutput);
		void QuotedScalar(std::string& inputScalar, std::string& desiredOutput);
		void CommaScalar(std::string& inputScalar, std::string& desiredOutput);
		void DashScalar(std::string& inputScalar, std::string& desiredOutput);
		void URLScalar(std::string& inputScalar, std::string& desiredOutput);

		// misc tests
		bool SimpleSeq();
		bool SimpleMap();
		bool FlowSeq();
		bool FlowMap();
		bool FlowMapWithOmittedKey();
		bool FlowMapWithOmittedValue();
		bool FlowMapWithSoloEntry();
		bool FlowMapEndingWithSoloEntry();
		bool QuotedSimpleKeys();
		bool CompressedMapAndSeq();
		bool NullBlockSeqEntry();
		bool NullBlockMapKey();
		bool NullBlockMapValue();
		bool SimpleAlias();
		bool AliasWithNull();
		bool AnchorInSimpleKey();
		bool AliasAsSimpleKey();
		bool ExplicitDoc();
		bool MultipleDocs();
		bool ExplicitEndDoc();
		bool MultipleDocsWithSomeExplicitIndicators();
	}
}

#endif // TESTS_H_62B23520_7C8E_11DE_8A39_0800200C9A66
