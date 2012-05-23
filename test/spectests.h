#ifndef SPECTESTS_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define SPECTESTS_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "teststruct.h"

namespace Test {
	namespace Spec {
		// 2.1
		TEST SeqScalars();
		
		// 2.2
		TEST MappingScalarsToScalars();
		
		// 2.3
		TEST MappingScalarsToSequences();
		
		// 2.4
		TEST SequenceOfMappings();
		
		// 2.5
		TEST SequenceOfSequences();
		
		// 2.6
		TEST MappingOfMappings();
		
		// 2.7
		TEST TwoDocumentsInAStream();
	
		// 2.8
		TEST PlayByPlayFeed();
		
		// 2.9
		TEST SingleDocumentWithTwoComments();
		
		// 2.10
		TEST SimpleAnchor();
		
		// 2.11
		TEST MappingBetweenSequences();
		
		// 2.12
		TEST CompactNestedMapping();
		
		// 2.13
		TEST InLiteralsNewlinesArePreserved();
		
		// 2.14
		TEST InFoldedScalarsNewlinesBecomeSpaces();
		
		// 2.15
		TEST FoldedNewlinesArePreservedForMoreIndentedAndBlankLines();
		
		// 2.16
		TEST IndentationDeterminesScope();
		
		// 2.17
		TEST QuotedScalars();
		
		// 2.18
		TEST MultiLineFlowScalars();
		
		// TODO: 2.19 - 2.22 schema tags
		
		// 2.23
		TEST VariousExplicitTags();
		
		// 2.24
		TEST GlobalTags();
		
		// 2.25
		TEST UnorderedSets();
		
		// 2.26
		TEST OrderedMappings();
		
		// 2.27
		TEST Invoice();
		
		// 2.28
		TEST LogFile();
		
		// TODO: 5.1 - 5.2 BOM
		
		// 5.3
		TEST BlockStructureIndicators();
		
		// 5.4
		TEST FlowStructureIndicators();
		
		// 5.5
		TEST CommentIndicator();
		
		// 5.6
		TEST NodePropertyIndicators();
		
		// 5.7
		TEST BlockScalarIndicators();
		
		// 5.8
		TEST QuotedScalarIndicators();
		
		// TODO: 5.9 directive
		// TODO: 5.10 reserved indicator
		
		// 5.11
		TEST LineBreakCharacters();
		
		// 5.12
		TEST TabsAndSpaces();
		
		// 5.13
		TEST EscapedCharacters();
		
		// 5.14
		TEST InvalidEscapedCharacters();
		
		// 6.1
		TEST IndentationSpaces();
		
		// 6.2
		TEST IndentationIndicators();
		
		// 6.3
		TEST SeparationSpaces();
		
		// 6.4
		TEST LinePrefixes();
		
		// 6.5
		TEST EmptyLines();
		
		// 6.6
		TEST LineFolding();
		
		// 6.7
		TEST BlockFolding();
		
		// 6.8
		TEST FlowFolding();
		
		// 6.9
		TEST SeparatedComment();
		
		// 6.10
		TEST CommentLines();
		
		// 6.11
		TEST MultiLineComments();

		// 6.12
		TEST SeparationSpacesII();
		
		// 6.13
		TEST ReservedDirectives();
		
		// 6.14
		TEST YAMLDirective();
		
		// 6.15
		TEST InvalidRepeatedYAMLDirective();
		
		// 6.16
		TEST TagDirective();
		
		// 6.17
		TEST InvalidRepeatedTagDirective();
		
		// 6.18
		TEST PrimaryTagHandle();
		
		// 6.19
		TEST SecondaryTagHandle();
		
		// 6.20
		TEST TagHandles();
		
		// 6.21
		TEST LocalTagPrefix();
		
		// 6.22
		TEST GlobalTagPrefix();
		
		// 6.23
		TEST NodeProperties();
		
		// 6.24
		TEST VerbatimTags();
		
		// 6.25
		TEST InvalidVerbatimTags();
		
		// 6.26
		TEST TagShorthands();
		
		// 6.27
		TEST InvalidTagShorthands();
		
		// 6.28
		TEST NonSpecificTags();
		
		// 6.29
		TEST NodeAnchors();
		
		// 7.1
		TEST AliasNodes();
		
		// 7.2
		TEST EmptyNodes();
		
		// 7.3
		TEST CompletelyEmptyNodes();
		
		// 7.4
		TEST DoubleQuotedImplicitKeys();
		
		// 7.5
		TEST DoubleQuotedLineBreaks();
		
		// 7.6
		TEST DoubleQuotedLines();
		
		// 7.7
		TEST SingleQuotedCharacters();
		
		// 7.8
		TEST SingleQuotedImplicitKeys();
		
		// 7.9
		TEST SingleQuotedLines();
		
		// 7.10
		TEST PlainCharacters();
		
		// 7.11
		TEST PlainImplicitKeys();
		
		// 7.12
		TEST PlainLines();
		
		// 7.13
		TEST FlowSequence();
		
		// 7.14
		TEST FlowSequenceEntries();
		
		// 7.15
		TEST FlowMappings();
		
		// 7.16
		TEST FlowMappingEntries();
		
		// 7.17
		TEST FlowMappingSeparateValues();
		
		// 7.18
		TEST FlowMappingAdjacentValues();
		
		// 7.19
		TEST SinglePairFlowMappings();
		
		// 7.20
		TEST SinglePairExplicitEntry();
		
		// 7.21
		TEST SinglePairImplicitEntries();
		
		// 7.22
		TEST InvalidImplicitKeys();
		
		// 7.23
		TEST FlowContent();
		
		// 7.24
		TEST FlowNodes();
		
		// 8.1
		TEST BlockScalarHeader();
		
		// 8.2
		TEST BlockIndentationHeader();
		
		// 8.3
		TEST InvalidBlockScalarIndentationIndicators();
		
		// 8.4
		TEST ChompingFinalLineBreak();
		
		// 8.5
		TEST ChompingTrailingLines();
		
		// 8.6
		TEST EmptyScalarChomping();
		
		// 8.7
		TEST LiteralScalar();
		
		// 8.8
		TEST LiteralContent();
		
		// 8.9
		TEST FoldedScalar();
		
		// 8.10
		TEST FoldedLines();
		
		// 8.11
		TEST MoreIndentedLines();
		
		// 8.12
		TEST EmptySeparationLines();
		
		// 8.13
		TEST FinalEmptyLines();
		
		// 8.14
		TEST BlockSequence();
		
		// 8.15
		TEST BlockSequenceEntryTypes();
		
		// 8.16
		TEST BlockMappings();
		
		// 8.17
		TEST ExplicitBlockMappingEntries();
		
		// 8.18
		TEST ImplicitBlockMappingEntries();
		
		// 8.19
		TEST CompactBlockMappings();
		
		// 8.20
		TEST BlockNodeTypes();
		
		// 8.21
		TEST BlockScalarNodes();
		
		// 8.22
		TEST BlockCollectionNodes();
	}

	bool RunSpecTests();
}

#endif // SPECTESTS_H_62B23520_7C8E_11DE_8A39_0800200C9A66

