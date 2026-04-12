#include <QTest>
#include <QString>

using namespace std::literals;

const auto STRING_LITERAL1 = u"$00$11$22$33$44$55$66$77$88$99$II$LL$**$RR$BB$$"_s);

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
    size_t result = {};

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
        auto result = nocolor_strlen(QStringLiteral(STRING_LITERAL1));
      }
    }
    Q_UNUSED(result);
  }

  void benchmark_0()
  {
    QBENCHMARK
    {
      auto result = QStringLiteral(STRING_LITERAL1);
    }
  }

  void benchmark_()
  {
    QBENCHMARK
    {
      auto result = QStringLiteral(STRING_LITERAL1);
    }
  }

  void benchmark_str_cmp()
  {
    qint32 result = {};
    QBENCHMARK
    {
      result = u"ABC123"_s != u"abc123"_s;
    }
    Q_UNUSED(result);
  }

  void benchmark_space_to_underscore_data()
  {
    QTest::addColumn<VariableType>("type");
    QTest::newRow("QString") << VariableType::STD_STRING;
    QTest::newRow("QString") << VariableType::QSTRING;
  }

  void benchmark_space_to_underscore()
  {
    QFETCH(VariableType, type);
    if (type == VariableType::STD_STRING)
    {
      QString result;
      QBENCHMARK
      {
        result = space_to_underscore(QString("  this is a test  "));
      }
      Q_UNUSED(result);
    }
    else if (type == VariableType::QSTRING)
    {
      QString result;
      QBENCHMARK
      {
        result = space_to_underscore(u"  this is a test  "_s);
      }
      Q_UNUSED(result);
    }
  }

  void benchmark_str_nospace()
  {
    QBENCHMARK
    {
      str_nospace("  this is a test  ");
    }
  }

  void benchmark_str_n_nosp_cmp_begin_data()
  {
    QTest::addColumn<VariableType>("type");
    QTest::newRow("QString") << VariableType::STD_STRING;
    QTest::newRow("QString") << VariableType::QSTRING;
  }
  void benchmark_str_n_nosp_cmp_begin()
  {
    QFETCH(VariableType, type);
    qint32 result = {};
    if (type == VariableType::STD_STRING)
    {
      QBENCHMARK
      {
        result = str_n_nosp_cmp_begin(QString("  this is a test  "), QString("__THIS_IS_A_test__"));
      }
    }
    else if (type == VariableType::QSTRING)
    {
      QBENCHMARK
      {
        result = str_n_nosp_cmp_begin(u"  this is a test  "_s, u"__THIS_IS_A_test__"_s);
      }
    }
    Q_UNUSED(result);
  }
};

QTEST_MAIN(BenchmarkUtility)
#include "benchmarkDC.moc"