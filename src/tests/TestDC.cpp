#include <cstring>
#include <memory>

#include <QTest>
#include <QtLogging>

#include "DC/utility.h"
#include "DC/sing.h"
#include "DC/comm.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/spells.h"
#include "DC/vault.h"

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

public:
    TestDC()
    {
        qSetMessagePattern(QStringLiteral("%{if-category}%{category}:%{endif}%{file}:%{line} %{function}: %{message}"));
    }

private:
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
        DC::config cf;
        cf.sql = false;

        DC dc(cf);
        dc.random_ = QRandomGenerator(0);
        dc.boot_zones();
        extern room_t top_of_world_alloc;
        top_of_world_alloc = 2000;
        dc.boot_world();
        renum_world();
        renum_zone_table();
        void boot_social_messages(void);
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

        do_sing(&ch, str_hsh("'flight of the bumblebee'"));
        QCOMPARE(conn.output, "Lie still; you are DEAD.\r\n");
        conn.output = {};

        ch.setPosition(position_t::STANDING);
        do_sing(&ch, str_hsh("'flight of the bumblebee'"));
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

        do_sing(&ch, str_hsh("'flight of the bumblebee'"));
        QCOMPARE(conn.output, "You raise your clear (?) voice towards the sky.\r\n");
        conn.output = {};
        QVERIFY(ch.songs.empty());

        ch.setClass(10);
        do_sing(&ch, str_hsh("'flight of the bumblebee'"));
        QCOMPARE(conn.output, "You do not have enough ki!\r\n");
        conn.output = {};
        QVERIFY(ch.songs.empty());

        ch.setLevel(60);
        ch.intel = 25;
        redo_ki(&ch);
        ch.ki = ki_limit(&ch);
        do_sing(&ch, str_hsh("'flight of the bumblebee'"));
        QCOMPARE(conn.output, "You begin to sing a lofty song...\r\n");
        conn.output = {};
        QVERIFY(!ch.songs.empty());

        do_sing(&ch, str_hsh("'flight of the bumblebee'"));
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

    void test_do_vault_get()
    {
        DC::config cf;
        cf.sql = false;

        DC dc(cf);
        dc.random_ = QRandomGenerator(0);
        dc.boot_db();

        Character ch;
        ch.setName(QStringLiteral("Testplayer"));
        ch.in_room = 3;
        ch.height = 72;
        ch.weight = 150;
        ch.setClass(CLASS_WARRIOR);
        Player player;
        ch.player = &player;
        Connection conn;
        conn.descriptor = 1;
        conn.character = &ch;
        ch.desc = &conn;
        dc.character_list.insert(&ch);
        ch.do_on_login_stuff();

        auto new_rnum = create_blank_item(1);
        QCOMPARE(new_rnum.error(), create_error::entry_exists);
        int rnum = real_object(1);
        Object *o1 = clone_object(rnum);
        Object *o2 = clone_object(rnum);
        Object *o3 = clone_object(rnum);
        QVERIFY(o1);
        QVERIFY(o2);
        QVERIFY(o3);
        GET_OBJ_NAME(o1) = str_hsh("sword");
        GET_OBJ_SHORT(o1) = str_hsh("a short sword");
        GET_OBJ_NAME(o2) = str_hsh("sword");
        GET_OBJ_SHORT(o2) = str_hsh("a short sword");
        GET_OBJ_NAME(o3) = str_hsh("mushroom");
        GET_OBJ_SHORT(o3) = str_hsh("a small mushroom");

        command_return_t rc;
        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't feel safe enough to manage your valuables.\r\n");
        conn.output = {};

        rc = move_char(&ch, 3001);
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};

        vault_data *vault = has_vault(ch.getNameC());
        if (vault)
        {
            remove_vault(ch.getNameC()); // free it up first..
        }

        rc = do_vault(&ch, str_hsh("list"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have a vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have a vault.\r\n");
        conn.output = {};

        while (ch.getLevel() < 10)
        {
            ch.incrementLevel();
            advance_level(&ch, 0);
        }
        QCOMPARE(conn.output, "Your gain is: 13/16 hp, 1/1 m, 1/21 mv, 0/0 prac, 1/1 ki.\r\nYour gain is: 14/30 hp, 1/2 m, 1/22 mv, 0/0 prac, 0/1 ki.\r\nYour gain is: 11/41 hp, 1/3 m, 1/23 mv, 0/0 prac, 1/2 ki.\r\nYour gain is: 13/54 hp, 1/4 m, 1/24 mv, 0/0 prac, 0/2 ki.\r\nYour gain is: 13/67 hp, 1/5 m, 1/25 mv, 0/0 prac, 1/3 ki.\r\nYour gain is: 10/77 hp, 1/6 m, 1/26 mv, 0/0 prac, 0/3 ki.\r\nYou are now able to participate in pkilling!\n\rRead HELP PKILL for more information.\r\nYour gain is: 14/91 hp, 1/7 m, 1/27 mv, 0/0 prac, 1/4 ki.\r\nYour gain is: 13/104 hp, 1/8 m, 1/28 mv, 0/0 prac, 0/4 ki.\r\nYour gain is: 10/114 hp, 1/9 m, 1/29 mv, 0/0 prac, 1/5 ki.\r\nYour gain is: 13/127 hp, 1/10 m, 1/30 mv, 0/0 prac, 0/5 ki.\r\nYou have been given a vault in which to place your valuables!\n\rRead HELP VAULT for more information.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("list"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "Your vault is currently empty and can hold 100 pounds.\r\n");
        conn.output = {};

        int status;
        status = obj_to_char(o1, &ch);
        QVERIFY(status);
        status = obj_to_char(o2, &ch);
        QVERIFY(status);
        status = obj_to_char(o3, &ch);
        QVERIFY(status);

        rc = do_vault(&ch, str_hsh("put all")); // put all
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a small mushroom has been placed in the vault.\r\na short sword has been placed in the vault.\r\na short sword has been placed in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("get all")); // get all
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a short sword has been removed from the vault.\r\na short sword has been removed from the vault.\r\na small mushroom has been removed from the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all.sword")); // put all.keyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a short sword has been placed in the vault.\r\na short sword has been placed in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put mushroom")); // put keyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a small mushroom has been placed in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("get all.sword")); // get all.keyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a short sword has been removed from the vault.\r\na short sword has been removed from the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("get mushroom")); // get keyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a small mushroom has been removed from the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put 2.mushroom")); // put bad#.keyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have anything like that.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put 1.mushroom")); // put #.keyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a small mushroom has been placed in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put 2.sword"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a short sword has been placed in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put 2.sword"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have anything like that.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put 1.sword"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a short sword has been placed in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("get all.missing")); // get all.missingkeyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all.missing")); // put all.missingkeyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put undefined.missing")); // put undefined.missingkeyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have anything like that.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put missing")); // put missing
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have anything like that.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("get missing")); // get missing
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("get undefined.sword")); // put undefined.keyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put 1.missing")); // put #.missingkeyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have anything like that.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put 2.missing")); // put bad#.missingkeyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have anything like that.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put -2.sword")); // put invalid#.keyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have anything like that.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put -2.missing")); // put invalid#.missingkeyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "You don't have anything like that.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};
        rc = do_vault(&ch, str_hsh("get undefined.sword")); // get undefined.keyword
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};
        rc = do_vault(&ch, str_hsh("get undefined.missing")); // get undefined.missingkeyword
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};
        rc = do_vault(&ch, str_hsh("get 1.sword")); // get #.keyword
        QCOMPARE(conn.output, "a short sword has been removed from the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "a short sword has been placed in the vault.\r\n");
        conn.output = {};
        rc = do_vault(&ch, str_hsh("get 1.missing")); // get #.missingkeyword
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};
        rc = do_vault(&ch, str_hsh("get 3.sword")); // get bad#.keyword
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};
        rc = do_vault(&ch, str_hsh("get 3.missing")); // get bad#.missingkeyword
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};
        rc = do_vault(&ch, str_hsh("get 3.sword")); // get invalid#.keyword
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        rc = do_vault(&ch, str_hsh("put all"));
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn.output, "");
        conn.output = {};
        rc = do_vault(&ch, str_hsh("get 3.missing")); // get invalid#.missingkeyword
        QCOMPARE(conn.output, "There is nothing like that in the vault.\r\n");
        conn.output = {};

        remove_vault(ch.getNameC());
    }
};

QTEST_MAIN(TestDC)
#include "TestDC.moc"