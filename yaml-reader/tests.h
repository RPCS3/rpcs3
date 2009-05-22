#include <string>

namespace YAML { class Emitter; }

namespace Test {
	void RunAll(bool verbose);
	bool Inout(const std::string& file, bool verbose);
	
	bool RunEmitterTests();
	
	namespace Emitter {
		// correct emitting
		void SimpleScalar(YAML::Emitter& out, std::string& desiredOutput);
		void SimpleSeq(YAML::Emitter& out, std::string& desiredOutput);
		void SimpleFlowSeq(YAML::Emitter& ouptut, std::string& desiredOutput);
		void EmptyFlowSeq(YAML::Emitter& out, std::string& desiredOutput);
		void NestedBlockSeq(YAML::Emitter& out, std::string& desiredOutput);
		void NestedFlowSeq(YAML::Emitter& out, std::string& desiredOutput);
		void SimpleMap(YAML::Emitter& out, std::string& desiredOutput);
		void SimpleFlowMap(YAML::Emitter& out, std::string& desiredOutput);
		void MapAndList(YAML::Emitter& out, std::string& desiredOutput);
		void ListAndMap(YAML::Emitter& out, std::string& desiredOutput);
		void NestedBlockMap(YAML::Emitter& out, std::string& desiredOutput);
		void NestedFlowMap(YAML::Emitter& out, std::string& desiredOutput);
		void MapListMix(YAML::Emitter& out, std::string& desiredOutput);
		void SimpleLongKey(YAML::Emitter& out, std::string& desiredOutput);
		void SingleLongKey(YAML::Emitter& out, std::string& desiredOutput);
		void ComplexLongKey(YAML::Emitter& out, std::string& desiredOutput);
		void AutoLongKey(YAML::Emitter& out, std::string& desiredOutput);
		void ScalarFormat(YAML::Emitter& out, std::string& desiredOutput);
		void AutoLongKeyScalar(YAML::Emitter& out, std::string& desiredOutput);
		void LongKeyFlowMap(YAML::Emitter& out, std::string& desiredOutput);
		void BlockMapAsKey(YAML::Emitter& out, std::string& desiredOutput);
		void AliasAndAnchor(YAML::Emitter& out, std::string& desiredOutput);
		void ComplexDoc(YAML::Emitter& out, std::string& desiredOutput);
		void STLContainers(YAML::Emitter& out, std::string& desiredOutput);
		void SimpleComment(YAML::Emitter& out, std::string& desiredOutput);
		void MultiLineComment(YAML::Emitter& out, std::string& desiredOutput);
		void ComplexComments(YAML::Emitter& out, std::string& desiredOutput);
		void Indentation(YAML::Emitter& out, std::string& desiredOutput);
		void SimpleGlobalSettings(YAML::Emitter& out, std::string& desiredOutput);
		void ComplexGlobalSettings(YAML::Emitter& out, std::string& desiredOutput);

		// incorrect emitting
		void ExtraEndSeq(YAML::Emitter& out, std::string& desiredError);
		void ExtraEndMap(YAML::Emitter& out, std::string& desiredError);
		void BadSingleQuoted(YAML::Emitter& out, std::string& desiredError);
		void InvalidAnchor(YAML::Emitter& out, std::string& desiredError);
		void InvalidAlias(YAML::Emitter& out, std::string& desiredError);
		void MissingKey(YAML::Emitter& out, std::string& desiredError);
		void MissingValue(YAML::Emitter& out, std::string& desiredError);
		void UnexpectedKey(YAML::Emitter& out, std::string& desiredError);
		void UnexpectedValue(YAML::Emitter& out, std::string& desiredError);
	}
}
