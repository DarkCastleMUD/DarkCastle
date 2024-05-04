#include <QTest>
#include "utility.h"
#include <string>
#include <memory>

using namespace std::literals;

class UtilityTest : public QObject
{
    Q_OBJECT
private slots:
    void test_double_dollars_c()
    {
        std::unique_ptr<char> source(new char[11]);
        std::unique_ptr<char> destination(new char[13]);
        QVERIFY(source.get() != nullptr);
        QVERIFY(destination.get() != nullptr);

        strncpy(source.get(), "abc$def$12", 10);
        double_dollars(destination.get(), source.get());
        QCOMPARE(strncmp(destination.get(), "abc$$def$$12", 12), 0);
    }
    void test_double_dollars_std_string()
    {
        std::string source = "abc$def$12"s;
        std::string destination = double_dollars(source);
        QCOMPARE(destination, "abc$$def$$12"s);
    }
    void test_double_dollars_qstring()
    {
        QString source = QStringLiteral("abc$def$12");
        QString destination = double_dollars(source);
        QCOMPARE(destination, QStringLiteral("abc$$def$$12"));
    }
};

QTEST_MAIN(UtilityTest)
#include "test_utility.moc"