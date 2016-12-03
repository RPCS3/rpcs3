#include "yaml-cpp/exceptions.h"

namespace YAML {

// These destructors are defined out-of-line so the vtable is only emitted once.
Exception::~Exception() noexcept {}
ParserException::~ParserException() noexcept {}
RepresentationException::~RepresentationException() noexcept {}
InvalidScalar::~InvalidScalar() noexcept {}
KeyNotFound::~KeyNotFound() noexcept {}
InvalidNode::~InvalidNode() noexcept {}
BadConversion::~BadConversion() noexcept {}
BadDereference::~BadDereference() noexcept {}
BadSubscript::~BadSubscript() noexcept {}
BadPushback::~BadPushback() noexcept {}
BadInsert::~BadInsert() noexcept {}
EmitterException::~EmitterException() noexcept {}
BadFile::~BadFile() noexcept {}
}
