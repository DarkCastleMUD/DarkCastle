#include <QTest>
#include "utility.h"
#include <string>
#include <memory>

using namespace std::literals;

#define STRING_LITERAL1 "$00$11$22$33$44$55$66$77$88$99$II$LL$**$RR$BB$$"

class UtilityBenchmark : public QObject
{
    Q_OBJECT

    enum class VariableType
    {
        C_STRING,
        STD_STRING,
        QSTRING
    };

private slots:

    void
    benchmark_nocolor_strlen_data()
    {
        QTest::addColumn<VariableType>("type");
        QTest::newRow("C style string") << VariableType::C_STRING;
        QTest::newRow("QString") << VariableType::QSTRING;
    }

    void benchmark_nocolor_strlen()
    {
        QFETCH(VariableType, type);
        size_t result{};

        if (type == VariableType::C_STRING)
        {
            QBENCHMARK
            {
                result = nocolor_strlen(STRING_LITERAL1);
            }
        }
        else if (type == VariableType::QSTRING)
        {
            QBENCHMARK
            {
                result = nocolor_strlen(QStringLiteral(STRING_LITERAL1));
            }
        }
        Q_UNUSED(result);
    }

    void benchmark_str_dup0()
    {
        char *result{};
        QBENCHMARK
        {
            result = str_dup0(STRING_LITERAL1);
        }
        QCOMPARE(strlen(result), strlen(STRING_LITERAL1));
    }

    void benchmark_str_dup()
    {
        char *result{};
        QBENCHMARK
        {
            result = str_dup(STRING_LITERAL1);
        }
        QCOMPARE(strlen(result), strlen(STRING_LITERAL1));
    }
};

QTEST_MAIN(UtilityBenchmark)
#include "benchmark_utility.moc"