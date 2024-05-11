#include <QTest>
#include "utility.h"
#include <string>
#include <memory>

using namespace std::literals;

#define STRING_LITERAL1 "$00$11$22$33$44$55$66$77$88$99$II$LL$**$RR$BB$$"
#define STRING_LITERAL2 "$0$1$2$3$4$5$6$7$8$9$I$L$*$R$B"
#define STRING_LITERAL3 ""
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

    void test_nocolor_strlen_qstring()
    {
        QCOMPARE(nocolor_strlen(QStringLiteral(STRING_LITERAL1)), 16);
        QCOMPARE(nocolor_strlen(QStringLiteral(STRING_LITERAL2)), 0);
        QCOMPARE(nocolor_strlen(QStringLiteral(STRING_LITERAL3)), 0);
        QCOMPARE(nocolor_strlen(QStringLiteral("")), 0);
    }

    void test_nocolor_strlen_c()
    {
        QCOMPARE(nocolor_strlen(STRING_LITERAL1), 16);
        QCOMPARE(nocolor_strlen(STRING_LITERAL2), 0);
        QCOMPARE(nocolor_strlen(STRING_LITERAL3), 0);
        QCOMPARE(nocolor_strlen(nullptr), 0);
    }

    void test_str_dup0()
    {
        QCOMPARE(strlen(str_dup0(STRING_LITERAL1)), strlen(STRING_LITERAL1));
        QCOMPARE(str_dup0(nullptr), nullptr);
    }

    void test_str_dup()
    {
        QCOMPARE(strlen(str_dup(STRING_LITERAL1)), strlen(STRING_LITERAL1));
        // QCOMPARE(str_dup(nullptr), nullptr);
    }
};

QTEST_MAIN(UtilityTest)
#include "test_utility.moc"