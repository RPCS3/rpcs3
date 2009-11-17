#pragma once

#ifndef EMITTER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define EMITTER_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "emittermanip.h"
#include "ostream.h"
#include "null.h"
#include <memory>
#include <string>
#include <sstream>

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
		bool SetOutputCharset(EMITTER_MANIP value);
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
		Emitter& Write(bool b);
		Emitter& Write(const _Alias& alias);
		Emitter& Write(const _Anchor& anchor);
		Emitter& Write(const _Tag& tag);
		Emitter& Write(const _Comment& comment);
		Emitter& Write(const _Null& null);
		
		template <typename T>
		Emitter& WriteIntegralType(T value);
		
		template <typename T>
		Emitter& WriteStreamable(T value);

	private:
		void PreWriteIntegralType(std::stringstream& str);
		void PostWriteIntegralType(const std::stringstream& str);
	
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
	
	template <typename T>
	inline Emitter& Emitter::WriteIntegralType(T value)
	{
		if(!good())
			return *this;
		
		std::stringstream str;
		PreWriteIntegralType(str);
		str << value;
		PostWriteIntegralType(str);
		return *this;
	}

	template <typename T>
	inline Emitter& Emitter::WriteStreamable(T value)
	{
		if(!good())
			return *this;
		
		PreAtomicWrite();
		EmitSeparationIfNecessary();
		
		std::stringstream str;
		str << value;
		m_stream << str.str();
		
		PostAtomicWrite();
		return *this;
	}
	
	// overloads of insertion
	inline Emitter& operator << (Emitter& emitter, const std::string& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, bool v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const _Alias& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const _Anchor& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const _Tag& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const _Comment& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const _Null& v) { return emitter.Write(v); }

	inline Emitter& operator << (Emitter& emitter, const char *v) { return emitter.Write(std::string(v)); }

	inline Emitter& operator << (Emitter& emitter, int v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, unsigned int v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, short v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, unsigned short v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, long v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, unsigned long v) { return emitter.WriteIntegralType(v); }

	inline Emitter& operator << (Emitter& emitter, float v) { return emitter.WriteStreamable(v); }
	inline Emitter& operator << (Emitter& emitter, double v) { return emitter.WriteStreamable(v); }

	inline Emitter& operator << (Emitter& emitter, EMITTER_MANIP value) {
		return emitter.SetLocalValue(value);
	}
	
	inline Emitter& operator << (Emitter& emitter, _Indent indent) {
		return emitter.SetLocalIndent(indent);
	}
}

#endif // EMITTER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
