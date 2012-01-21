#ifndef EMITTER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define EMITTER_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/binary.h"
#include "yaml-cpp/emittermanip.h"
#include "yaml-cpp/ostream.h"
#include "yaml-cpp/noncopyable.h"
#include "yaml-cpp/null.h"
#include <memory>
#include <string>
#include <sstream>

namespace YAML
{
	class EmitterState;
	
	class YAML_CPP_API Emitter: private noncopyable
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
        bool SetFloatPrecision(unsigned n);
        bool SetDoublePrecision(unsigned n);
		
		// local setters
		Emitter& SetLocalValue(EMITTER_MANIP value);
		Emitter& SetLocalIndent(const _Indent& indent);
        Emitter& SetLocalPrecision(const _Precision& precision);
		
		// overloads of write
		Emitter& Write(const std::string& str);
		Emitter& Write(bool b);
		Emitter& Write(char ch);
		Emitter& Write(const _Alias& alias);
		Emitter& Write(const _Anchor& anchor);
		Emitter& Write(const _Tag& tag);
		Emitter& Write(const _Comment& comment);
		Emitter& Write(const _Null& null);
		Emitter& Write(const Binary& binary);
		
		template <typename T>
		Emitter& WriteIntegralType(T value);
		
		template <typename T>
		Emitter& WriteStreamable(T value);

	private:
		void PreWriteIntegralType(std::stringstream& str);
		void PreWriteStreamable(std::stringstream& str);
		void PostWriteIntegralType(const std::stringstream& str);
		void PostWriteStreamable(const std::stringstream& str);
        
        template<typename T> void SetStreamablePrecision(std::stringstream&) {}
        unsigned GetFloatPrecision() const;
        unsigned GetDoublePrecision() const;
	
	private:
		void PreAtomicWrite();
		bool GotoNextPreAtomicState();
		void PostAtomicWrite();
		void EmitSeparationIfNecessary();
		
		void EmitBeginDoc();
		void EmitEndDoc();
		void EmitBeginSeq();
		void EmitEndSeq();
		void EmitBeginMap();
		void EmitEndMap();
		void EmitKey();
		void EmitValue();
		void EmitNewline();
		void EmitKindTag();
		void EmitTag(bool verbatim, const _Tag& tag);
		
		const char *ComputeFullBoolName(bool b) const;
		bool CanEmitNewline() const;
		
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
		
		std::stringstream str;
		PreWriteStreamable(str);
        SetStreamablePrecision<T>(str);
		str << value;
		PostWriteStreamable(str);
		return *this;
	}
	
    template<>
    inline void Emitter::SetStreamablePrecision<float>(std::stringstream& str)
    {
		str.precision(GetFloatPrecision());
    }

    template<>
    inline void Emitter::SetStreamablePrecision<double>(std::stringstream& str)
    {
		str.precision(GetDoublePrecision());
    }

	// overloads of insertion
	inline Emitter& operator << (Emitter& emitter, const std::string& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, bool v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, char v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, unsigned char v) { return emitter.Write(static_cast<char>(v)); }
	inline Emitter& operator << (Emitter& emitter, const _Alias& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const _Anchor& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const _Tag& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const _Comment& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const _Null& v) { return emitter.Write(v); }
	inline Emitter& operator << (Emitter& emitter, const Binary& b) { return emitter.Write(b); }

	inline Emitter& operator << (Emitter& emitter, const char *v) { return emitter.Write(std::string(v)); }

	inline Emitter& operator << (Emitter& emitter, int v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, unsigned int v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, short v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, unsigned short v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, long v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, unsigned long v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, long long v) { return emitter.WriteIntegralType(v); }
	inline Emitter& operator << (Emitter& emitter, unsigned long long v) { return emitter.WriteIntegralType(v); }

	inline Emitter& operator << (Emitter& emitter, float v) { return emitter.WriteStreamable(v); }
	inline Emitter& operator << (Emitter& emitter, double v) { return emitter.WriteStreamable(v); }

	inline Emitter& operator << (Emitter& emitter, EMITTER_MANIP value) {
		return emitter.SetLocalValue(value);
	}
	
	inline Emitter& operator << (Emitter& emitter, _Indent indent) {
		return emitter.SetLocalIndent(indent);
	}
    
    inline Emitter& operator << (Emitter& emitter, _Precision precision) {
        return emitter.SetLocalPrecision(precision);
    }
}

#endif // EMITTER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
