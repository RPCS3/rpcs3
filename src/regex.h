#pragma once

#include <vector>
#include <string>

namespace YAML
{
	class Stream;
	struct Buffer;

	enum REGEX_OP { REGEX_EMPTY, REGEX_MATCH, REGEX_RANGE, REGEX_OR, REGEX_AND, REGEX_NOT, REGEX_SEQ };

	// simplified regular expressions
	// . Only straightforward matches (no repeated characters)
	// . Only matches from start of string
	class RegEx
	{
	private:
		// the operators
		struct Operator {
			virtual ~Operator() {}
			virtual int Match(const Buffer& buffer, const RegEx& regex) const = 0;
		};

		struct MatchOperator: public Operator {
			virtual int Match(const Buffer& buffer, const RegEx& regex) const;
		};

		struct RangeOperator: public Operator {
			virtual int Match(const Buffer& buffer, const RegEx& regex) const;
		};

		struct OrOperator: public Operator {
			virtual int Match(const Buffer& buffer, const RegEx& regex) const;
		};

		struct AndOperator: public Operator {
			virtual int Match(const Buffer& buffer, const RegEx& regex) const;
		};

		struct NotOperator: public Operator {
			virtual int Match(const Buffer& buffer, const RegEx& regex) const;
		};

		struct SeqOperator: public Operator {
			virtual int Match(const Buffer& buffer, const RegEx& regex) const;
		};

	public:
		friend struct Operator;

		RegEx();
		RegEx(char ch);
		RegEx(char a, char z);
		RegEx(const std::string& str, REGEX_OP op = REGEX_SEQ);
		RegEx(const RegEx& rhs);
		~RegEx();

		RegEx& operator = (const RegEx& rhs);

		bool Matches(char ch) const;
		bool Matches(const std::string& str) const;
		bool Matches(const Buffer& buffer) const;
		bool Matches(const Stream& in) const;
		int Match(const std::string& str) const;
		int Match(const Buffer& buffer) const;
		int Match(const Stream& in) const;

		friend RegEx operator ! (const RegEx& ex);
		friend RegEx operator || (const RegEx& ex1, const RegEx& ex2);
		friend RegEx operator && (const RegEx& ex1, const RegEx& ex2);
		friend RegEx operator + (const RegEx& ex1, const RegEx& ex2);

	private:
		RegEx(REGEX_OP op);
		void SetOp();

	private:
		REGEX_OP m_op;
		Operator *m_pOp;
		char m_a, m_z;
		std::vector <RegEx> m_params;
	};
}
