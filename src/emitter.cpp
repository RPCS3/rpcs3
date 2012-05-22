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
        
        PrepareNode(m_pState->NextGroupType(GroupType::Seq));
        
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

        PrepareNode(m_pState->NextGroupType(GroupType::Map));
        
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
    void Emitter::PrepareNode(EmitterNodeType::value child)
    {
        switch(m_pState->CurGroupNodeType()) {
            case EmitterNodeType::None:
                PrepareTopNode(child);
                break;
            case EmitterNodeType::FlowSeq:
                FlowSeqPrepareNode(child);
                break;
            case EmitterNodeType::BlockSeq:
                BlockSeqPrepareNode(child);
                break;
            case EmitterNodeType::FlowMap:
                FlowMapPrepareNode(child);
                break;
            case EmitterNodeType::BlockMap:
                BlockMapPrepareNode(child);
                break;
            case EmitterNodeType::Scalar:
                assert(false);
                break;
        }
    }
    
    void Emitter::PrepareTopNode(EmitterNodeType::value child)
    {
        if(!m_pState->HasBegunNode() && m_stream.pos() > 0) {
            EmitBeginDoc();
        }
        
        switch(child) {
            case EmitterNodeType::None:
            case EmitterNodeType::Scalar:
            case EmitterNodeType::FlowSeq:
            case EmitterNodeType::FlowMap:
                // TODO: if we were writing null, and
                // we wanted it blank, we wouldn't want a space
                if(m_pState->HasBegunNode())
                    m_stream << " ";
                break;
            case EmitterNodeType::BlockSeq:
            case EmitterNodeType::BlockMap:
                if(m_pState->HasBegunNode())
                    m_stream << "\n";
                break;
        }
    }
    
    void Emitter::FlowSeqPrepareNode(EmitterNodeType::value child)
    {
    }

    void Emitter::BlockSeqPrepareNode(EmitterNodeType::value child)
    {
        const unsigned curIndent = m_pState->CurIndent();
        const unsigned nextIndent = curIndent + m_pState->CurGroupIndent();
        
        if(!m_pState->HasBegunNode()) {
            if(m_pState->CurGroupChildCount() > 0) {
                m_stream << "\n";
            }
            m_stream << IndentTo(curIndent);
            m_stream << "-";
        }
        
        switch(child) {
            case EmitterNodeType::None:
            case EmitterNodeType::Scalar:
            case EmitterNodeType::FlowSeq:
            case EmitterNodeType::FlowMap:
                if(m_stream.col() < nextIndent)
                    m_stream << IndentTo(nextIndent);
                else
                    m_stream << " ";
                break;
            case EmitterNodeType::BlockSeq:
                m_stream << "\n";
                break;
            case EmitterNodeType::BlockMap:
                if(m_pState->HasBegunNode())
                    m_stream << "\n";
                break;
        }
    }
    
    void Emitter::FlowMapPrepareNode(EmitterNodeType::value child)
    {
    }

    void Emitter::BlockMapPrepareNode(EmitterNodeType::value child)
    {
        const unsigned curIndent = m_pState->CurIndent();
        const unsigned nextIndent = curIndent + m_pState->CurGroupIndent();
        const std::size_t childCount = m_pState->CurGroupChildCount();

        if(!m_pState->HasBegunNode()) {
            if(childCount % 2 == 0) {
                // key
                if(childCount > 0) {
                    m_stream << "\n";
                }
                if(false /* long key */) {
                }
            } else {
                // value
                if(false /* was long key */) {
                } else {
                    m_stream << ":";
                }
            }
        }
        
        if(childCount % 2 == 0) {
            // key
            switch(child) {
                case EmitterNodeType::None:
                case EmitterNodeType::Scalar:
                case EmitterNodeType::FlowSeq:
                case EmitterNodeType::FlowMap:
                    if(m_stream.col() < curIndent)
                        m_stream << IndentTo(curIndent);
                    else
                        m_stream << " ";
                    break;
                case EmitterNodeType::BlockSeq:
                case EmitterNodeType::BlockMap:
                    break;
            }
        } else {
            // value
            switch(child) {
                case EmitterNodeType::None:
                case EmitterNodeType::Scalar:
                case EmitterNodeType::FlowSeq:
                case EmitterNodeType::FlowMap:
                    if(m_stream.col() < nextIndent)
                        m_stream << IndentTo(nextIndent);
                    else
                        m_stream << " ";
                    break;
                case EmitterNodeType::BlockSeq:
                case EmitterNodeType::BlockMap:
                    m_stream << "\n";
                    break;
            }
        }
    }

	// *******************************************************************************************
	// overloads of Write
	
	Emitter& Emitter::Write(const std::string& str)
	{
		if(!good())
			return *this;
        
        PrepareNode(EmitterNodeType::Scalar);
        
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
        
        if(m_pState->HasAnchor()) {
            m_pState->SetError(ErrorMsg::INVALID_ANCHOR);
            return *this;
        }

        PrepareNode(EmitterNodeType::None);

		if(!Utils::WriteAnchor(m_stream, anchor.content)) {
			m_pState->SetError(ErrorMsg::INVALID_ANCHOR);
			return *this;
		}
        
        m_pState->SetAnchor();

		return *this;
	}
	
	Emitter& Emitter::Write(const _Tag& tag)
	{
		if(!good())
			return *this;
        
        if(m_pState->HasTag()) {
            m_pState->SetError(ErrorMsg::INVALID_TAG);
            return *this;
        }
        
        PrepareNode(EmitterNodeType::None);
        
		bool success = false;
		if(tag.type == _Tag::Type::Verbatim)
			success = Utils::WriteTag(m_stream, tag.content, true);
		else if(tag.type == _Tag::Type::PrimaryHandle)
			success = Utils::WriteTag(m_stream, tag.content, false);
		else
			success = Utils::WriteTagWithPrefix(m_stream, tag.prefix, tag.content);
		
		if(!success) {
			m_pState->SetError(ErrorMsg::INVALID_TAG);
			return *this;
		}

        m_pState->SetTag();

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

