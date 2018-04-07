#include "expression_evaluator.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include <QString>
#include <QJSEngine>

u64 expression_evaluator::evaluate_u64(const cpu_thread& cpu, const QString& expression)
{
	if (cpu.id_type() == 1)
	{
		return evaluate_u64((ppu_thread&)cpu, expression);
	}
	else
	{
		return evaluate_u64((SPUThread&)cpu, expression);
	}
}

u64 expression_evaluator::evaluate_u64(const ppu_thread& ppu, const QString& expression)
{
	return evaluate_u64_impl(ppu, expression);
}

u64 expression_evaluator::evaluate_u64(const SPUThread& spu, const QString& expression)
{
	return evaluate_u64_impl(spu, expression);
}

bool expression_evaluator::evaluate_bool(const cpu_thread& cpu, const QString& expression)
{
	if (cpu.id_type() == 1)
	{
		return evaluate_bool((ppu_thread&)cpu, expression);
	}
	else
	{
		return evaluate_bool((SPUThread&)cpu, expression);
	}
}

bool expression_evaluator::evaluate_bool(const ppu_thread& ppu, const QString& expression)
{
	return evaluate_bool_impl(ppu, expression);
}

bool expression_evaluator::evaluate_bool(const SPUThread& spu, const QString& expression)
{
	return evaluate_bool_impl(spu, expression);
}

template<typename T>
inline u64 expression_evaluator::evaluate_u64_impl(const T& thread, const QString& expression)
{
	auto context = make_context(thread);
	auto value = context->evaluate(expression);
	return static_cast<u64>(value.toNumber());
}

template<typename T>
inline bool expression_evaluator::evaluate_bool_impl(const T& thread, const QString& expression)
{
	auto context = make_context(thread);
	auto value = context->evaluate(expression);
	return value.toBool();
}

QJSEngine* expression_evaluator::make_context(const ppu_thread& ppu)
{
	auto context = new QJSEngine();

	// Reduce boilerplate
	const auto prop = [&](const QString& name, const QJSValue& value)
	{
		context->globalObject().setProperty(name, value);
	};

	const auto prop64 = [&](const QString& name, u64 value)
	{
		prop(name + "hi", QJSValue((u32)(value >> 32)));
		prop(name, QJSValue((u32)(value)));
	};

	// Define properties for registers
	prop("pc", ppu.cia);
	prop("cia", ppu.cia);

	for (int i = 0; i < 32; ++i)
	{
		prop64(QString("r%1").arg(i), ppu.gpr[i]);
	}

	prop64("lr", ppu.lr);
	prop64("ctr", ppu.ctr);

	return context;
}

QJSEngine* expression_evaluator::make_context(const SPUThread& spu)
{
	auto context = new QJSEngine();

	// Reduce boilerplate
	const auto prop = [&](const QString &name, const QJSValue &value)
	{
		context->globalObject().setProperty(name, value);
	};

	// Define properties for registers
	prop("pc", spu.pc);
	prop("cia", spu.pc);

	for (int i = 0; i < 128; ++i)
	{
		prop(QString("r%1hi").arg(i), QJSValue(spu.gpr[i]._u32[0]));
		prop(QString("r%1lo").arg(i), QJSValue(spu.gpr[i]._u32[1]));
		prop(QString("r%1hilo").arg(i), QJSValue(spu.gpr[i]._u32[2]));
		prop(QString("r%1hihi").arg(i), QJSValue(spu.gpr[i]._u32[3]));
	}

	return context;
}