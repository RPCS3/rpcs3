#include "emitterstate.h"
#include "exceptions.h"

namespace YAML
{
	EmitterState::EmitterState(): m_isGood(true), m_curIndent(0), m_requiresSeparation(false)
	{
		// start up
		m_stateStack.push(ES_WAITING_FOR_DOC);
		
		// set default global manipulators
		m_charset.set(EmitNonAscii);
		m_strFmt.set(Auto);
		m_boolFmt.set(TrueFalseBool);
		m_boolLengthFmt.set(LongBool);
		m_boolCaseFmt.set(LowerCase);
		m_intFmt.set(Dec);
		m_indent.set(2);
		m_preCommentIndent.set(2);
		m_postCommentIndent.set(1);
		m_seqFmt.set(Block);
		m_mapFmt.set(Block);
		m_mapKeyFmt.set(Auto);
	}
	
	EmitterState::~EmitterState()
	{
		while(!m_groups.empty())
			_PopGroup();
	}
	
	std::auto_ptr <EmitterState::Group> EmitterState::_PopGroup()
	{
		if(m_groups.empty())
			return std::auto_ptr <Group> (0);
		
		std::auto_ptr <Group> pGroup(m_groups.top());
		m_groups.pop();
		return pGroup;
	}

	// SetLocalValue
	// . We blindly tries to set all possible formatters to this value
	// . Only the ones that make sense will be accepted
	void EmitterState::SetLocalValue(EMITTER_MANIP value)
	{
		SetOutputCharset(value, LOCAL);
		SetStringFormat(value, LOCAL);
		SetBoolFormat(value, LOCAL);
		SetBoolCaseFormat(value, LOCAL);
		SetBoolLengthFormat(value, LOCAL);
		SetIntFormat(value, LOCAL);
		SetFlowType(GT_SEQ, value, LOCAL);
		SetFlowType(GT_MAP, value, LOCAL);
		SetMapKeyFormat(value, LOCAL);
	}
	
	void EmitterState::BeginGroup(GROUP_TYPE type)
	{
		unsigned lastIndent = (m_groups.empty() ? 0 : m_groups.top()->indent);
		m_curIndent += lastIndent;
		
		std::auto_ptr <Group> pGroup(new Group(type));
		
		// transfer settings (which last until this group is done)
		pGroup->modifiedSettings = m_modifiedSettings;

		// set up group
		pGroup->flow = GetFlowType(type);
		pGroup->indent = GetIndent();
		pGroup->usingLongKey = (GetMapKeyFormat() == LongKey ? true : false);

		m_groups.push(pGroup.release());
	}
	
	void EmitterState::EndGroup(GROUP_TYPE type)
	{
		if(m_groups.empty())
			return SetError(ErrorMsg::UNMATCHED_GROUP_TAG);
		
		// get rid of the current group
		{
			std::auto_ptr <Group> pFinishedGroup = _PopGroup();
			if(pFinishedGroup->type != type)
				return SetError(ErrorMsg::UNMATCHED_GROUP_TAG);
		}

		// reset old settings
		unsigned lastIndent = (m_groups.empty() ? 0 : m_groups.top()->indent);
		assert(m_curIndent >= lastIndent);
		m_curIndent -= lastIndent;
		
		// some global settings that we changed may have been overridden
		// by a local setting we just popped, so we need to restore them
		m_globalModifiedSettings.restore();
	}
		
	GROUP_TYPE EmitterState::GetCurGroupType() const
	{
		if(m_groups.empty())
			return GT_NONE;
		
		return m_groups.top()->type;
	}
	
	FLOW_TYPE EmitterState::GetCurGroupFlowType() const
	{
		if(m_groups.empty())
			return FT_NONE;
		
		return (m_groups.top()->flow == Flow ? FT_FLOW : FT_BLOCK);
	}
	
	bool EmitterState::CurrentlyInLongKey()
	{
		if(m_groups.empty())
			return false;
		return m_groups.top()->usingLongKey;
	}
	
	void EmitterState::StartLongKey()
	{
		if(!m_groups.empty())
			m_groups.top()->usingLongKey = true;
	}
	
	void EmitterState::StartSimpleKey()
	{
		if(!m_groups.empty())
			m_groups.top()->usingLongKey = false;
	}

	void EmitterState::ClearModifiedSettings()
	{
		m_modifiedSettings.clear();
	}

	bool EmitterState::SetOutputCharset(EMITTER_MANIP value, FMT_SCOPE scope)
	{
		switch(value) {
			case EmitNonAscii:
			case EscapeNonAscii:
				_Set(m_charset, value, scope);
				return true;
			default:
				return false;
		}
	}
	
	bool EmitterState::SetStringFormat(EMITTER_MANIP value, FMT_SCOPE scope)
	{
		switch(value) {
			case Auto:
			case SingleQuoted:
			case DoubleQuoted:
			case Literal:
				_Set(m_strFmt, value, scope);
				return true;
			default:
				return false;
		}
	}
	
	bool EmitterState::SetBoolFormat(EMITTER_MANIP value, FMT_SCOPE scope)
	{
		switch(value) {
			case OnOffBool:
			case TrueFalseBool:
			case YesNoBool:
				_Set(m_boolFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	bool EmitterState::SetBoolLengthFormat(EMITTER_MANIP value, FMT_SCOPE scope)
	{
		switch(value) {
			case LongBool:
			case ShortBool:
				_Set(m_boolLengthFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	bool EmitterState::SetBoolCaseFormat(EMITTER_MANIP value, FMT_SCOPE scope)
	{
		switch(value) {
			case UpperCase:
			case LowerCase:
			case CamelCase:
				_Set(m_boolCaseFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	bool EmitterState::SetIntFormat(EMITTER_MANIP value, FMT_SCOPE scope)
	{
		switch(value) {
			case Dec:
			case Hex:
			case Oct:
				_Set(m_intFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	bool EmitterState::SetIndent(unsigned value, FMT_SCOPE scope)
	{
		if(value == 0)
			return false;
		
		_Set(m_indent, value, scope);
		return true;
	}

	bool EmitterState::SetPreCommentIndent(unsigned value, FMT_SCOPE scope)
	{
		if(value == 0)
			return false;
		
		_Set(m_preCommentIndent, value, scope);
		return true;
	}
	
	bool EmitterState::SetPostCommentIndent(unsigned value, FMT_SCOPE scope)
	{
		if(value == 0)
			return false;
		
		_Set(m_postCommentIndent, value, scope);
		return true;
	}

	bool EmitterState::SetFlowType(GROUP_TYPE groupType, EMITTER_MANIP value, FMT_SCOPE scope)
	{
		switch(value) {
			case Block:
			case Flow:
				_Set(groupType == GT_SEQ ? m_seqFmt : m_mapFmt, value, scope);
				return true;
			default:
				return false;
		}
	}

	EMITTER_MANIP EmitterState::GetFlowType(GROUP_TYPE groupType) const
	{
		// force flow style if we're currently in a flow
		FLOW_TYPE flowType = GetCurGroupFlowType();
		if(flowType == FT_FLOW)
			return Flow;
		
		// otherwise, go with what's asked of use
		return (groupType == GT_SEQ ? m_seqFmt.get() : m_mapFmt.get());
	}
	
	bool EmitterState::SetMapKeyFormat(EMITTER_MANIP value, FMT_SCOPE scope)
	{
		switch(value) {
			case Auto:
			case LongKey:
				_Set(m_mapKeyFmt, value, scope);
				return true;
			default:
				return false;
		}
	}
}

