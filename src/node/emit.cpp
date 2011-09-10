#include "yaml-cpp/value/emit.h"
#include "yaml-cpp/emitfromevents.h"
#include "yaml-cpp/emitter.h"
#include "valueevents.h"

namespace YAML
{
	Emitter& operator << (Emitter& out, const Value& value)
	{
		EmitFromEvents emitFromEvents(out);
		ValueEvents events(value);
		events.Emit(emitFromEvents);
		return out;
	}
	
	std::ostream& operator << (std::ostream& out, const Value& value)
	{
		Emitter emitter;
		emitter << value;
		out << emitter.c_str();
		return out;
	}
}
