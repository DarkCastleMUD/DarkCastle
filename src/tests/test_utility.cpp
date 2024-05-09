#include <QTest>
#include "utility.h"
#include <string>
#include <memory>

using namespace std::literals;

class UtilityTest : public QObject
{
    Q_OBJECT
private slots:
    void test_double_dollars_qstring()
    {
        QString source = QStringLiteral("abc$def$12");
        QString destination = double_dollars(source);
        QCOMPARE(destination, QStringLiteral("abc$$def$$12"));
    }
};

QTEST_MAIN(UtilityTest)
#include "test_utility.moc"