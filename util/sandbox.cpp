#include "yaml-cpp/yaml.h"
#include "yaml-cpp/eventhandler.h"
#include <iostream>

class NullEventHandler: public YAML::EventHandler
{
public:
    typedef YAML::Mark Mark;
    typedef YAML::anchor_t anchor_t;
    
    NullEventHandler() {}
    
    virtual void OnDocumentStart(const Mark&) {}
    virtual void OnDocumentEnd() {}
    virtual void OnNull(const Mark&, anchor_t) {}
    virtual void OnAlias(const Mark&, anchor_t) {}
    virtual void OnScalar(const Mark&, const std::string&, anchor_t, const std::string&) {}
    virtual void OnSequenceStart(const Mark&, const std::string&, anchor_t) {}
    virtual void OnSequenceEnd() {}
    virtual void OnMapStart(const Mark&, const std::string&, anchor_t) {}
    virtual void OnMapEnd() {}
};

int main()
{
    std::stringstream stream("---{header: {id: 1");
    YAML::Parser parser(stream);
//    parser.PrintTokens(std::cout);
    NullEventHandler handler;
    parser.HandleNextDocument(handler);
    return 0;
}
