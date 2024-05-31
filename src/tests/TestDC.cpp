#include <cstring>
#include <memory>

#include <QTest>

#include "utility.h"
#include "sing.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

using namespace std::literals;

#define STRING_LITERAL1 "$00$11$22$33$44$55$66$77$88$99$II$LL$**$RR$BB$$"
#define STRING_LITERAL2 "$0$1$2$3$4$5$6$7$8$9$I$L$*$R$B"
#define STRING_LITERAL3 ""
#define STRING_LITERAL4 "$"
#define STRING_LITERAL5 "test"
#define STRING_LITERAL6 "$z$>$;"

class TestDC : public QObject
{
    Q_OBJECT

    enum class VariableType
    {
        C_STRING,
        STD_STRING,
        QSTRING
    };

private slots:

    void test_double_dollars_qstring()
    {
        QString source = QStringLiteral("abc$def$12");
        QString destination = double_dollars(source);
        QCOMPARE(destination, QStringLiteral("abc$$def$$12"));
    }

    void test_nocolor_strlen_data()
    {
        QTest::addColumn<VariableType>("type");
        QTest::addColumn<QString>("string_literal");
        QTest::addColumn<size_t>("expected");

        QTest::newRow("C string test1") << VariableType::C_STRING << STRING_LITERAL1 << 16UL;
        QTest::newRow("C string test2") << VariableType::C_STRING << STRING_LITERAL2 << 0UL;
        QTest::newRow("C string test3") << VariableType::C_STRING << STRING_LITERAL3 << 0UL;
        QTest::newRow("C string test4") << VariableType::C_STRING << STRING_LITERAL4 << 1UL;
        QTest::newRow("C string test5") << VariableType::C_STRING << STRING_LITERAL5 << 4UL;
        QTest::newRow("C string test6") << VariableType::C_STRING << STRING_LITERAL6 << 6UL;

        QTest::newRow("QString test1") << VariableType::QSTRING << QStringLiteral(STRING_LITERAL1) << 16UL;
        QTest::newRow("QString test2") << VariableType::QSTRING << QStringLiteral(STRING_LITERAL2) << 0UL;
        QTest::newRow("QString test3") << VariableType::QSTRING << QStringLiteral(STRING_LITERAL3) << 0UL;
        QTest::newRow("QString test4") << VariableType::QSTRING << QStringLiteral(STRING_LITERAL4) << 1UL;
        QTest::newRow("QString test5") << VariableType::QSTRING << QStringLiteral(STRING_LITERAL5) << 4UL;
        QTest::newRow("QString test6") << VariableType::QSTRING << QStringLiteral(STRING_LITERAL6) << 6UL;
    }

    void test_nocolor_strlen()
    {
        QFETCH(VariableType, type);
        QFETCH(QString, string_literal);
        QFETCH(size_t, expected);

        if (type == VariableType::C_STRING)
        {
            QCOMPARE(nocolor_strlen(string_literal), expected);
        }
        else if (type == VariableType::QSTRING)
        {
            QCOMPARE(nocolor_strlen(QString(string_literal)), expected);
        }
    }

    void test_str_dup0()
    {
        std::unique_ptr<char, decltype(std::free) *> new_string = {str_dup0(STRING_LITERAL1), std::free};
        QVERIFY(new_string.get() != nullptr);
        QCOMPARE(strlen(new_string.get()), strlen(STRING_LITERAL1));

        QCOMPARE(str_dup0(nullptr), nullptr);
    }

    void test_str_dup()
    {
        std::unique_ptr<char, decltype(std::free) *> new_string = {str_dup(STRING_LITERAL1), std::free};
        QVERIFY(new_string.get() != nullptr);
        QCOMPARE(strlen(new_string.get()), strlen(STRING_LITERAL1));
        // causes expected crash
        // QCOMPARE(str_dup(nullptr), nullptr);
    }

