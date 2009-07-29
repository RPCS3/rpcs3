#pragma once

#ifndef MARK_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define MARK_H_62B23520_7C8E_11DE_8A39_0800200C9A66


namespace YAML
{
	struct Mark {
		Mark(): pos(0), line(0), column(0) {}
		
		static const Mark null() { return Mark(-1, -1, -1); }
		
		int pos;
		int line, column;
		
	private:
		Mark(int pos_, int line_, int column_): pos(pos_), line(line_), column(column_) {}
	};
}

#endif // MARK_H_62B23520_7C8E_11DE_8A39_0800200C9A66
