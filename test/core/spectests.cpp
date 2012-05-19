#include "spectests.h"
#include "specexamples.h"
#include "yaml-cpp/yaml.h"
#include "yaml-cpp/eventhandler.h"
#include <cassert>

namespace Test {
    struct Event {
        enum Type { DocStart, DocEnd, Null, Alias, Scalar, SeqStart, SeqEnd, MapStart, MapEnd };
        
        typedef YAML::Mark Mark;
        typedef YAML::anchor_t anchor_t;

        Event(Type type_, const std::string& tag_, anchor_t anchor_, const std::string& scalar_): type(type_), tag(tag_), anchor(anchor_), scalar(scalar_) {}
        
        Type type;
        std::string tag;
        anchor_t anchor;
        std::string scalar;
        
        std::ostream& write(std::ostream& out) const {
            switch(type) {
                case DocStart:
                    return out << "DocStart";
                case DocEnd:
                    return out << "DocEnd";
                case Null:
                    return out << "Null(" << anchor << ")";
                case Alias:
                    return out << "Alias(" << anchor << ")";
                case Scalar:
                    return out << "Scalar(" << tag << ", " << anchor << ", " << scalar << ")";
                case SeqStart:
                    return out << "SeqStart(" << tag << ", " << anchor << ")";
                case SeqEnd:
                    return out << "SeqEnd";
                case MapStart:
                    return out << "MapStart(" << tag << ", " << anchor << ")";
                case MapEnd:
                    return out << "MapEnd";
            }
            assert(false);
            return out;
        }
    };
    
    std::ostream& operator << (std::ostream& out, const Event& event) {
        return event.write(out);
    }
    
    bool operator == (const Event& a, const Event& b) {
        return a.type == b.type && a.tag == b.tag && a.anchor == b.anchor && a.scalar == b.scalar;
    }
    
    bool operator != (const Event& a, const Event& b) {
        return !(a == b);
    }

	class MockEventHandler: public YAML::EventHandler
	{
	public:
        typedef YAML::Mark Mark;
        typedef YAML::anchor_t anchor_t;
        
        MockEventHandler() {}
        
		virtual void OnDocumentStart(const Mark&) {
            m_actualEvents.push_back(Event(Event::DocStart, "", 0, ""));
        }
        
		virtual void OnDocumentEnd() {
            m_actualEvents.push_back(Event(Event::DocEnd, "", 0, ""));
        }
		
		virtual void OnNull(const Mark&, anchor_t anchor) {
            m_actualEvents.push_back(Event(Event::Null, "", anchor, ""));
        }
        
		virtual void OnAlias(const Mark&, anchor_t anchor) {
            m_actualEvents.push_back(Event(Event::Alias, "", anchor, ""));
        }
        
		virtual void OnScalar(const Mark&, const std::string& tag, anchor_t anchor, const std::string& value) {
            m_actualEvents.push_back(Event(Event::Scalar, tag, anchor, value));
        }
        
		virtual void OnSequenceStart(const Mark&, const std::string& tag, anchor_t anchor) {
            m_actualEvents.push_back(Event(Event::SeqStart, tag, anchor, ""));
        }
        
		virtual void OnSequenceEnd() {
            m_actualEvents.push_back(Event(Event::SeqEnd, "", 0, ""));
        }
        
		virtual void OnMapStart(const Mark&, const std::string& tag, anchor_t anchor) {
            m_actualEvents.push_back(Event(Event::MapStart, tag, anchor, ""));
        }
        
		virtual void OnMapEnd() {
            m_actualEvents.push_back(Event(Event::MapEnd, "", 0, ""));
        }
        
        void Expect(const Event& event) { m_expectedEvents.push_back(event); }
        
        Test::TEST Check() const {
            std::size_t N = std::max(m_expectedEvents.size(), m_actualEvents.size());
            for(std::size_t i=0;i<N;i++) {
                if(i >= m_expectedEvents.size()) {
                    std::stringstream out;
                    for(std::size_t j=0;j<i;j++) {
                        out << m_expectedEvents[j] << "\n";
                    }
                    out << "EXPECTED: (no event expected)\n";
                    out << "ACTUAL  : " << m_actualEvents[i] << "\n";
                    return out.str().c_str();
                }

                if(i >= m_actualEvents.size()) {
                    std::stringstream out;
                    for(std::size_t j=0;j<i;j++) {
                        out << m_expectedEvents[j] << "\n";
                    }
                    out << "EXPECTED: " << m_expectedEvents[i] << "\n";
                    out << "ACTUAL  : (no event recorded)\n";
                    return out.str().c_str();
                }
                
                if(m_expectedEvents[i] != m_actualEvents[i]) {
                    std::stringstream out;
                    for(std::size_t j=0;j<i;j++) {
                        out << m_expectedEvents[j] << "\n";
                    }
                    out << "EXPECTED: " << m_expectedEvents[i] << "\n";
                    out << "ACTUAL  : " << m_actualEvents[i] << "\n";
                    return out.str().c_str();
                }
            }
            
            return true;
        }
        