    void test_dice()
    {
        QRandomGenerator rng(0);
        QCOMPARE(dice(4, 10, &rng), 11);
        QCOMPARE(dice(10, 4, &rng), 28);
        QCOMPARE(dice(0, 0, &rng), 1);
        QCOMPARE(dice(1, 0, &rng), 1);
        QCOMPARE(dice(0, 1, &rng), 0);
    }

    void test_str_cmp()
    {
        QVERIFY(str_cmp("ABC123", "abc123") == 0);
        QVERIFY(str_cmp("abc123", "abc123") == 0);
        QVERIFY(str_cmp("ABC123", "ABC123") == 0);
        QVERIFY(str_cmp("XYZ987", "ABC123") != 0);
    }

    void test_space_to_underscore()
    {
        QCOMPARE(space_to_underscore(QStringLiteral("  this is a test  ")), "__this_is_a_test__");
        QCOMPARE(space_to_underscore(std::string("  this is a test  ")), "__this_is_a_test__");
    }

    void test_str_nospace()
    {
        std::unique_ptr<char, decltype(std::free) *> result = {str_nospace("  this is a test  "), std::free};
        QVERIFY(result.get());
        QCOMPARE(result.get(), "__this_is_a_test__");
    }

    void test_str_nosp_cmp_c_string()
    {
        QCOMPARE(str_nosp_cmp("  this is a test  ", "__this_is_a_test__"), 0);
    }

    void test_str_nosp_cmp_qstring()
    {
        QCOMPARE(str_nosp_cmp(QStringLiteral("  this is a test  "), QStringLiteral("__this_is_a_test__")), 0);
    }

    void test_str_n_nosp_cmp_c_string()
    {
        QCOMPARE(str_n_nosp_cmp("  this is a test  ABC", "__THIS_IS_A_test__XYZ", 18), 0);
    }

    void test_str_n_nosp_cmp_begin()
    {
        QCOMPARE(str_n_nosp_cmp_begin(std::string("  this is a test  "), std::string("__THIS_IS_A_test__")), MatchType::Exact);
        QCOMPARE(str_n_nosp_cmp_begin(std::string("  that is a test  "), std::string("__THIS_IS_A_test__")), MatchType::Failure);
        QCOMPARE(str_n_nosp_cmp_begin(std::string("  this is a"), std::string("__THIS_IS_A_test__XYZ")), MatchType::Subset);

        QCOMPARE(str_n_nosp_cmp_begin(QStringLiteral("  this is a test  "), QStringLiteral("__THIS_IS_A_test__")), MatchType::Exact);
        QCOMPARE(str_n_nosp_cmp_begin(QStringLiteral("  that is a test  "), QStringLiteral("__THIS_IS_A_test__")), MatchType::Failure);
        QCOMPARE(str_n_nosp_cmp_begin(QStringLiteral("  this is a"), QStringLiteral("__THIS_IS_A_test__XYZ")), MatchType::Subset);
    }

