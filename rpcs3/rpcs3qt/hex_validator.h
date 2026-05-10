#pragma once

#include <QValidator>
#include <QRegularExpression>

class HexValidator : public QValidator
{
public:
	explicit HexValidator(QObject* parent = nullptr, int max_bits = 32)
		: QValidator(parent)
		, m_max_bits(max_bits)
	{}

	State validate(QString& input, int& pos) const override
	{
		Q_UNUSED(pos);

		QString stripped = input.toLower().remove(' ');

		if (stripped.startsWith("0x"))
			stripped = stripped.mid(2);

		if (stripped.endsWith("h"))
			stripped.chop(1);

		if (stripped.isEmpty())
			return QValidator::Intermediate;

		if (stripped.length() > 16)
			return QValidator::Invalid;

		static const QRegularExpression hex_re("^[0-9a-f]+$");
		if (!hex_re.match(stripped).hasMatch())
			return QValidator::Invalid;

		QString sig = stripped;
		sig.remove(QRegularExpression("^0+"));
		const int sig_nibbles = sig.isEmpty() ? 1 : sig.length();
		if (sig_nibbles > (m_max_bits + 3) / 4)
			return QValidator::Invalid;

		bool ok = false;
		const qulonglong value = stripped.toULongLong(&ok, 16);
		if (!ok)
			return QValidator::Invalid;

		if (m_max_bits < 64)
		{
			const qulonglong max_val = (qulonglong(1) << m_max_bits) - 1;
			if (value > max_val)
				return QValidator::Invalid;
		}

		return QValidator::Acceptable;
	}

private:
	const int m_max_bits;
};

inline QString normalize_hex_qstring(const QString& input)
{
	QString s = input;
	s.remove(' ');
	s = s.toLower();
	if (s.startsWith("0x"))
		s = s.mid(2);
	if (s.endsWith('h'))
		s.chop(1);
	return s;
}

inline bool parse_hex_qstring(const QString& input, u64* result, int max_bits = 32)
{
	QString s = input;
	int pos = 0;
	const HexValidator validator(nullptr, max_bits);
	const QValidator::State st = validator.validate(s, pos);

	if (st != QValidator::Acceptable)
		return false;

	const QString norm = normalize_hex_qstring(input);
	bool ok = false;
	const quint64 value = norm.toULongLong(&ok, 16);

	if (ok && result)
		*result = static_cast<u64>(value);

	return ok;
}
