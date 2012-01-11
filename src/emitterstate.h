#ifndef EMITTERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define EMITTERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "ptr_stack.h"
#include "setting.h"
#include "yaml-cpp/emittermanip.h"
#include <cassert>
#include <vector>
#include <stack>
#include <memory>

namespace YAML
{
	enum FMT_SCOPE {
		LOCAL,
		GLOBAL
	};
	
	enum GROUP_TYPE {
		GT_NONE,
		GT_SEQ,
		GT_MAP
	};
	
	enum FLOW_TYPE {
		FT_NONE,
		FT_FLOW,
		FT_BLOCK
	};
	
	enum NODE_STATE {
		NS_START,
		NS_READY_FOR_ATOM,
		NS_END
	};
	
	enum EMITTER_STATE {
		ES_WAITING_FOR_DOC,
		ES_WRITING_DOC,
		ES_DONE_WITH_DOC,
		
		// block seq
		ES_WAITING_FOR_BLOCK_SEQ_ENTRY,
		ES_WRITING_BLOCK_SEQ_ENTRY,
		ES_DONE_WITH_BLOCK_SEQ_ENTRY,
		
		// flow seq
		ES_WAITING_FOR_FLOW_SEQ_ENTRY,
		ES_WRITING_FLOW_SEQ_ENTRY,
		ES_DONE_WITH_FLOW_SEQ_ENTRY,
		
		// block map
		ES_WAITING_FOR_BLOCK_MAP_ENTRY,
		ES_WAITING_FOR_BLOCK_MAP_KEY,
		ES_WRITING_BLOCK_MAP_KEY,
		ES_DONE_WITH_BLOCK_MAP_KEY,
		ES_WAITING_FOR_BLOCK_MAP_VALUE,
		ES_WRITING_BLOCK_MAP_VALUE,
		ES_DONE_WITH_BLOCK_MAP_VALUE,
		
		// flow map
		ES_WAITING_FOR_FLOW_MAP_ENTRY,
		ES_WAITING_FOR_FLOW_MAP_KEY,
		ES_WRITING_FLOW_MAP_KEY,
		ES_DONE_WITH_FLOW_MAP_KEY,
		ES_WAITING_FOR_FLOW_MAP_VALUE,
		ES_WRITING_FLOW_MAP_VALUE,
		ES_DONE_WITH_FLOW_MAP_VALUE
	};
	
	class EmitterState
	{
	public:
		EmitterState();
		~EmitterState();
		
		// basic state checking
		bool good() const { return m_isGood; }
		const std::string GetLastError() const { return m_lastError; }
		void SetError(const std::string& error) { m_isGood = false; m_lastError = error; }
		
		// main state of the machine
		EMITTER_STATE GetCurState() const { return m_stateStack.top(); }
		void SwitchState(EMITTER_STATE state) { PopState(); PushState(state); }
		void PushState(EMITTER_STATE state) { m_stateStack.push(state); }
		void PopState() { m_stateStack.pop(); }
		
		void SetLocalValue(EMITTER_MANIP value);
		
		// group handling
		void BeginGroup(GROUP_TYPE type);
		void EndGroup(GROUP_TYPE type);
		
		GROUP_TYPE GetCurGroupType() const;
		FLOW_TYPE GetCurGroupFlowType() const;
		int GetCurIndent() const { return m_curIndent; }
		
		bool CurrentlyInLongKey();
		void StartLongKey();
		void StartSimpleKey();

		bool RequiresSoftSeparation() const { return m_requiresSoftSeparation; }
		bool RequiresHardSeparation() const { return m_requiresHardSeparation; }
		void RequireSoftSeparation() { m_requiresSoftSeparation = true; }
		void RequireHardSeparation() { m_requiresSoftSeparation = true; m_requiresHardSeparation = true; }
		void ForceHardSeparation() { m_requiresSoftSeparation = false; }
		void UnsetSeparation() { m_requiresSoftSeparation = false; m_requiresHardSeparation = false; }

		void ClearModifiedSettings();

		// formatters
		bool SetOutputCharset(EMITTER_MANIP value, FMT_SCOPE scope);
		EMITTER_MANIP GetOutputCharset() const { return m_charset.get(); }

