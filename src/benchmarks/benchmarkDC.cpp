#include <QTest>
#include "DC/utility.h"
#include <string>
#include <memory>

using namespace std::literals;

#define STRING_LITERAL1 "$00$11$22$33$44$55$66$77$88$99$II$LL$**$RR$BB$$"

class BenchmarkUtility : public QObject
{
    Q_OBJECT

    enum class VariableType
    {
        C_STRING,
        STD_STRING,
        QSTRING
    };

private slots:

    void benchmark_nocolor_strlen_data()
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
        QBENCHMARK
        {
            std::unique_ptr<char, decltype(std::free) *> result = {str_dup0(STRING_LITERAL1), std::free};
        }
    }

    void benchmark_str_dup()
    {
        QBENCHMARK
        {
            std::unique_ptr<char, decltype(std::free) *> result = {str_dup(STRING_LITERAL1), std::free};
        }
    }

    void benchmark_str_cmp()
    {
        int result{};
        QBENCHMARK
        {
            result = str_cmp("ABC123", "abc123");
        }
        Q_UNUSED(result);
    }

    void benchmark_space_to_underscore_data()
    {
        QTest::addColumn<VariableType>("type");
        QTest::newRow("std::string") << VariableType::STD_STRING;
        QTest::newRow("QString") << VariableType::QSTRING;
    }

    void benchmark_space_to_underscore()
    {
        QFETCH(VariableType, type);
        if (type == VariableType::STD_STRING)
        {
            std::string result;
            QBENCHMARK
            {
                result = space_to_underscore(std::string("  this is a test  "));
            }
            Q_UNUSED(result);
        }
        else if (type == VariableType::QSTRING)
        {
            QString result;
            QBENCHMARK
            {
                result = space_to_underscore(QStringLiteral("  this is a test  "));
            }
            Q_UNUSED(result);
        }
    }

    void benchmark_str_nospace()
    {
        QBENCHMARK
        {
            std::unique_ptr<char, decltype(std::free) *> result = {str_nospace("  this is a test  "), std::free};
        }
    }

    void benchmark_str_n_nosp_cmp_begin_data()
    {
        QTest::addColumn<VariableType>("type");
        QTest::newRow("std::string") << VariableType::STD_STRING;
        QTest::newRow("QString") << VariableType::QSTRING;
    }
    void benchmark_str_n_nosp_cmp_begin()
    {
        QFETCH(VariableType, type);
        int result{};
        if (type == VariableType::STD_STRING)
        {
            QBENCHMARK
            {
                result = str_n_nosp_cmp_begin(std::string("  this is a test  "), std::string("__THIS_IS_A_test__"));
            }
        }
        else if (type == VariableType::QSTRING)
        {
            QBENCHMARK
            {
                result = str_n_nosp_cmp_begin(QStringLiteral("  this is a test  "), QStringLiteral("__THIS_IS_A_test__"));
            }
        }
        Q_UNUSED(result);
    }
};

QTEST_MAIN(BenchmarkUtility)
#include "benchmarkDC.moc"