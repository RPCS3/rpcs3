#pragma once

#include <QValidator>
#include <QRegularExpression>

class HexValidator : public QValidator
{
public:
	explicit HexValidator(int max_nibbles, QObject* parent = nullptr)
		: QValidator(parent), m_max_nibbles(max_nibbles)
    {}

    State validate(QString& input, int& pos) const override
    {
        Q_UNUSED(pos);

        QString stripped = input;
        stripped.remove(' ');

        if (stripped.startsWith("0x", Qt::CaseInsensitive)) 
            stripped = stripped.mid(2);

        if (stripped.endsWith("h", Qt::CaseInsensitive))
			stripped.chop(1);

        if (stripped.isEmpty())
            return QValidator::Intermediate;

        static const QRegularExpression hex_re("^[0-9A-Fa-f]*$");
		if (!hex_re.match(stripped).hasMatch())
            return QValidator::Invalid;

        if (stripped.length() > m_max_nibbles)
            return QValidator::Invalid;

        return QValidator::Acceptable;
    }

private:
	const int m_max_nibbles;
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
