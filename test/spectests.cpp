#include "spectests.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

namespace Test
{
	namespace {
		void RunSpecTest(TEST (*test)(), const std::string& index, const std::string& name, int& passed, int& total) {
			TEST ret;
			try {
				ret = test();
			} catch(const YAML::Exception& e) {
				ret.ok = false;
				ret.error = std::string("  Exception caught: ") + e.what();
			}
			
			if(!ret.ok) {
				std::cout << "Spec test " << index << " failed: " << name << "\n";
				std::cout << ret.error << "\n";
			}
			
			if(ret.ok)
				passed++;
			total++;
		}
	}
	
	bool RunSpecTests()
	{
		int passed = 0;
		int total = 0;
		RunSpecTest(&Spec::SeqScalars, "2.1", "Sequence of Scalars", passed, total);
		RunSpecTest(&Spec::MappingScalarsToScalars, "2.2", "Mapping Scalars to Scalars", passed, total);
		RunSpecTest(&Spec::MappingScalarsToSequences, "2.3", "Mapping Scalars to Sequences", passed, total);
		RunSpecTest(&Spec::SequenceOfMappings, "2.4", "Sequence of Mappings", passed, total);
		RunSpecTest(&Spec::SequenceOfSequences, "2.5", "Sequence of Sequences", passed, total);
		RunSpecTest(&Spec::MappingOfMappings, "2.6", "Mapping of Mappings", passed, total);
		RunSpecTest(&Spec::TwoDocumentsInAStream, "2.7", "Two Documents in a Stream", passed, total);
		RunSpecTest(&Spec::PlayByPlayFeed, "2.8", "Play by Play Feed from a Game", passed, total);
		RunSpecTest(&Spec::SingleDocumentWithTwoComments, "2.9", "Single Document with Two Comments", passed, total);
		RunSpecTest(&Spec::SimpleAnchor, "2.10", "Node for \"Sammy Sosa\" appears twice in this document", passed, total);
		RunSpecTest(&Spec::MappingBetweenSequences, "2.11", "Mapping between Sequences", passed, total);
		RunSpecTest(&Spec::CompactNestedMapping, "2.12", "Compact Nested Mapping", passed, total);
		RunSpecTest(&Spec::InLiteralsNewlinesArePreserved, "2.13", "In literals, newlines are preserved", passed, total);
		RunSpecTest(&Spec::InFoldedScalarsNewlinesBecomeSpaces, "2.14", "In folded scalars, newlines become spaces", passed, total);
		RunSpecTest(&Spec::FoldedNewlinesArePreservedForMoreIndentedAndBlankLines, "2.15", "Folded newlines are preserved for \"more indented\" and blank lines", passed, total);
		RunSpecTest(&Spec::IndentationDeterminesScope, "2.16", "Indentation determines scope", passed, total);
		RunSpecTest(&Spec::QuotedScalars, "2.17", "Quoted scalars", passed, total);
		RunSpecTest(&Spec::MultiLineFlowScalars, "2.18", "Multi-line flow scalars", passed, total);
		
		RunSpecTest(&Spec::VariousExplicitTags, "2.23", "Various Explicit Tags", passed, total);
		RunSpecTest(&Spec::GlobalTags, "2.24", "Global Tags", passed, total);
		RunSpecTest(&Spec::UnorderedSets, "2.25", "Unordered Sets", passed, total);
		RunSpecTest(&Spec::OrderedMappings, "2.26", "Ordered Mappings", passed, total);
		RunSpecTest(&Spec::Invoice, "2.27", "Invoice", passed, total);
		RunSpecTest(&Spec::LogFile, "2.28", "Log File", passed, total);
		
		RunSpecTest(&Spec::BlockStructureIndicators, "5.3", "Block Structure Indicators", passed, total);
		RunSpecTest(&Spec::FlowStructureIndicators, "5.4", "Flow Structure Indicators", passed, total);
		RunSpecTest(&Spec::NodePropertyIndicators, "5.6", "Node Property Indicators", passed, total);
		RunSpecTest(&Spec::BlockScalarIndicators, "5.7", "Block Scalar Indicators", passed, total);
		RunSpecTest(&Spec::QuotedScalarIndicators, "5.8", "Quoted Scalar Indicators", passed, total);
		RunSpecTest(&Spec::LineBreakCharacters, "5.11", "Line Break Characters", passed, total);
		RunSpecTest(&Spec::TabsAndSpaces, "5.12", "Tabs and Spaces", passed, total);
		RunSpecTest(&Spec::EscapedCharacters, "5.13", "Escaped Characters", passed, total);
		RunSpecTest(&Spec::InvalidEscapedCharacters, "5.14", "Invalid Escaped Characters", passed, total);
		
		RunSpecTest(&Spec::IndentationSpaces, "6.1", "Indentation Spaces", passed, total);
		RunSpecTest(&Spec::IndentationIndicators, "6.2", "Indentation Indicators", passed, total);
		RunSpecTest(&Spec::SeparationSpaces, "6.3", "Separation Spaces", passed, total);
		RunSpecTest(&Spec::LinePrefixes, "6.4", "Line Prefixes", passed, total);
		RunSpecTest(&Spec::EmptyLines, "6.5", "Empty Lines", passed, total);
		RunSpecTest(&Spec::LineFolding, "6.6", "Line Folding", passed, total);
		RunSpecTest(&Spec::BlockFolding, "6.7", "Block Folding", passed, total);
		RunSpecTest(&Spec::FlowFolding, "6.8", "Flow Folding", passed, total);
		RunSpecTest(&Spec::SeparatedComment, "6.9", "Separated Comment", passed, total);
		RunSpecTest(&Spec::CommentLines, "6.10", "Comment Lines", passed, total);
		RunSpecTest(&Spec::MultiLineComments, "6.11", "Multi-Line Comments", passed, total);
		RunSpecTest(&Spec::SeparationSpacesII, "6.12", "Separation Spaces", passed, total);
		RunSpecTest(&Spec::ReservedDirectives, "6.13", "Reserved Directives", passed, total);
		RunSpecTest(&Spec::YAMLDirective, "6.14", "YAML Directive", passed, total);
		RunSpecTest(&Spec::InvalidRepeatedYAMLDirective, "6.15", "Invalid Repeated YAML Directive", passed, total);
		RunSpecTest(&Spec::TagDirective, "6.16", "Tag Directive", passed, total);
		RunSpecTest(&Spec::InvalidRepeatedTagDirective, "6.17", "Invalid Repeated Tag Directive", passed, total);
		RunSpecTest(&Spec::PrimaryTagHandle, "6.18", "Primary Tag Handle", passed, total);
		RunSpecTest(&Spec::SecondaryTagHandle, "6.19", "SecondaryTagHandle", passed, total);
		RunSpecTest(&Spec::TagHandles, "6.20", "TagHandles", passed, total);
		RunSpecTest(&Spec::LocalTagPrefix, "6.21", "LocalTagPrefix", passed, total);
		RunSpecTest(&Spec::GlobalTagPrefix, "6.22", "GlobalTagPrefix", passed, total);
		RunSpecTest(&Spec::NodeProperties, "6.23", "NodeProperties", passed, total);
		RunSpecTest(&Spec::VerbatimTags, "6.24", "Verbatim Tags", passed, total);
		RunSpecTest(&Spec::InvalidVerbatimTags, "6.25", "Invalid Verbatim Tags", passed, total);
		RunSpecTest(&Spec::TagShorthands, "6.26", "Tag Shorthands", passed, total);
		RunSpecTest(&Spec::InvalidTagShorthands, "6.27", "Invalid Tag Shorthands", passed, total);
		RunSpecTest(&Spec::NonSpecificTags, "6.28", "Non Specific Tags", passed, total);
		RunSpecTest(&Spec::NodeAnchors, "6.29", "Node Anchors", passed, total);
		
		RunSpecTest(&Spec::AliasNodes, "7.1", "Alias Nodes", passed, total);
		RunSpecTest(&Spec::EmptyNodes, "7.2", "Empty Nodes", passed, total);
		RunSpecTest(&Spec::CompletelyEmptyNodes, "7.3", "Completely Empty Nodes", passed, total);
		RunSpecTest(&Spec::DoubleQuotedImplicitKeys, "7.4", "Double Quoted Implicit Keys", passed, total);
		RunSpecTest(&Spec::DoubleQuotedLineBreaks, "7.5", "Double Quoted Line Breaks", passed, total);
		RunSpecTest(&Spec::DoubleQuotedLines, "7.6", "Double Quoted Lines", passed, total);
		RunSpecTest(&Spec::SingleQuotedCharacters, "7.7", "Single Quoted Characters", passed, total);
		RunSpecTest(&Spec::SingleQuotedImplicitKeys, "7.8", "Single Quoted Implicit Keys", passed, total);
		RunSpecTest(&Spec::SingleQuotedLines, "7.9", "Single Quoted Lines", passed, total);
		RunSpecTest(&Spec::PlainCharacters, "7.10", "Plain Characters", passed, total);
		RunSpecTest(&Spec::PlainImplicitKeys, "7.11", "Plain Implicit Keys", passed, total);
		RunSpecTest(&Spec::PlainLines, "7.12", "Plain Lines", passed, total);
		RunSpecTest(&Spec::FlowSequence, "7.13", "Flow Sequence", passed, total);
		RunSpecTest(&Spec::FlowSequenceEntries, "7.14", "Flow Sequence Entries", passed, total);
		RunSpecTest(&Spec::FlowMappings, "7.15", "Flow Mappings", passed, total);
		RunSpecTest(&Spec::FlowMappingEntries, "7.16", "Flow Mapping Entries", passed, total);
		RunSpecTest(&Spec::FlowMappingSeparateValues, "7.17", "Flow Mapping Separate Values", passed, total);
		RunSpecTest(&Spec::FlowMappingAdjacentValues, "7.18", "Flow Mapping Adjacent Values", passed, total);
		RunSpecTest(&Spec::SinglePairFlowMappings, "7.19", "Single Pair Flow Mappings", passed, total);
		RunSpecTest(&Spec::SinglePairExplicitEntry, "7.20", "Single Pair Explicit Entry", passed, total);
		RunSpecTest(&Spec::SinglePairImplicitEntries, "7.21", "Single Pair Implicit Entries", passed, total);
		RunSpecTest(&Spec::InvalidImplicitKeys, "7.22", "Invalid Implicit Keys", passed, total);
		RunSpecTest(&Spec::FlowContent, "7.23", "Flow Content", passed, total);
		RunSpecTest(&Spec::FlowNodes, "7.24", "FlowNodes", passed, total);
		
		RunSpecTest(&Spec::BlockScalarHeader, "8.1", "Block Scalar Header", passed, total);
		RunSpecTest(&Spec::BlockIndentationHeader, "8.2", "Block Indentation Header", passed, total);
		RunSpecTest(&Spec::InvalidBlockScalarIndentationIndicators, "8.3", "Invalid Block Scalar Indentation Indicators", passed, total);
		RunSpecTest(&Spec::ChompingFinalLineBreak, "8.4", "Chomping Final Line Break", passed, total);
		RunSpecTest(&Spec::ChompingTrailingLines, "8.5", "Chomping Trailing Lines", passed, total);
		RunSpecTest(&Spec::EmptyScalarChomping, "8.6", "Empty Scalar Chomping", passed, total);
		RunSpecTest(&Spec::LiteralScalar, "8.7", "Literal Scalar", passed, total);
		RunSpecTest(&Spec::LiteralContent, "8.8", "Literal Content", passed, total);
		RunSpecTest(&Spec::FoldedScalar, "8.9", "Folded Scalar", passed, total);
		RunSpecTest(&Spec::FoldedLines, "8.10", "Folded Lines", passed, total);
		RunSpecTest(&Spec::MoreIndentedLines, "8.11", "More Indented Lines", passed, total);
		RunSpecTest(&Spec::EmptySeparationLines, "8.12", "Empty Separation Lines", passed, total);
		RunSpecTest(&Spec::FinalEmptyLines, "8.13", "Final Empty Lines", passed, total);
		RunSpecTest(&Spec::BlockSequence, "8.14", "Block Sequence", passed, total);
		RunSpecTest(&Spec::BlockSequenceEntryTypes, "8.15", "Block Sequence Entry Types", passed, total);
		RunSpecTest(&Spec::BlockMappings, "8.16", "Block Mappings", passed, total);
		RunSpecTest(&Spec::ExplicitBlockMappingEntries, "8.17", "Explicit Block Mapping Entries", passed, total);
		RunSpecTest(&Spec::ImplicitBlockMappingEntries, "8.18", "Implicit Block Mapping Entries", passed, total);
		RunSpecTest(&Spec::CompactBlockMappings, "8.19", "Compact Block Mappings", passed, total);
		RunSpecTest(&Spec::BlockNodeTypes, "8.20", "Block Node Types", passed, total);
		RunSpecTest(&Spec::BlockScalarNodes, "8.21", "Block Scalar Nodes", passed, total);
		RunSpecTest(&Spec::BlockCollectionNodes, "8.22", "Block Collection Nodes", passed, total);
		
		std::cout << "Spec tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
}
