#pragma once

#include "emittermanip.h"
#include "ostream.h"
#include <memory>
#include <string>

namespace YAML
{
	class EmitterState;
	
	class Emitter
	{
	public:
		Emitter();
		~Emitter();
		
		// output
		const char *c_str() const;
		unsigned size() const;
		
		// state checking
		bool good() const;
		const std::string GetLastError() const;
		
		// global setters
		bool SetStringFormat(EMITTER_MANIP value);
		bool SetBoolFormat(EMITTER_MANIP value);
		bool SetIntBase(EMITTER_MANIP value);
		bool SetSeqFormat(EMITTER_MANIP value);
		bool SetMapFormat(EMITTER_MANIP value);
		bool SetIndent(unsigned n);
		bool SetPreCommentIndent(unsigned n);
		bool SetPostCommentIndent(unsigned n);
		
		// local setters
		Emitter& SetLocalValue(EMITTER_MANIP value);
		Emitter& SetLocalIndent(const _Indent& indent);
		
		// overloads of write
		Emitter& Write(const std::string& str);
		Emitter& Write(const char *str);
		Emitter& Write(int i);
		Emitter& Write(bool b);
		Emitter& Write(float f);
		Emitter& Write(double d);
		Emitter& Write(const _Alias& alias);
		Emitter& Write(const _Anchor& anchor);
		Emitter& Write(const _Comment& comment);
		
	private:
		enum ATOMIC_TYPE { AT_SCALAR, AT_SEQ, AT_BLOCK_SEQ, AT_FLOW_SEQ, AT_MAP, AT_BLOCK_MAP, AT_FLOW_MAP };
		
		void PreAtomicWrite();
		bool GotoNextPreAtomicState();
		void PostAtomicWrite();
		void EmitSeparationIfNecessary();
		
		void EmitBeginSeq();
		void EmitEndSeq();
		void EmitBeginMap();
		void EmitEndMap();
		void EmitKey();
		void EmitValue();
		
	private:
		ostream m_stream;
		std::auto_ptr <EmitterState> m_pState;
	};
	
	// overloads of insertion
	template <typename T>
	inline Emitter& operator << (Emitter& emitter, T v) {
		return emitter.Write(v);
	}

	template <>
	inline Emitter& operator << (Emitter& emitter, EMITTER_MANIP value) {
		return emitter.SetLocalValue(value);
	}
	
	template <>
	inline Emitter& operator << (Emitter& emitter, _Indent indent) {
		return emitter.SetLocalIndent(indent);
	}
}