		bool SetStringFormat(EMITTER_MANIP value, FMT_SCOPE scope);
		EMITTER_MANIP GetStringFormat() const { return m_strFmt.get(); }
		
		bool SetBoolFormat(EMITTER_MANIP value, FMT_SCOPE scope);
		EMITTER_MANIP GetBoolFormat() const { return m_boolFmt.get(); }

		bool SetBoolLengthFormat(EMITTER_MANIP value, FMT_SCOPE scope);
		EMITTER_MANIP GetBoolLengthFormat() const { return m_boolLengthFmt.get(); }

		bool SetBoolCaseFormat(EMITTER_MANIP value, FMT_SCOPE scope);
		EMITTER_MANIP GetBoolCaseFormat() const { return m_boolCaseFmt.get(); }

		bool SetIntFormat(EMITTER_MANIP value, FMT_SCOPE scope);
		EMITTER_MANIP GetIntFormat() const { return m_intFmt.get(); }

		bool SetIndent(unsigned value, FMT_SCOPE scope);
		int GetIndent() const { return m_indent.get(); }
		
		bool SetPreCommentIndent(unsigned value, FMT_SCOPE scope);
		int GetPreCommentIndent() const { return m_preCommentIndent.get(); }
		bool SetPostCommentIndent(unsigned value, FMT_SCOPE scope);
		int GetPostCommentIndent() const { return m_postCommentIndent.get(); }
		
		bool SetFlowType(GROUP_TYPE groupType, EMITTER_MANIP value, FMT_SCOPE scope);
		EMITTER_MANIP GetFlowType(GROUP_TYPE groupType) const;
		
		bool SetMapKeyFormat(EMITTER_MANIP value, FMT_SCOPE scope);
		EMITTER_MANIP GetMapKeyFormat() const { return m_mapKeyFmt.get(); }
        
        bool SetFloatPrecision(int value, FMT_SCOPE scope);
        unsigned GetFloatPrecision() const { return m_floatPrecision.get(); }
        bool SetDoublePrecision(int value, FMT_SCOPE scope);
        unsigned GetDoublePrecision() const { return m_doublePrecision.get(); }
		
	private:
		template <typename T>
		void _Set(Setting<T>& fmt, T value, FMT_SCOPE scope);
		
	private:
		// basic state ok?
		bool m_isGood;
		std::string m_lastError;
		
		// other state
		std::stack<EMITTER_STATE> m_stateStack;
		
		Setting<EMITTER_MANIP> m_charset;
		Setting<EMITTER_MANIP> m_strFmt;
		Setting<EMITTER_MANIP> m_boolFmt;
		Setting<EMITTER_MANIP> m_boolLengthFmt;
		Setting<EMITTER_MANIP> m_boolCaseFmt;
		Setting<EMITTER_MANIP> m_intFmt;
		Setting<unsigned> m_indent;
		Setting<unsigned> m_preCommentIndent, m_postCommentIndent;
		Setting<EMITTER_MANIP> m_seqFmt;
		Setting<EMITTER_MANIP> m_mapFmt;
		Setting<EMITTER_MANIP> m_mapKeyFmt;
        Setting<int> m_floatPrecision;
        Setting<int> m_doublePrecision;
		
		SettingChanges m_modifiedSettings;
		SettingChanges m_globalModifiedSettings;
		
		struct Group {
			Group(GROUP_TYPE type_): type(type_), usingLongKey(false), indent(0) {}
			
			GROUP_TYPE type;
			EMITTER_MANIP flow;
			bool usingLongKey;
			int indent;
			
			SettingChanges modifiedSettings;
		};

		ptr_stack<Group> m_groups;
		unsigned m_curIndent;
		bool m_requiresSoftSeparation;
		bool m_requiresHardSeparation;
	};

	template <typename T>
	void EmitterState::_Set(Setting<T>& fmt, T value, FMT_SCOPE scope) {
		switch(scope) {
			case LOCAL:
				m_modifiedSettings.push(fmt.set(value));
				break;
			case GLOBAL:
				fmt.set(value);
				m_globalModifiedSettings.push(fmt.set(value));  // this pushes an identity set, so when we restore,
				                                                // it restores to the value here, and not the previous one
				break;
			default:
				assert(false);
		}
	}
}

#endif // EMITTERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
