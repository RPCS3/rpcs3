#include "yaml-cpp/emitter.h"
#include "emitterstate.h"
#include "emitterutils.h"
#include "indentation.h"
#include "yaml-cpp/exceptions.h"
#include <sstream>

namespace YAML
{	
	Emitter::Emitter(): m_pState(new EmitterState)
	{
	}
	
	Emitter::~Emitter()
	{
	}
	
	const char *Emitter::c_str() const
	{
		return m_stream.str();
	}
	
	unsigned Emitter::size() const
	{
		return m_stream.pos();
	}
	
	// state checking
	bool Emitter::good() const
	{
		return m_pState->good();
	}
	
	const std::string Emitter::GetLastError() const
	{
		return m_pState->GetLastError();
	}

	// global setters
	bool Emitter::SetOutputCharset(EMITTER_MANIP value)
	{
		return m_pState->SetOutputCharset(value, FmtScope::Global);
	}

	bool Emitter::SetStringFormat(EMITTER_MANIP value)
	{
		return m_pState->SetStringFormat(value, FmtScope::Global);
	}
	
	bool Emitter::SetBoolFormat(EMITTER_MANIP value)
	{
		bool ok = false;
		if(m_pState->SetBoolFormat(value, FmtScope::Global))
			ok = true;
		if(m_pState->SetBoolCaseFormat(value, FmtScope::Global))
			ok = true;
		if(m_pState->SetBoolLengthFormat(value, FmtScope::Global))
			ok = true;
		return ok;
	}
	
	bool Emitter::SetIntBase(EMITTER_MANIP value)
	{
		return m_pState->SetIntFormat(value, FmtScope::Global);
	}
	
	bool Emitter::SetSeqFormat(EMITTER_MANIP value)
	{
		return m_pState->SetFlowType(GroupType::Seq, value, FmtScope::Global);
	}
	
	bool Emitter::SetMapFormat(EMITTER_MANIP value)
	{
		bool ok = false;
		if(m_pState->SetFlowType(GroupType::Map, value, FmtScope::Global))
			ok = true;
		if(m_pState->SetMapKeyFormat(value, FmtScope::Global))
			ok = true;
		return ok;
	}
	
	bool Emitter::SetIndent(unsigned n)
	{
		return m_pState->SetIndent(n, FmtScope::Global);
	}
	
	bool Emitter::SetPreCommentIndent(unsigned n)
	{
		return m_pState->SetPreCommentIndent(n, FmtScope::Global);
	}
	
	bool Emitter::SetPostCommentIndent(unsigned n)
	{
		return m_pState->SetPostCommentIndent(n, FmtScope::Global);
	}
    
    bool Emitter::SetFloatPrecision(unsigned n)
    {
        return m_pState->SetFloatPrecision(n, FmtScope::Global);
    }

    bool Emitter::SetDoublePrecision(unsigned n)
    {
        return m_pState->SetDoublePrecision(n, FmtScope::Global);
    }

	// SetLocalValue
	// . Either start/end a group, or set a modifier locally
	Emitter& Emitter::SetLocalValue(EMITTER_MANIP value)
	{
		if(!good())
			return *this;
		
		switch(value) {
			case BeginDoc:
				EmitBeginDoc();
				break;
			case EndDoc:
				EmitEndDoc();
				break;
			case BeginSeq:
				EmitBeginSeq();
				break;
			case EndSeq:
				EmitEndSeq();
				break;
			case BeginMap:
				EmitBeginMap();
				break;
			case EndMap:
				EmitEndMap();
				break;
			case Key:
			case Value:
                // deprecated (these can be deduced by the parity of nodes in a map)
				break;
			case TagByKind:
				EmitKindTag();
				break;
			case Newline:
				EmitNewline();
				break;
			default:
				m_pState->SetLocalValue(value);
				break;
		}
		return *this;
	}
	
	Emitter& Emitter::SetLocalIndent(const _Indent& indent)
	{
		m_pState->SetIndent(indent.value, FmtScope::Local);
		return *this;
	}