        std::vector<Event> m_expectedEvents;
        std::vector<Event> m_actualEvents;
	};
    
#define HANDLE(ex)\
    MockEventHandler handler;\
    std::stringstream stream(ex);\
    YAML::Parser parser(stream);\
    parser.HandleNextDocument(handler)
    
#define HANDLE_NEXT(ex)\
    parser.HandleNextDocument(handler)
    
#define EXPECT_DOC_START()\
    do {\
        handler.Expect(Event(Event::DocStart, "", 0, ""));\
    } while(false)
    
#define EXPECT_DOC_END()\
    do {\
        handler.Expect(Event(Event::DocEnd, "", 0, ""));\
    } while(false)
    
#define EXPECT_NULL(anchor)\
    do {\
        handler.Expect(Event(Event::Null, "", anchor, ""));\
    } while(false)
    
#define EXPECT_ALIAS(anchor)\
    do {\
        handler.Expect(Event(Event::Alias, "", anchor, ""));\
    } while(false)
    
#define EXPECT_SCALAR(tag, anchor, value)\
    do {\
        handler.Expect(Event(Event::Scalar, tag, anchor, value));\
    } while(false)
    
#define EXPECT_SEQ_START(tag, anchor)\
    do {\
        handler.Expect(Event(Event::SeqStart, tag, anchor, ""));\
    } while(false)
    
#define EXPECT_SEQ_END()\
    do {\
        handler.Expect(Event(Event::SeqEnd, "", 0, ""));\
    } while(false)
    
#define EXPECT_MAP_START(tag, anchor)\
    do {\
        handler.Expect(Event(Event::MapStart, tag, anchor, ""));\
    } while(false)
    
#define EXPECT_MAP_END()\
    do {\
        handler.Expect(Event(Event::MapEnd, "", 0, ""));\
    } while(false)
    
#define DONE()\
    do {\
        return handler.Check();\
    } while(false)
    
