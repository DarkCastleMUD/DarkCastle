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
#include "DC/terminal.h"

using namespace std::literals;

#define STRING_LITERAL1 "$00$11$22$33$44$55$66$77$88$99$II$LL$**$RR$BB$$"
#define STRING_LITERAL1_NOCOLOR "0123456789IL*RB$"
#define STRING_LITERAL1_COLOR BLACK "0" BLUE "1" GREEN "2" CYAN "3" RED "4" YELLOW "5" PURPLE "6" GREY "7" \
                                    "8"                                                                    \
                                    "9" INVERSE "I" FLASH "L"                                              \
                                    "*" NTEXT "R" BOLD "B"                                                 \
                                    "$"
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
        qSetMessagePattern(QStringLiteral("%{if-category}%{category}:%{endif}%{function}:%{line}:%{message}"));
    }
    QByteArray checksumFile(QString filename)
    {
        QFile file(filename);
        file.open(QIODeviceBase::ReadOnly);
        QCryptographicHash file_hash(QCryptographicHash::Algorithm::Sha512);
        file_hash.addData(file.readAll());
        return file_hash.result();
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

    void test_handle_ansi()
    {
        std::unique_ptr<Character> ch = std::make_unique<Character>();
        std::unique_ptr<Player> player = std::make_unique<Player>();
        ch->player = player.get();
        ch->do_toggle({"ansi"});
        QVERIFY(isSet(ch->player->toggles, Player::PLR_ANSI));
        QCOMPARE(handle_ansi(QStringLiteral(STRING_LITERAL1), ch.get()), QStringLiteral(STRING_LITERAL1_COLOR));

        ch->do_toggle({"ansi"});
        QVERIFY(!isSet(ch->player->toggles, Player::PLR_ANSI));
        QCOMPARE(handle_ansi(QStringLiteral(STRING_LITERAL1), ch.get()), QStringLiteral(STRING_LITERAL1_NOCOLOR));
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
        dc.boot_db();
        dc.random_ = QRandomGenerator(0);

        Character ch;
        ch.setName(QStringLiteral("Testsing"));
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
        QVERIFY(ch.isMortalPlayer());
        QVERIFY(!ch.isImmortalPlayer());
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
        QCOMPARE(conn.output, "You feel more competent in your flight of the bumblebee ability. It increased to 2 out of 75.\r\nYou forgot the words!\r\n");
        conn.output = {};
        QVERIFY(ch.songs.empty());

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

        QCOMPARE(dc.character_list.erase(&ch), 1);
        ch.desc = nullptr;
        ch.player = nullptr;
    }

    void test_do_vault_get()
    {
        DC::config cf;
        cf.sql = false;

        DC dc(cf);
        dc.boot_db();
        dc.random_ = QRandomGenerator(0);

        Character ch;
        ch.setName(QStringLiteral("Testvault"));
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
        QCOMPARE(conn.output, "Your gain is: 11/14 hp, 1/1 m, 1/21 mv, 0/0 prac, 1/1 ki.\r\nYour gain is: 12/26 hp, 1/2 m, 1/22 mv, 0/0 prac, 0/1 ki.\r\nYour gain is: 10/36 hp, 1/3 m, 1/23 mv, 0/0 prac, 1/2 ki.\r\nYour gain is: 14/50 hp, 1/4 m, 1/24 mv, 0/0 prac, 0/2 ki.\r\nYour gain is: 14/64 hp, 1/5 m, 1/25 mv, 0/0 prac, 1/3 ki.\r\nYour gain is: 13/77 hp, 1/6 m, 1/26 mv, 0/0 prac, 0/3 ki.\r\nYou are now able to participate in pkilling!\n\rRead HELP PKILL for more information.\r\nYour gain is: 12/89 hp, 1/7 m, 1/27 mv, 0/0 prac, 1/4 ki.\r\nYour gain is: 11/100 hp, 1/8 m, 1/28 mv, 0/0 prac, 0/4 ki.\r\nYour gain is: 13/113 hp, 1/9 m, 1/29 mv, 0/0 prac, 1/5 ki.\r\nYour gain is: 13/126 hp, 1/10 m, 1/30 mv, 0/0 prac, 0/5 ki.\r\nYou have been given a vault in which to place your valuables!\n\rRead HELP VAULT for more information.\r\n");
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

        QCOMPARE(dc.character_list.erase(&ch), 1);
        ch.desc = nullptr;
        ch.player = nullptr;
    }

    void test_fread()
    {
        DC::config cf;
        cf.sql = false;
        DC dc(cf);

        QFile testfile(QStringLiteral("fread_tests.txt"));
        testfile.open(QIODeviceBase::WriteOnly);
        testfile.write("3\n");
        testfile.write("-1\n");
        testfile.write("abc\n123~\n");
        testfile.write("abc123~\n");
        testfile.write("~\n");
        testfile.write("\n");
        testfile.close();

        FILE *stream = fopen(qPrintable(testfile.fileName()), "r");
        QVERIFY(stream);
        QCOMPARE(ftell(stream), 0);

        bool error_range_over_raised = false;
        int val1 = 3333333;
        try
        {
            val1 = fread_int(stream, 0, 0);
        }
        catch (error_range_over)
        {
            error_range_over_raised = true;
        }
        QVERIFY(error_range_over_raised);
        QCOMPARE(val1, 3333333);
        QCOMPARE(ftell(stream), 2);

        QCOMPARE(fseek(stream, 0, SEEK_SET), 0);
        bool error_range_under_raised = false;
        val1 = 3333333;
        try
        {
            val1 = fread_int(stream, 10, 20);
        }
        catch (error_range_under)
        {
            error_range_under_raised = true;
        }
        QVERIFY(error_range_under_raised);
        QCOMPARE(val1, 3333333);
        QCOMPARE(ftell(stream), 2);

        QCOMPARE(fseek(stream, 0, SEEK_SET), 0);
        int val2 = fread_int(stream, -10, 10);
        QCOMPARE(val2, 3);

        int val3 = fread_int(stream, -10, 10);
        QCOMPARE(val3, -1);

        QString str1 = fread_qstring(stream);
        QCOMPARE(str1, QStringLiteral("abc\r\n123"));

        QString str2 = fread_qstring(stream);
        QCOMPARE(str2, QStringLiteral("abc123"));

        QString str3 = fread_qstring(stream);
        QCOMPARE(str3, QStringLiteral(""));

        fclose(stream);
        testfile.remove();
    }

    void test_room_write()
    {
        DC::config cf;
        cf.sql = false;

        DC dc(cf);
        dc.boot_db();
        dc.random_ = QRandomGenerator(0);

        QString filename;
        if (dc.world_file_list)
        {
            filename = dc.world_file_list->filename;
        }
        else
        {
            filename = "1-1.txt";
        }

        QString legacyfile_filename = QStringLiteral("world/%1.legacyfile").arg(filename);
        QString qfile_filename = QStringLiteral("world/%1.qfile").arg(filename);
        QString qsavefile_filename = QStringLiteral("world/%1.qsavefile").arg(filename);
        QString fstream_filename = QStringLiteral("world/%1.fstream").arg(filename);
        uint64_t rooms_written{};
        {
            LegacyFileWorld lfw(QStringLiteral("%1.legacyfile").arg(filename));
            QFile qf(qfile_filename);
            QSaveFile qsf(qsavefile_filename);
            std::fstream fstream_world_file;
            fstream_world_file.open(fstream_filename.toStdString(), std::ios::out);
            QVERIFY(lfw.isOpen());
            QVERIFY(qf.open(QIODeviceBase::WriteOnly));
            QVERIFY(qsf.open(QIODeviceBase::WriteOnly));
            QVERIFY(fstream_world_file.is_open());

            QTextStream out(&qf);
            QTextStream out2(&qsf);

            if (dc.world_file_list)
            {
                for (int x = dc.world_file_list->firstnum; x <= dc.world_file_list->lastnum; x++)
                {
                    write_one_room(lfw, x);
                    out << DC::getInstance()->world[x];
                    out2 << DC::getInstance()->world[x];
                    fstream_world_file << DC::getInstance()->world[x];
                    rooms_written++;
                }
            }
            else
            {
                write_one_room(lfw, 1);
                out << DC::getInstance()->world[1];
                out2 << DC::getInstance()->world[1];
                fstream_world_file << DC::getInstance()->world[1];
                rooms_written = 1;
            }

            out << "$~\n";
            out2 << "$~\n";
            qsf.commit();
            fstream_world_file << "$~\n";
        }

        qInfo("Wrote %d rooms to '%s'.", rooms_written, qPrintable(filename));

        auto original_checksum = checksumFile(QStringLiteral("world/%1").arg(filename));
        auto legacyfile_checksum = checksumFile(legacyfile_filename);
        auto qfile_checksum = checksumFile(qfile_filename);
        auto qsavefile_checksum = checksumFile(qsavefile_filename);
        auto fstream_checksum = checksumFile(fstream_filename);

        QCOMPARE(legacyfile_checksum.toHex(), original_checksum.toHex());
        QCOMPARE(qfile_checksum.toHex(), original_checksum.toHex());
        QCOMPARE(qsavefile_checksum.toHex(), original_checksum.toHex());
        QCOMPARE(fstream_checksum.toHex(), original_checksum.toHex());

        {
            FILE *fl = fopen(qPrintable(legacyfile_filename), "r");
            QVERIFY(fl);

            QFile qf(qfile_filename);
            QVERIFY(qf.open(QIODeviceBase::ReadOnly));

            std::fstream fstream_world_file;
            fstream_world_file.open(fstream_filename.toStdString(), std::ios::in);
            QVERIFY(fstream_world_file.is_open());

            QTextStream in(&qf);
            int room_nr = {};

            Room original_room1 = DC::getInstance()->world[1];
            int new_room_nr = DC::getInstance()->read_one_room(fl, room_nr);
            QCOMPARE(room_nr, 1);
            QCOMPARE(new_room_nr, 1);
            Room new_room1 = DC::getInstance()->world[1];
            QCOMPARE(new_room1, original_room1);

            // Room r1;
            // in >> r1;
            // QCOMPARE(r1, original_room1);
            //   fstream_world_file >> r2;
            //   QCOMPARE(r2, original_room1);
        }

        QVERIFY(QFile(legacyfile_filename).remove());
        QVERIFY(QFile(qfile_filename).remove());
        QVERIFY(QFile(qsavefile_filename).remove());
        QVERIFY(QFile(fstream_filename).remove());
    }

    void test_do_vend()
    {
        DC::config cf;
        cf.sql = false;

        DC dc(cf);
        dc.boot_db();
        dc.random_ = QRandomGenerator(0);
        auto base_character_count = dc.character_list.size();

        Character ch;
        ch.setName(QStringLiteral("Testvend"));
        ch.in_room = 3;
        ch.height = 72;
        ch.weight = 150;
        ch.setClass(CLASS_WARRIOR);
        Player player;
        ch.player = &player;
        Connection conn;
        dc.descriptor_list = &conn;
        conn.descriptor = 1;
        conn.character = &ch;
        ch.desc = &conn;
        dc.character_list.insert(&ch);
        QCOMPARE(dc.character_list.count(&ch), 1);
        QCOMPARE(dc.character_list.size(), base_character_count + 1);
        ch.do_on_login_stuff();
        while (ch.getLevel() < 10)
        {
            ch.incrementLevel();
            advance_level(&ch, 0);
        }
        ch.setMove(GET_MAX_MOVE(&ch));
        conn.output = {};

        Character ch2;
        ch2.setName(QStringLiteral("Testvend2"));
        ch2.in_room = 3;
        ch2.height = 72;
        ch2.weight = 150;
        ch2.setClass(CLASS_WARRIOR);
        Player player2;
        ch2.player = &player2;
        Connection conn2;
        dc.descriptor_list->next = &conn2;
        conn2.descriptor = 1;
        conn2.character = &ch2;
        ch2.desc = &conn2;
        dc.character_list.insert(&ch2);
        QCOMPARE(dc.character_list.size(), base_character_count + 2);
        ch2.do_on_login_stuff();
        while (ch2.getLevel() < 10)
        {
            ch2.incrementLevel();
            advance_level(&ch2, 0);
        }
        ch2.setMove(GET_MAX_MOVE(&ch2));
        conn2.output = {};

        auto rc = do_channel(&ch, str_hsh("auction"));
        QCOMPARE(conn.output, "auction channel turned ON.\r\n");
        conn.output = {};
        QCOMPARE(rc, eSUCCESS);

        rc = do_channel(&ch2, str_hsh("auction"));
        QCOMPARE(conn2.output, "auction channel turned ON.\r\n");
        conn2.output = {};
        QCOMPARE(rc, eSUCCESS);

        rc = ch.do_auction({QStringLiteral("test")});
        QCOMPARE(conn.output, "");
        conn.output = {};
        QCOMPARE(conn2.output, "");
        conn2.output = {};
        QCOMPARE(rc, eSUCCESS);

        auto new_mob_rnum = real_mobile(5258);
        QVERIFY(new_mob_rnum != -1);

        auto new_rnum = create_blank_item(1);
        QCOMPARE(new_rnum.error(), create_error::entry_exists);
        int rnum = real_object(1);
        Object *o1 = clone_object(rnum);
        Object *o2 = clone_object(rnum);
        QVERIFY(o1);
        QVERIFY(o2);
        GET_OBJ_NAME(o1) = str_hsh("sword");
        GET_OBJ_SHORT(o1) = str_hsh("a short sword");

        rc = do_vend(&ch, str_hsh(""));
        QCOMPARE(conn.output, "You must be in an auction house to do this!\r\n");
        conn.output = {};
        QCOMPARE(rc, eFAILURE);

        rc = move_char(&ch, 5200);
        QCOMPARE(conn.output, "");
        QCOMPARE(rc, eSUCCESS);
        rc = move_char(&ch2, 5200);
        QCOMPARE(conn.output, "");
        QCOMPARE(rc, eSUCCESS);

        auto items_posted_qty = dc.TheAuctionHouse.getItemsPosted();

        rc = do_vend(&ch, str_hsh(""));
        QCOMPARE(conn.output, "Syntax: vend <buy | sell | list | cancel | modify | collect | search | identify>\r\n");
        conn.output = {};
        QCOMPARE(rc, eSUCCESS);

        rc = do_vend(&ch, str_hsh("sell sword"));
        QCOMPARE(conn.output, "You don't seem to have that item.\n\rSyntax: vend sell <item> <price> [person]\r\n");
        conn.output = {};
        QCOMPARE(rc, eSUCCESS);

        auto status = obj_to_char(o1, &ch);
        QVERIFY(status);

        rc = do_vend(&ch, str_hsh("sell sword"));
        QCOMPARE(conn.output, "How much do you want to sell it for?\n\rSyntax: vend sell <item> <price> [person]\r\n");
        conn.output = {};
        QCOMPARE(rc, eSUCCESS);

        rc = do_vend(&ch, str_hsh("sell sword 1000"));
        QCOMPARE(conn.output, "The Consignment broker informs you that he does not handle items that have been restrung.\r\n");
        conn.output = {};
        QCOMPARE(rc, eSUCCESS);

        extract_obj(o1);

        status = obj_to_char(o2, &ch);
        QVERIFY(status);

        QCOMPARE(conn2.output, "");
        rc = do_vend(&ch, str_hsh("sell item 1000000"));
        QCOMPARE(conn.output, "You are now selling a reflecty test item for 1000000 coins.\r\n"
                              "Saving Testvend.\r\n");
        conn.output = {};
        QCOMPARE(rc, eSUCCESS);
        QCOMPARE(conn2.output, "");

        rc = do_vend(&ch, str_hsh("list mine"));
        QCOMPARE(conn.output, "Ticket-Buyer--------Price------Status--T--Item---------------------------\r\n\n\r"
                              "You are using 1 of your 1 available tickets.\r\n\n\r"
                              "00002)              1,000,000  PUBLIC     a reflecty test item          \n\r\n\r"
                              "'N' indicates an item is NO_TRADE and requires a Genuine Wendy Wingding to purchase.\r\n"
                              "'*' indicates you are unable to use this item.\r\n");
        conn.output = {};
        QCOMPARE(rc, eSUCCESS);

        rc = do_vend(&ch, str_hsh("cancel 2"));
        QCOMPARE(conn.output, "The Consignment Broker retrieves a reflecty test item and returns it to you.\r\nSaving Testvend.\r\n");
        conn.output = {};
        QCOMPARE(rc, eSUCCESS);

        rc = do_vend(&ch, str_hsh("list mine"));
        QCOMPARE(conn.output, "Ticket-Buyer--------Price------Status--T--Item---------------------------\r\n\n\r"
                              "You do not have any tickets.\r\n\n\r"
                              "You are using 0 of your 1 available tickets.\r\n");
        conn.output = {};
        QCOMPARE(rc, eSUCCESS);

        dc.TheAuctionHouse.setItemsPosted(items_posted_qty);
        dc.TheAuctionHouse.Save();

        QCOMPARE(dc.character_list.erase(&ch), 1);
        QCOMPARE(dc.character_list.count(&ch), 0);
        QCOMPARE(dc.character_list.erase(&ch2), 1);
        QCOMPARE(dc.character_list.count(&ch2), 0);
        ch.desc = nullptr;
        ch.player = nullptr;
        ch2.desc = nullptr;
        ch2.player = nullptr;
    }

    void test_do_medit()
    {
        DC::config cf;
        cf.sql = false;

        DC dc(cf);
        dc.boot_db();
        dc.random_ = QRandomGenerator(0);
        auto base_character_count = dc.character_list.size();

        Character ch;
        ch.setName(QStringLiteral("Test"));
        Player player;
        ch.player = &player;
        Connection conn;
        dc.descriptor_list = &conn;
        conn.descriptor = 1;
        conn.character = &ch;
        ch.desc = &conn;
        dc.character_list.insert(&ch);
        conn.output = {};

        auto rc = do_medit(&ch, str_hsh(""));
        QCOMPARE(conn.output, "Syntax:  medit [mob_num] [field] [arg]\r\n  Edit a mob_num with no field or arg to view the item.\r\n  Edit a field with no args for help on that field.\r\n\r\nThe field must be one of the following:\n\r          keywords         shortdesc          longdesc       description\r\n               sex             class              race             level\r\n         alignment      loadposition   defaultposition          actflags\r\n       affectflags        numdamdice       sizedamdice           damroll\r\n           hitroll       hphitpoints              gold  experiencepoints\r\n            immune           suscept            resist        armorclass\r\n              stat          strength         dexterity      intelligence\r\n            wisdom      constitution               new            delete\r\n              type                v1                v2                v3\r\n                v4\r\n");
        conn.output = {};
        ch.player->last_mob_edit = {};
        QCOMPARE(rc, eFAILURE);

        rc = do_medit(&ch, str_hsh("0"));
        QCOMPARE(conn.output, "0 is an invalid mob vnum.\r\n");
        conn.output = {};
        ch.player->last_mob_edit = {};
        QCOMPARE(rc, eFAILURE);

        rc = do_medit(&ch, str_hsh("-1"));
        QCOMPARE(conn.output, "-1 is an invalid mob vnum.\r\n");
        conn.output = {};
        ch.player->last_mob_edit = {};
        QCOMPARE(rc, eFAILURE);

        rc = do_medit(&ch, str_hsh("1"));
        QCOMPARE(conn.output, "Changing last mob vnum from 0 to 1.\r\nMOB - Name: [chain]  VNum: 1  RNum: 0  In room: -1 Mobile type: NORMAL\n\rShort description: Chain\n\rTitle: None\n\rLong description: Chain is here, looking for ideas to steal.\r\nDetailed description:\r\nKevin looks like he's between the ages of 22-24.  He is picking his nose.\r\nEvery few seconds he types \"score\" then he jots down some notes.  He\r\nappears to be reading as many help files as he can find.  He also seems\r\ninterested in finding a copy of the DC code, and is keeping an eye out for\r\nany Imps that might be nearby.\r\n\r\nClass: Mage   Level:[105] Alignment:[0] Spelldamage:[30] Race: Rodent\r\nMobspec: exists  Progtypes: 25611\r\nHeight:[198]  Weight:[200]  Sex:[FEMALE]  Hometown:[3001]\n\rStr:[15]+[ 0]=15 Int:[15]+[ 0]=15 Wis:[10]+[ 0]=10\r\nDex:[20]+[ 0]=20 Con:[20]+[ 0]=20\n\rMana:[ 1150/ 1150+27  ]  Hit:[ 4000/ 4000+166]  Move:[ 1150/ 1150+105]  Ki:[175/175]\n\rAC:[-40]  Exp:[0]  Hitroll:[21]  Damroll:[33]  Gold: [0]\n\rPosition: Standing  Fighting: Nobody  Default position: Standing  Timer:[0] \n\rNPC flags: [134217731 0]SPEC SENTINEL NOMATRIX \n\rNon-Combat Special Proc: exists  Combat Special Proc: none  Mob Progs: exists\r\nNPC Bare Hand Damage: 0d0.\r\nCarried weight: 0   Carried items: 0\n\rItems in inventory: 0  Items in equipment: 0\n\rSave Vs: FIRE[35] COLD[35] ENERGY[35] ACID[35] MAGIC[35] POISON[-15]\n\rThirst: -1  Hunger: -1  Drunk: -1\n\rMelee: [0] Spell: [0] Song: [0] Reflect: [0]\r\nTracking: 'NOBODY'\n\rHates: 'NOBODY'\n\rFears: 'NOBODY'\n\rMaster: 'NOBODY'\n\rFollowers:\r\nCombat flags: NoBits \n\rAffected by: [35914280 0] DETECT-INVISIBLE SENSE-LIFE EAS true-SIGHT INFARED \r\nImmune: [3669751] PIERCE SLASH MAGIC FIRE ENERGY ACID POISON COLD PARA BLUDGEON WHIP CRUSH HIT BITE STING CLAW PHYSICAL KI SONG \n\rSusceptible: [128] POISON \n\rResistant: [0] NoBits \n\rLag Left:  0\r\n");
        conn.output = {};
        ch.player->last_mob_edit = {};
        QCOMPARE(rc, eSUCCESS);

        rc = do_medit(&ch, str_hsh("abc"));
        QCOMPARE(conn.output, "Invalid field.\r\n");
        conn.output = {};
        ch.player->last_mob_edit = {};
        QCOMPARE(rc, eFAILURE);
    }
};

QTEST_MAIN(TestDC)
#include "testDC.moc"