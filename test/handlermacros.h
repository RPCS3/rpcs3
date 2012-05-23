#include "teststruct.h"
#pragma once

#include "yaml-cpp/yaml.h"
#include "yaml-cpp/eventhandler.h"
#include <string>
#include <cassert>

namespace Test {
    inline std::string Quote(const std::string& text) {
        YAML::Emitter out;
        out << YAML::DoubleQuoted << text;
        return out.c_str();
    }
    
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
                    return out << "Scalar(" << Quote(tag) << ", " << anchor << ", " << Quote(scalar) << ")";
                case SeqStart:
                    return out << "SeqStart(" << Quote(tag) << ", " << anchor << ")";
                case SeqEnd:
                    return out << "SeqEnd";
                case MapStart:
                    return out << "MapStart(" << Quote(tag) << ", " << anchor << ")";
                case MapEnd:
                    return out << "MapEnd";
            }
            assert(false);
            return out;
        }
    };
    
    inline std::ostream& operator << (std::ostream& out, const Event& event) {
        return event.write(out);
    }
    
    inline bool operator == (const Event& a, const Event& b) {
        return a.type == b.type && a.tag == b.tag && a.anchor == b.anchor && a.scalar == b.scalar;
    }
    
    inline bool operator != (const Event& a, const Event& b) {
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
                        out << "  " << m_expectedEvents[j] << "\n";
                    }
                    out << "  EXPECTED: (no event expected)\n";
                    out << "  ACTUAL  : " << m_actualEvents[i] << "\n";
                    return out.str().c_str();
                }
                
                if(i >= m_actualEvents.size()) {
                    std::stringstream out;
                    for(std::size_t j=0;j<i;j++) {
                        out << "  " << m_expectedEvents[j] << "\n";
                    }
                    out << "  EXPECTED: " << m_expectedEvents[i] << "\n";
                    out << "  ACTUAL  : (no event recorded)\n";
                    return out.str().c_str();
                }
                
                if(m_expectedEvents[i] != m_actualEvents[i]) {
                    std::stringstream out;
                    for(std::size_t j=0;j<i;j++) {
                        out << "  " <<  m_expectedEvents[j] << "\n";
                    }
                    out << "  EXPECTED: " << m_expectedEvents[i] << "\n";
                    out << "  ACTUAL  : " << m_actualEvents[i] << "\n";
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
while(parser.HandleNextDocument(handler)) {}

#define EXPECT_DOC_START()\
handler.Expect(Event(Event::DocStart, "", 0, ""))
    
#define EXPECT_DOC_END()\
handler.Expect(Event(Event::DocEnd, "", 0, ""))
    
#define EXPECT_NULL(anchor)\
handler.Expect(Event(Event::Null, "", anchor, ""))
    
#define EXPECT_ALIAS(anchor)\
handler.Expect(Event(Event::Alias, "", anchor, ""))
    
#define EXPECT_SCALAR(tag, anchor, value)\
handler.Expect(Event(Event::Scalar, tag, anchor, value))
    
#define EXPECT_SEQ_START(tag, anchor)\
handler.Expect(Event(Event::SeqStart, tag, anchor, ""))
    
#define EXPECT_SEQ_END()\
handler.Expect(Event(Event::SeqEnd, "", 0, ""))
    
#define EXPECT_MAP_START(tag, anchor)\
handler.Expect(Event(Event::MapStart, tag, anchor, ""))
    
#define EXPECT_MAP_END()\
handler.Expect(Event(Event::MapEnd, "", 0, ""))
    
#define DONE()\
return handler.Check()
    
}