	namespace Spec {
		// 2.1
		TEST SeqScalars()
        {
            HANDLE(ex2_1);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.2
		TEST MappingScalarsToScalars()
        {
            return "  not written yet";
        }
		
		// 2.3
		TEST MappingScalarsToSequences()
        {
            return "  not written yet";
        }
		
		// 2.4
		TEST SequenceOfMappings()
        {
            return "  not written yet";
        }
		
		// 2.5
		TEST SequenceOfSequences()
        {
            return "  not written yet";
        }
		
		// 2.6
		TEST MappingOfMappings()
        {
            return "  not written yet";
        }
		
		// 2.7
		TEST TwoDocumentsInAStream()
        {
            return "  not written yet";
        }
        
		// 2.8
		TEST PlayByPlayFeed()
        {
            return "  not written yet";
        }
		
		// 2.9
		TEST SingleDocumentWithTwoComments()
        {
            return "  not written yet";
        }
		
		// 2.10
		TEST SimpleAnchor()
        {
            return "  not written yet";
        }
		
		// 2.11
		TEST MappingBetweenSequences()
        {
            return "  not written yet";
        }
		
		// 2.12
		TEST CompactNestedMapping()
        {
            return "  not written yet";
        }
		
		// 2.13
		TEST InLiteralsNewlinesArePreserved()
        {
            return "  not written yet";
        }
		
		// 2.14
		TEST InFoldedScalarsNewlinesBecomeSpaces()
        {
            return "  not written yet";
        }
		
		// 2.15
		TEST FoldedNewlinesArePreservedForMoreIndentedAndBlankLines()
        {
            return "  not written yet";
        }
		
		// 2.16
		TEST IndentationDeterminesScope()
        {
            return "  not written yet";
        }
		
		// 2.17
		TEST QuotedScalars()
        {
            return "  not written yet";
        }
		
		// 2.18
		TEST MultiLineFlowScalars()
        {
            return "  not written yet";
        }
		
		// TODO: 2.19 - 2.22 schema tags
		
		// 2.23
		TEST VariousExplicitTags()
        {
            return "  not written yet";
        }
		
		// 2.24
		TEST GlobalTags()
        {
            return "  not written yet";
        }
		
		// 2.25
		TEST UnorderedSets()
        {
            return "  not written yet";
        }
		
		// 2.26
		TEST OrderedMappings()
        {
            return "  not written yet";
        }
		
		// 2.27
		TEST Invoice()
        {
            return "  not written yet";
        }
		
		// 2.28
		TEST LogFile()
        {
            return "  not written yet";
        }
		
		// TODO: 5.1 - 5.2 BOM
		
		// 5.3
		TEST BlockStructureIndicators()
        {
            return "  not written yet";
        }
		
		// 5.4
		TEST FlowStructureIndicators()
        {
            return "  not written yet";
        }
		
		// 5.5
		TEST CommentIndicator()
        {
            return "  not written yet";
        }
		
		// 5.6
		TEST NodePropertyIndicators()
        {
            return "  not written yet";
        }
		
		// 5.7
		TEST BlockScalarIndicators()
        {
            return "  not written yet";
        }
		
		// 5.8
		TEST QuotedScalarIndicators()
        {
            return "  not written yet";
        }
		
		// TODO: 5.9 directive
		// TODO: 5.10 reserved indicator
		
		// 5.11
		TEST LineBreakCharacters()
        {
            return "  not written yet";
        }
		
		// 5.12
		TEST TabsAndSpaces()
        {
            return "  not written yet";
        }
		
		// 5.13
		TEST EscapedCharacters()
        {
            return "  not written yet";
        }
		
		// 5.14
		TEST InvalidEscapedCharacters()
        {
            return "  not written yet";
        }
		
		// 6.1
		TEST IndentationSpaces()
        {
            return "  not written yet";
        }
		
		// 6.2
		TEST IndentationIndicators()
        {
            return "  not written yet";
        }
		
		// 6.3
		TEST SeparationSpaces()
        {
            return "  not written yet";
        }
		
		// 6.4
		TEST LinePrefixes()
        {
            return "  not written yet";
        }
		
		// 6.5
		TEST EmptyLines()
        {
            return "  not written yet";
        }
		
		// 6.6
		TEST LineFolding()
        {
            return "  not written yet";
        }
		
		// 6.7
		TEST BlockFolding()
        {
            return "  not written yet";
        }
		
		// 6.8
		TEST FlowFolding()
        {
            return "  not written yet";
        }
		
		// 6.9
		TEST SeparatedComment()
        {
            return "  not written yet";
        }
		
		// 6.10
		TEST CommentLines()
        {
            return "  not written yet";
        }
		
		// 6.11
		TEST MultiLineComments()
        {
            return "  not written yet";
        }
        
		// 6.12
		TEST SeparationSpacesII()
        {
            return "  not written yet";
        }
		
		// 6.13
		TEST ReservedDirectives()
        {
            return "  not written yet";
        }
		
		// 6.14
		TEST YAMLDirective()
        {
            return "  not written yet";
        }
		
		// 6.15
		TEST InvalidRepeatedYAMLDirective()
        {
            return "  not written yet";
        }
		
		// 6.16
		TEST TagDirective()
        {
            return "  not written yet";
        }
		
		// 6.17
		TEST InvalidRepeatedTagDirective()
        {
            return "  not written yet";
        }
		
		// 6.18
		TEST PrimaryTagHandle()
        {
            return "  not written yet";
        }
		
		// 6.19
		TEST SecondaryTagHandle()
        {
            return "  not written yet";
        }
		
		// 6.20
		TEST TagHandles()
        {
            return "  not written yet";
        }
		
		// 6.21
		TEST LocalTagPrefix()
        {
            return "  not written yet";
        }
		
		// 6.22
		TEST GlobalTagPrefix()
        {
            return "  not written yet";
        }
		
		// 6.23
		TEST NodeProperties()
        {
            return "  not written yet";
        }
		
		// 6.24
		TEST VerbatimTags()
        {
            return "  not written yet";
        }
		
		// 6.25
		TEST InvalidVerbatimTags()
        {
            return "  not written yet";
        }
		
		// 6.26
		TEST TagShorthands()
        {
            return "  not written yet";
        }
		
		// 6.27
		TEST InvalidTagShorthands()
        {
            return "  not written yet";
        }
		
		// 6.28
		TEST NonSpecificTags()
        {
            return "  not written yet";
        }
		
		// 6.29
		TEST NodeAnchors()
        {
            return "  not written yet";
        }
		
		// 7.1
		TEST AliasNodes()
        {
            return "  not written yet";
        }
		
		// 7.2
		TEST EmptyNodes()
        {
            return "  not written yet";
        }
		
		// 7.3
		TEST CompletelyEmptyNodes()
        {
            return "  not written yet";
        }
		
		// 7.4
		TEST DoubleQuotedImplicitKeys()
        {
            return "  not written yet";
        }
		
		// 7.5
		TEST DoubleQuotedLineBreaks()
        {
            return "  not written yet";
        }
		
		// 7.6
		TEST DoubleQuotedLines()
        {
            return "  not written yet";
        }
		
		// 7.7
		TEST SingleQuotedCharacters()
        {
            return "  not written yet";
        }
		
		// 7.8
		TEST SingleQuotedImplicitKeys()
        {
            return "  not written yet";
        }
		
		// 7.9
		TEST SingleQuotedLines()
        {
            return "  not written yet";
        }
		
		// 7.10
		TEST PlainCharacters()
        {
            return "  not written yet";
        }
		
		// 7.11
		TEST PlainImplicitKeys()
        {
            return "  not written yet";
        }
		
		// 7.12
		TEST PlainLines()
        {
            return "  not written yet";
        }
		
		// 7.13
		TEST FlowSequence()
        {
            return "  not written yet";
        }
		
		// 7.14
		TEST FlowSequenceEntries()
        {
            return "  not written yet";
        }
		
		// 7.15
		TEST FlowMappings()
        {
            return "  not written yet";
        }
		
		// 7.16
		TEST FlowMappingEntries()
        {
            return "  not written yet";
        }
		
		// 7.17
		TEST FlowMappingSeparateValues()
        {
            return "  not written yet";
        }
		
		// 7.18
		TEST FlowMappingAdjacentValues()
        {
            return "  not written yet";
        }
		
		// 7.19
		TEST SinglePairFlowMappings()
        {
            return "  not written yet";
        }
		
		// 7.20
		TEST SinglePairExplicitEntry()
        {
            return "  not written yet";
        }
		
		// 7.21
		TEST SinglePairImplicitEntries()
        {
            return "  not written yet";
        }
		
		// 7.22
		TEST InvalidImplicitKeys()
        {
            return "  not written yet";
        }
		
		// 7.23
		TEST FlowContent()
        {
            return "  not written yet";
        }
		
		// 7.24
		TEST FlowNodes()
        {
            return "  not written yet";
        }
		
		// 8.1
		TEST BlockScalarHeader()
        {
            return "  not written yet";
        }
		
		// 8.2
		TEST BlockIndentationHeader()
        {
            return "  not written yet";
        }
		
		// 8.3
		TEST InvalidBlockScalarIndentationIndicators()
        {
            return "  not written yet";
        }
		
		// 8.4
		TEST ChompingFinalLineBreak()
        {
            return "  not written yet";
        }
		
		// 8.5
		TEST ChompingTrailingLines()
        {
            return "  not written yet";
        }
		
		// 8.6
		TEST EmptyScalarChomping()
        {
            return "  not written yet";
        }
		
		// 8.7
		TEST LiteralScalar()
        {
            return "  not written yet";
        }
		
		// 8.8
		TEST LiteralContent()
        {
            return "  not written yet";
        }
		
		// 8.9
		TEST FoldedScalar()
        {
            return "  not written yet";
        }
		
		// 8.10
		TEST FoldedLines()
        {
            return "  not written yet";
        }
		
		// 8.11
		TEST MoreIndentedLines()
        {
            return "  not written yet";
        }
		
		// 8.12
		TEST EmptySeparationLines()
        {
            return "  not written yet";
        }
		
		// 8.13
		TEST FinalEmptyLines()
        {
            return "  not written yet";
        }
		
		// 8.14
		TEST BlockSequence()
        {
            return "  not written yet";
        }
		
		// 8.15
		TEST BlockSequenceEntryTypes()
        {
            return "  not written yet";
        }
		
		// 8.16
		TEST BlockMappings()
        {
            return "  not written yet";
        }
		
		// 8.17
		TEST ExplicitBlockMappingEntries()
        {
            return "  not written yet";
        }
		
		// 8.18
		TEST ImplicitBlockMappingEntries()
        {
            return "  not written yet";
        }
		
		// 8.19
		TEST CompactBlockMappings()
        {
            return "  not written yet";
        }
		
		// 8.20
		TEST BlockNodeTypes()
        {
            return "  not written yet";
        }
		
		// 8.21
		TEST BlockScalarNodes()
        {
            return "  not written yet";
        }
		
		// 8.22
		TEST BlockCollectionNodes()
        {
            return "  not written yet";
        }
	}
}