    void test_update_character_singing()
    {
        char argc = 1;
        char *argv[] = {""};
        DC::config cf;
        cf.argc_ = argc;
        cf.argv_ = argv;
        cf.sql = false;

        DC dc(cf);
        dc.random_ = QRandomGenerator(0);
        dc.boot_zones();
        extern room_t top_of_world_alloc;
        top_of_world_alloc = 2000;
        dc.boot_world();
        renum_world();
        renum_zone_table();
        boot_social_messages();

        Character ch;
        ch.in_room = 3;
        ch.height = 72;
        ch.weight = 150;
        Player player;
        ch.player = &player;
        Connection conn;
        conn.descriptor = 1;
        conn.character = &ch;
        ch.desc = &conn;
        dc.character_list.insert(&ch);
        ch.do_on_login_stuff();

        QVERIFY(ch.isPlayer());
        QVERIFY(ch.isMortal());
        QVERIFY(!ch.isNPC());
        QVERIFY(!dc.character_list.empty());

        do_sing(&ch, "'flight of the bumblebee'");
        QCOMPARE(conn.output, "Lie still; you are DEAD.\r\n");
        conn.output = {};

        ch.setPosition(position_t::STANDING);
        do_sing(&ch, "'flight of the bumblebee'");
        QCOMPARE(conn.output, "You raise your clear (?) voice towards the sky.\r\n");
        conn.output = {};
        QVERIFY(ch.songs.empty());

        skill_results_t results = find_skills_by_name("flight_of_the_bumblebee");
        QVERIFY(!results.empty());
        QVERIFY(results.size() == 1);
        auto skillnum = results.begin()->second;
        QCOMPARE(ch.has_skill(skillnum), 0);
        ch.learn_skill(skillnum, 1, 100);
        QCOMPARE(ch.has_skill(skillnum), 1);

        do_sing(&ch, "'flight of the bumblebee'");
        QCOMPARE(conn.output, "You raise your clear (?) voice towards the sky.\r\n");
        conn.output = {};
        QVERIFY(ch.songs.empty());

        ch.setClass(10);
        do_sing(&ch, "'flight of the bumblebee'");
        QCOMPARE(conn.output, "You do not have enough ki!\r\n");
        conn.output = {};
        QVERIFY(ch.songs.empty());

        ch.setLevel(60);
        ch.intel = 25;
        redo_ki(&ch);
        ch.ki = ki_limit(&ch);
        do_sing(&ch, "'flight of the bumblebee'");
        QCOMPARE(conn.output, "You begin to sing a lofty song...\r\n");
        conn.output = {};
        QVERIFY(!ch.songs.empty());

        do_sing(&ch, "'flight of the bumblebee'");
        QCOMPARE(conn.output, "You are already in the middle of another song!\r\n");
        conn.output = {};
        QVERIFY(!ch.songs.empty());

        update_bard_singing();
        QVERIFY(!ch.songs.empty());
        QCOMPARE(conn.output, "Singing [flight of the bumblebee]: * * * * \r\n");
        conn.output = {};

        update_bard_singing();
        QVERIFY(!ch.songs.empty());
        QCOMPARE(conn.output, "Singing [flight of the bumblebee]: * * * \r\n");
        conn.output = {};

        update_bard_singing();
        QVERIFY(!ch.songs.empty());
        QCOMPARE(conn.output, "Singing [flight of the bumblebee]: * * \r\n");
        conn.output = {};

        update_bard_singing();
        QVERIFY(!ch.songs.empty());
        QVERIFY(!ch.affected_by_spell(SKILL_SONG_FLIGHT_OF_BEE));
        QVERIFY(!IS_AFFECTED(&ch, AFF_FLYING));
        QCOMPARE(conn.output, "Singing [flight of the bumblebee]: * \r\n");
        conn.output = {};

        update_bard_singing();
        QVERIFY(!ch.songs.empty());
        QVERIFY(ch.affected_by_spell(SKILL_SONG_FLIGHT_OF_BEE));
        QVERIFY(IS_AFFECTED(&ch, AFF_FLYING));
        QCOMPARE(conn.output, "Your feet feel like air.\r\n");
        conn.output = {};

        update_bard_singing();
        QVERIFY(ch.songs.empty());
        QVERIFY(ch.affected_by_spell(SKILL_SONG_FLIGHT_OF_BEE));
        QVERIFY(IS_AFFECTED(&ch, AFF_FLYING));
        QCOMPARE(conn.output, "");
        conn.output = {};

        affect_update(DC::PULSE_TIME);
        QVERIFY(ch.affected_by_spell(SKILL_SONG_FLIGHT_OF_BEE));
        QVERIFY(IS_AFFECTED(&ch, AFF_FLYING));
        QCOMPARE(conn.output, "");
        conn.output = {};

        affect_update(DC::PULSE_TIME);
        QVERIFY(!ch.affected_by_spell(SKILL_SONG_FLIGHT_OF_BEE));
        QVERIFY(!IS_AFFECTED(&ch, AFF_FLYING));
        QCOMPARE(conn.output, "Your feet touch the ground once more.\r\n");
        conn.output = {};
    }
};

QTEST_MAIN(TestDC)
#include "TestDC.moc"