    Emitter& Emitter::SetLocalPrecision(const _Precision& precision)
    {
        if(precision.floatPrecision >= 0)
            m_pState->SetFloatPrecision(precision.floatPrecision, FmtScope::Local);
        if(precision.doublePrecision >= 0)
            m_pState->SetDoublePrecision(precision.doublePrecision, FmtScope::Local);
        return *this;
    }

	// EmitBeginDoc
	void Emitter::EmitBeginDoc()
	{
		if(!good())
			return;
        
        if(m_pState->CurGroupType() != GroupType::None) {
			m_pState->SetError("Unexpected begin document");
			return;
        }
        
        if(m_pState->HasAnchor() || m_pState->HasTag()) {
			m_pState->SetError("Unexpected begin document");
			return;
        }
        
        if(m_stream.col() > 0)
            m_stream << "\n";
        m_stream << "---\n";
	}
	
	// EmitEndDoc
	void Emitter::EmitEndDoc()
	{
		if(!good())
			return;
        
        if(m_pState->CurGroupType() != GroupType::None) {
			m_pState->SetError("Unexpected begin document");
			return;
        }
        
        if(m_pState->HasAnchor() || m_pState->HasTag()) {
			m_pState->SetError("Unexpected begin document");
			return;
        }
        
        if(m_stream.col() > 0)
            m_stream << "\n";
        m_stream << "...\n";
	}

	// EmitBeginSeq
	void Emitter::EmitBeginSeq()
	{
		if(!good())
			return;
        
        PrepareNode();
        
        m_pState->BeginGroup(GroupType::Seq);
	}
	
	// EmitEndSeq
	void Emitter::EmitEndSeq()
	{
		if(!good())
			return;
        
        m_pState->EndGroup(GroupType::Seq);
	}
	
	// EmitBeginMap
	void Emitter::EmitBeginMap()
	{
		if(!good())
			return;

        PrepareNode();
        
        m_pState->BeginGroup(GroupType::Map);
	}
	
	// EmitEndMap
	void Emitter::EmitEndMap()
	{
		if(!good())
			return;

        m_pState->EndGroup(GroupType::Map);
    }

	// EmitNewline
	void Emitter::EmitNewline()
	{
		if(!good())
			return;
	}

	bool Emitter::CanEmitNewline() const
	{
        return false;
	}

    // Put the stream in a state so we can simply write the next node
    // E.g., if we're in a sequence, write the "- "
    void Emitter::PrepareNode()
    {
        switch(m_pState->CurGroupType()) {
            case GroupType::None:
                PrepareTopNode();
                break;
            case GroupType::Seq:
                switch(m_pState->CurGroupFlowType()) {
                    case FlowType::Flow:
                        FlowSeqPrepareNode();
                        break;
                    case FlowType::Block:
                        BlockSeqPrepareNode();
                        break;
                    case FlowType::None:
                        assert(false);
                }
                break;
            case GroupType::Map:
                switch(m_pState->CurGroupFlowType()) {
                    case FlowType::Flow:
                        FlowMapPrepareNode();
                        break;
                    case FlowType::Block:
                        BlockMapPrepareNode();
                        break;
                    case FlowType::None:
                        assert(false);
                }
                break;
        }
    }
    
    void Emitter::PrepareTopNode()
    {
        const bool hasAnchor = m_pState->HasAnchor();
        const bool hasTag = m_pState->HasTag();
        
        if(!hasAnchor && !hasTag && m_stream.pos() > 0) {
            EmitBeginDoc();
        }
        
        // TODO: if we were writing null, and
        // we wanted it blank, we wouldn't want a space
        if(hasAnchor || hasTag)
            m_stream << " ";
    }
    
    void Emitter::FlowSeqPrepareNode()
    {
    }

    void Emitter::BlockSeqPrepareNode()
    {
    }
    
    void Emitter::FlowMapPrepareNode()
    {
    }

