#pragma once

#include <QValidator>
#include <QRegularExpression>

class HexValidator : public QValidator
{
public:
    explicit HexValidator(int maxNibbles, QObject *parent = nullptr)
      : QValidator(parent)
      , m_maxNibbles(maxNibbles)
    {}

    State validate(QString &input, int &pos) const override
    {
        Q_UNUSED(pos);

        QString stripped = input;
        stripped.remove(' ');

        if (stripped.startsWith("0x", Qt::CaseInsensitive)) 
            stripped = stripped.mid(2);

        if (stripped.isEmpty())
            return QValidator::Intermediate;

        static const QRegularExpression hexRe("^[0-9A-Fa-f]*$");
        if (!hexRe.match(stripped).hasMatch())
            return QValidator::Invalid;

        if (stripped.length() > m_maxNibbles)
            return QValidator::Invalid;

        return QValidator::Acceptable;
    }

private:
    const int m_maxNibbles;
};

inline QString normalizeHexQString(const QString &input) {
    QString s = input;
    s.remove(' ');
    if (s.startsWith("0x"))
        s = s.mid(2);
    return s;
}