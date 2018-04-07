#pragma once

#include "stdafx.h"

class cpu_thread;
class ppu_thread;
class SPUThread;
class QJSEngine;
class QString;

// Expression evaluator utility for debug tools.
// Evaluates expressions taking context of PPU or SPU thread into account.
class expression_evaluator
{
public:
	// Evaluate given expression to u64 given the context of a CPU thread
	u64 evaluate_u64(const cpu_thread& cpu, const QString& expression);

	// Evaluate given expression to u64 given the context of a PPU thread
	u64 evaluate_u64(const ppu_thread& ppu, const QString& expression);

	// Evaluate given expression to u64 given the context of a SPU thread
	u64 evaluate_u64(const SPUThread& spu, const QString& expression);

	// Evaluate given expression to boolean given the context of a CPU thread
	bool evaluate_bool(const cpu_thread& cpu, const QString& expression);

	// Evaluate given expression to boolean given the context of a PPU thread
	bool evaluate_bool(const ppu_thread& ppu, const QString& expression);

	// Evaluate given expression to boolean given the context of a SPU thread
	bool evaluate_bool(const SPUThread& spu, const QString& expression);

private:
	template<typename T>
	u64 evaluate_u64_impl(const T& thread, const QString& expression);

	template<typename T>
	bool evaluate_bool_impl(const T& thread, const QString& expression);

	QJSEngine* make_context(const ppu_thread& ppu);

	QJSEngine* make_context(const SPUThread& spu);
};