    void Emitter::BlockMapPrepareNode()
    {
    }

	// *******************************************************************************************
	// overloads of Write
	
	Emitter& Emitter::Write(const std::string& str)
	{
		if(!good())
			return *this;
        
        PrepareNode();
        
		const bool escapeNonAscii = m_pState->GetOutputCharset() == EscapeNonAscii;
        const StringFormat::value strFormat = Utils::ComputeStringFormat(str, m_pState->GetStringFormat(), m_pState->CurGroupFlowType(), escapeNonAscii);
        
        switch(strFormat) {
            case StringFormat::Plain:
                m_stream << str;
                break;
            case StringFormat::SingleQuoted:
                Utils::WriteSingleQuotedString(m_stream, str);
                break;
            case StringFormat::DoubleQuoted:
                Utils::WriteDoubleQuotedString(m_stream, str, escapeNonAscii);
                break;
            case StringFormat::Literal:
                Utils::WriteLiteralString(m_stream, str, m_pState->CurIndent() + m_pState->GetIndent());
                break;
        }

        m_pState->BeginScalar();
        
		return *this;
	}

    unsigned Emitter::GetFloatPrecision() const
    {
        return m_pState->GetFloatPrecision();
    }
    
    unsigned Emitter::GetDoublePrecision() const
    {
        return m_pState->GetDoublePrecision();
    }

	const char *Emitter::ComputeFullBoolName(bool b) const
	{
		const EMITTER_MANIP mainFmt = (m_pState->GetBoolLengthFormat() == ShortBool ? YesNoBool : m_pState->GetBoolFormat());
		const EMITTER_MANIP caseFmt = m_pState->GetBoolCaseFormat();
		switch(mainFmt) {
			case YesNoBool:
				switch(caseFmt) {
					case UpperCase: return b ? "YES" : "NO";
					case CamelCase: return b ? "Yes" : "No";
					case LowerCase: return b ? "yes" : "no";
					default: break;
				}
				break;
			case OnOffBool:
				switch(caseFmt) {
					case UpperCase: return b ? "ON" : "OFF";
					case CamelCase: return b ? "On" : "Off";
					case LowerCase: return b ? "on" : "off";
					default: break;
				}
				break;
			case TrueFalseBool:
				switch(caseFmt) {
					case UpperCase: return b ? "TRUE" : "FALSE";
					case CamelCase: return b ? "True" : "False";
					case LowerCase: return b ? "true" : "false";
					default: break;
				}
				break;
			default:
				break;
		}
		return b ? "y" : "n"; // should never get here, but it can't hurt to give these answers
	}

	Emitter& Emitter::Write(bool b)
	{
		if(!good())
			return *this;

        m_pState->BeginScalar();

		return *this;
	}

	Emitter& Emitter::Write(char ch)
	{
		if(!good())
			return *this;

        m_pState->BeginScalar();

		return *this;
	}

	Emitter& Emitter::Write(const _Alias& alias)
	{
		if(!good())
			return *this;

        m_pState->BeginScalar();

		return *this;
	}
	
	Emitter& Emitter::Write(const _Anchor& anchor)
	{
		if(!good())
			return *this;

        m_pState->BeginScalar();

		return *this;
	}
	
	Emitter& Emitter::Write(const _Tag& tag)
	{
		if(!good())
			return *this;

        m_pState->BeginScalar();
        
		return *this;
	}

	void Emitter::EmitKindTag()
	{
		Write(LocalTag(""));
	}

	Emitter& Emitter::Write(const _Comment& comment)
	{
		if(!good())
			return *this;

        m_pState->BeginScalar();

		return *this;
	}

	Emitter& Emitter::Write(const _Null& /*null*/)
	{
		if(!good())
			return *this;

        m_pState->BeginScalar();

		return *this;
	}

	Emitter& Emitter::Write(const Binary& binary)
	{
		Write(SecondaryTag("binary"));

		if(!good())
			return *this;

        m_pState->BeginScalar();

		return *this;
	}
}

