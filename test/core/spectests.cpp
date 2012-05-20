#include "spectests.h"
#include "specexamples.h"
#include "yaml-cpp/yaml.h"
#include "yaml-cpp/eventhandler.h"
#include <cassert>

#define YAML_ASSERT(cond) do { if(!(cond)) return "  Assert failed: " #cond; } while(false)

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
    while(parser.HandleNextDocument(handler))\
    
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
            HANDLE(ex2_2);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.278");
            EXPECT_SCALAR("?", 0, "rbi");
            EXPECT_SCALAR("?", 0, "147");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.3
		TEST MappingScalarsToSequences()
        {
            HANDLE(ex2_3);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "american");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Boston Red Sox");
            EXPECT_SCALAR("?", 0, "Detroit Tigers");
            EXPECT_SCALAR("?", 0, "New York Yankees");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "national");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "New York Mets");
            EXPECT_SCALAR("?", 0, "Chicago Cubs");
            EXPECT_SCALAR("?", 0, "Atlanta Braves");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.4
		TEST SequenceOfMappings()
        {
            HANDLE(ex2_4);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "name");
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.278");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "name");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "63");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.288");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.5
		TEST SequenceOfSequences()
        {
            HANDLE(ex2_5);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "name");
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_SCALAR("?", 0, "0.278");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "63");
            EXPECT_SCALAR("?", 0, "0.288");
            EXPECT_SEQ_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.6
		TEST MappingOfMappings()
        {
            HANDLE(ex2_6);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.278");
            EXPECT_MAP_END();
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SCALAR("?", 0, "63");
            EXPECT_SCALAR("?", 0, "avg");
            EXPECT_SCALAR("?", 0, "0.288");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.7
		TEST TwoDocumentsInAStream()
        {
            HANDLE(ex2_7);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Chicago Cubs");
            EXPECT_SCALAR("?", 0, "St Louis Cardinals");
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
        
		// 2.8
		TEST PlayByPlayFeed()
        {
            HANDLE(ex2_8);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "time");
            EXPECT_SCALAR("?", 0, "20:03:20");
            EXPECT_SCALAR("?", 0, "player");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "action");
            EXPECT_SCALAR("?", 0, "strike (miss)");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "time");
            EXPECT_SCALAR("?", 0, "20:03:47");
            EXPECT_SCALAR("?", 0, "player");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "action");
            EXPECT_SCALAR("?", 0, "grand slam");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.9
		TEST SingleDocumentWithTwoComments()
        {
            HANDLE(ex2_9);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "rbi");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.10
		TEST SimpleAnchor()
        {
            HANDLE(ex2_10);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "hr");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 1, "Sammy Sosa");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "rbi");
            EXPECT_SEQ_START("?", 0);
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.11
		TEST MappingBetweenSequences()
        {
            HANDLE(ex2_11);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "Detroit Tigers");
            EXPECT_SCALAR("?", 0, "Chicago cubs");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "2001-07-23");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "New York Yankees");
            EXPECT_SCALAR("?", 0, "Atlanta Braves");
            EXPECT_SEQ_END();
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "2001-07-02");
            EXPECT_SCALAR("?", 0, "2001-08-12");
            EXPECT_SCALAR("?", 0, "2001-08-14");
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.12
		TEST CompactNestedMapping()
        {
            HANDLE(ex2_12);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "item");
            EXPECT_SCALAR("?", 0, "Super Hoop");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "1");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "item");
            EXPECT_SCALAR("?", 0, "Basketball");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "4");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "item");
            EXPECT_SCALAR("?", 0, "Big Shoes");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "1");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.13
		TEST InLiteralsNewlinesArePreserved()
        {
            HANDLE(ex2_13);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0,
                          "\\//||\\/||\n"
                          "// ||  ||__");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.14
		TEST InFoldedScalarsNewlinesBecomeSpaces()
        {
            HANDLE(ex2_14);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "Mark McGwire's year was crippled by a knee injury.");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.15
		TEST FoldedNewlinesArePreservedForMoreIndentedAndBlankLines()
        {
            HANDLE(ex2_15);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0,
                          "Sammy Sosa completed another fine season with great stats.\n"
                          "\n"
                          "  63 Home Runs\n"
                          "  0.288 Batting Average\n"
                          "\n"
                          "What a year!");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.16
		TEST IndentationDeterminesScope()
        {
            HANDLE(ex2_16);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "name");
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "accomplishment");
            EXPECT_SCALAR("!", 0, "Mark set a major league home run record in 1998.\n");
            EXPECT_SCALAR("?", 0, "stats");
            EXPECT_SCALAR("!", 0,
                          "65 Home Runs\n"
                          "0.278 Batting Average\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.17
		TEST QuotedScalars()
        {
            HANDLE(ex2_17);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "unicode");
            EXPECT_SCALAR("!", 0, "Sosa did fine.\u263A");
            EXPECT_SCALAR("?", 0, "control");
            EXPECT_SCALAR("!", 0, "\b1998\t1999\t2000\n");
            EXPECT_SCALAR("?", 0, "hex esc");
            EXPECT_SCALAR("!", 0, "\x0d\x0a is \r\n");
            EXPECT_SCALAR("?", 0, "single");
            EXPECT_SCALAR("!", 0, "\"Howdy!\" he cried.");
            EXPECT_SCALAR("?", 0, "quoted");
            EXPECT_SCALAR("!", 0, " # Not a 'comment'.");
            EXPECT_SCALAR("?", 0, "tie-fighter");
            EXPECT_SCALAR("!", 0, "|\\-*-/|");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.18
		TEST MultiLineFlowScalars()
        {
            HANDLE(ex2_18);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "plain");
            EXPECT_SCALAR("?", 0, "This unquoted scalar spans many lines.");
            EXPECT_SCALAR("?", 0, "quoted");
            EXPECT_SCALAR("!", 0, "So does this quoted scalar.\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// TODO: 2.19 - 2.22 schema tags
		
		// 2.23
		TEST VariousExplicitTags()
        {
            HANDLE(ex2_23);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "not-date");
            EXPECT_SCALAR("tag:yaml.org,2002:str", 0, "2002-04-28");
            EXPECT_SCALAR("?", 0, "picture");
            EXPECT_SCALAR("tag:yaml.org,2002:binary", 0,
                          "R0lGODlhDAAMAIQAAP//9/X\n"
                          "17unp5WZmZgAAAOfn515eXv\n"
                          "Pz7Y6OjuDg4J+fn5OTk6enp\n"
                          "56enmleECcgggoBADs=\n");
            EXPECT_SCALAR("?", 0, "application specific tag");
            EXPECT_SCALAR("!something", 0,
                          "The semantics of the tag\n"
                          "above may be different for\n"
                          "different documents.");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.24
		TEST GlobalTags()
        {
            HANDLE(ex2_24);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("tag:clarkevans.com,2002:shape", 0);
            EXPECT_MAP_START("tag:clarkevans.com,2002:circle", 0);
            EXPECT_SCALAR("?", 0, "center");
            EXPECT_MAP_START("?", 1);
            EXPECT_SCALAR("?", 0, "x");
            EXPECT_SCALAR("?", 0, "73");
            EXPECT_SCALAR("?", 0, "y");
            EXPECT_SCALAR("?", 0, "129");
            EXPECT_MAP_END();
            EXPECT_SCALAR("?", 0, "radius");
            EXPECT_SCALAR("?", 0, "7");
            EXPECT_MAP_END();
            EXPECT_MAP_START("tag:clarkevans.com,2002:line", 0);
            EXPECT_SCALAR("?", 0, "start");
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("?", 0, "finish");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "x");
            EXPECT_SCALAR("?", 0, "89");
            EXPECT_SCALAR("?", 0, "y");
            EXPECT_SCALAR("?", 0, "102");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_MAP_START("tag:clarkevans.com,2002:label", 0);
            EXPECT_SCALAR("?", 0, "start");
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("?", 0, "color");
            EXPECT_SCALAR("?", 0, "0xFFEEBB");
            EXPECT_SCALAR("?", 0, "text");
            EXPECT_SCALAR("?", 0, "Pretty vector drawing.");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.25
		TEST UnorderedSets()
        {
            HANDLE(ex2_25);
            EXPECT_DOC_START();
            EXPECT_MAP_START("tag:yaml.org,2002:set", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_NULL(0);
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_NULL(0);
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_NULL(0);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.26
		TEST OrderedMappings()
        {
            HANDLE(ex2_26);
            EXPECT_DOC_START();
            EXPECT_SEQ_START("tag:yaml.org,2002:omap", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Mark McGwire");
            EXPECT_SCALAR("?", 0, "65");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Sammy Sosa");
            EXPECT_SCALAR("?", 0, "63");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Ken Griffey");
            EXPECT_SCALAR("?", 0, "58");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.27
		TEST Invoice()
        {
            HANDLE(ex2_27);
            EXPECT_DOC_START();
            EXPECT_MAP_START("tag:clarkevans.com,2002:invoice", 0);
            EXPECT_SCALAR("?", 0, "invoice");
            EXPECT_SCALAR("?", 0, "34843");
            EXPECT_SCALAR("?", 0, "date");
            EXPECT_SCALAR("?", 0, "2001-01-23");
            EXPECT_SCALAR("?", 0, "bill-to");
            EXPECT_MAP_START("?", 1);
            EXPECT_SCALAR("?", 0, "given");
            EXPECT_SCALAR("?", 0, "Chris");
            EXPECT_SCALAR("?", 0, "family");
            EXPECT_SCALAR("?", 0, "Dumars");
            EXPECT_SCALAR("?", 0, "address");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "lines");
            EXPECT_SCALAR("!", 0,
                          "458 Walkman Dr.\n"
                          "Suite #292\n");
            EXPECT_SCALAR("?", 0, "city");
            EXPECT_SCALAR("?", 0, "Royal Oak");
            EXPECT_SCALAR("?", 0, "state");
            EXPECT_SCALAR("?", 0, "MI");
            EXPECT_SCALAR("?", 0, "postal");
            EXPECT_SCALAR("?", 0, "48046");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_SCALAR("?", 0, "ship-to");
            EXPECT_ALIAS(1);
            EXPECT_SCALAR("?", 0, "product");
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sku");
            EXPECT_SCALAR("?", 0, "BL394D");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "4");
            EXPECT_SCALAR("?", 0, "description");
            EXPECT_SCALAR("?", 0, "Basketball");
            EXPECT_SCALAR("?", 0, "price");
            EXPECT_SCALAR("?", 0, "450.00");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sku");
            EXPECT_SCALAR("?", 0, "BL4438H");
            EXPECT_SCALAR("?", 0, "quantity");
            EXPECT_SCALAR("?", 0, "1");
            EXPECT_SCALAR("?", 0, "description");
            EXPECT_SCALAR("?", 0, "Super Hoop");
            EXPECT_SCALAR("?", 0, "price");
            EXPECT_SCALAR("?", 0, "2392.00");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "tax");
            EXPECT_SCALAR("?", 0, "251.42");
            EXPECT_SCALAR("?", 0, "total");
            EXPECT_SCALAR("?", 0, "4443.52");
            EXPECT_SCALAR("?", 0, "comments");
            EXPECT_SCALAR("?", 0, "Late afternoon is best. Backup contact is Nancy Billsmer @ 338-4338.");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 2.28
		TEST LogFile()
        {
            HANDLE(ex2_28);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Time");
            EXPECT_SCALAR("?", 0, "2001-11-23 15:01:42 -5");
            EXPECT_SCALAR("?", 0, "User");
            EXPECT_SCALAR("?", 0, "ed");
            EXPECT_SCALAR("?", 0, "Warning");
            EXPECT_SCALAR("?", 0, "This is an error message for the log file");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Time");
            EXPECT_SCALAR("?", 0, "2001-11-23 15:02:31 -5");
            EXPECT_SCALAR("?", 0, "User");
            EXPECT_SCALAR("?", 0, "ed");
            EXPECT_SCALAR("?", 0, "Warning");
            EXPECT_SCALAR("?", 0, "A slightly different error message.");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "Date");
            EXPECT_SCALAR("?", 0, "2001-11-23 15:03:17 -5");
            EXPECT_SCALAR("?", 0, "User");
            EXPECT_SCALAR("?", 0, "ed");
            EXPECT_SCALAR("?", 0, "Fatal");
            EXPECT_SCALAR("?", 0, "Unknown variable \"bar\"");
            EXPECT_SCALAR("?", 0, "Stack");
            EXPECT_SEQ_START("?", 0);
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "file");
            EXPECT_SCALAR("?", 0, "TopClass.py");
            EXPECT_SCALAR("?", 0, "line");
            EXPECT_SCALAR("?", 0, "23");
            EXPECT_SCALAR("?", 0, "code");
            EXPECT_SCALAR("!", 0, "x = MoreObject(\"345\\n\")\n");
            EXPECT_MAP_END();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "file");
            EXPECT_SCALAR("?", 0, "MoreClass.py");
            EXPECT_SCALAR("?", 0, "line");
            EXPECT_SCALAR("?", 0, "58");
            EXPECT_SCALAR("?", 0, "code");
            EXPECT_SCALAR("!", 0, "foo = bar");
            EXPECT_MAP_END();
            EXPECT_SEQ_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// TODO: 5.1 - 5.2 BOM
		
		// 5.3
		TEST BlockStructureIndicators()
        {
            HANDLE(ex5_3);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sequence");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "mapping");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sky");
            EXPECT_SCALAR("?", 0, "blue");
            EXPECT_SCALAR("?", 0, "sea");
            EXPECT_SCALAR("?", 0, "green");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.4
		TEST FlowStructureIndicators()
        {
            HANDLE(ex5_4);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sequence");
            EXPECT_SEQ_START("?", 0);
            EXPECT_SCALAR("?", 0, "one");
            EXPECT_SCALAR("?", 0, "two");
            EXPECT_SEQ_END();
            EXPECT_SCALAR("?", 0, "mapping");
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "sky");
            EXPECT_SCALAR("?", 0, "blue");
            EXPECT_SCALAR("?", 0, "sea");
            EXPECT_SCALAR("?", 0, "green");
            EXPECT_MAP_END();
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.5
		TEST CommentIndicator()
        {
            HANDLE(ex5_5);
            DONE();
        }
		
		// 5.6
		TEST NodePropertyIndicators()
        {
            HANDLE(ex5_6);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "anchored");
            EXPECT_SCALAR("!local", 1, "value");
            EXPECT_SCALAR("?", 0, "alias");
            EXPECT_ALIAS(1);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.7
		TEST BlockScalarIndicators()
        {
            HANDLE(ex5_7);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "literal");
            EXPECT_SCALAR("!", 0, "some\ntext\n");
            EXPECT_SCALAR("?", 0, "folded");
            EXPECT_SCALAR("!", 0, "some text\n");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.8
		TEST QuotedScalarIndicators()
        {
            HANDLE(ex5_8);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "single");
            EXPECT_SCALAR("!", 0, "text");
            EXPECT_SCALAR("?", 0, "double");
            EXPECT_SCALAR("!", 0, "text");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// TODO: 5.9 directive
		// TODO: 5.10 reserved indicator
		
		// 5.11
		TEST LineBreakCharacters()
        {
            HANDLE(ex5_11);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0,
                          "Line break (no glyph)\n"
                          "Line break (glyphed)\n");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.12
		TEST TabsAndSpaces()
        {
            HANDLE(ex5_12);
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "quoted");
            EXPECT_SCALAR("!", 0, "Quoted\t");
            EXPECT_SCALAR("?", 0, "block");
            EXPECT_SCALAR("!", 0,
                          "void main() {\n"
                          "\tprintf(\"Hello, world!\\n\");\n"
                          "}");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.13
		TEST EscapedCharacters()
        {
            HANDLE(ex5_13);
            EXPECT_DOC_START();
            EXPECT_SCALAR("!", 0, "Fun with \x5C \x22 \x07 \x08 \x1B \x0C \x0A \x0D \x09 \x0B " + std::string("\x00", 1) + " \x20 \xA0 \x85 \xe2\x80\xa8 \xe2\x80\xa9 A A A");
            EXPECT_DOC_END();
            DONE();
        }
		
		// 5.14
		TEST InvalidEscapedCharacters()
        {
            try {
                HANDLE(ex5_14);
            } catch(const YAML::ParserException& e) {
				YAML_ASSERT(e.msg == std::string(YAML::ErrorMsg::INVALID_ESCAPE) + "c");
				return true;
			}
            return "  no exception caught";
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
