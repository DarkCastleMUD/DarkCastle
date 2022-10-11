cmake_minimum_required(VERSION 3.16)

project(dcastle VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 -ggdb -Wno-write-strings -fdiagnostics-color=always")
find_package(Qt6 REQUIRED COMPONENTS Core)
execute_process(COMMAND git describe --tags --always --abbrev=8 OUTPUT_VARIABLE VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND date "+%Y-%m-%d %H:%M:%S" OUTPUT_VARIABLE BUILD_TIME OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
message("${VERSION}")
message("${BUILD_TIME}")
configure_file(version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/version.h)
add_library(DC
    act_comm.cpp
    act.cpp
    act_info.cpp
    alias.cpp
    arena.cpp
    assign_proc.cpp
    auction.cpp
    Backend.cpp
    ban.cpp
    board.cpp
    casino.cpp
    channel.cpp
    character.cpp
    clan.cpp
    class.cpp
    cl_barbarian.cpp
    cl_mage.cpp
    cl_monk.cpp
    cl_paladin.cpp
    cl_ranger.cpp
    cl_thief.cpp
    cl_warrior.cpp
    combinables.cpp
    CommandStack.cpp
    comm.cpp
    const.cpp
    corpse.cpp
    Database.cpp
    db.cpp
    DC.cpp
    fight.cpp
    fount.cpp
    game_portal.cpp
    golem.cpp
    group.cpp
    guild.cpp
    handler.cpp
    help.cpp
    info.cpp
    innate.cpp
    interp.cpp
    inventory.cpp
    ki.cpp
    Leaderboard.cpp
    limits.cpp
    magic.cpp
    memory.cpp
    meta.cpp
    mob_act.cpp
    MobActivity.cpp
    mob_commands.cpp
    mob_proc2.cpp
    mob_proc.cpp
    mob_prog.cpp
    modify.cpp
    move.cpp
    nanny.cpp
    new_edit.cpp
    news.cpp
    nlog.cpp
    non_off.cpp
    nullfile.cpp
    objects.cpp
    obj_proc.cpp
    offense.cpp
    quest.cpp
    save.cpp
    shop.cpp
    sing.cpp
    social.cpp
    spec.cpp
    spells.cpp
    stat.cpp
    Timer.cpp
    token.cpp
    Trace.cpp
    utility.cpp
    vault.cpp
    weather.cpp
    who.cpp
    wiz_101.cpp
    wiz_102.cpp
    wiz_103.cpp
    wiz_104.cpp
    wiz_105.cpp
    wiz_106.cpp
    wiz_107.cpp
    wiz_108.cpp
    wiz_109.cpp
    wiz_110.cpp
    wizard.cpp
)


add_executable(dcastle
dcastle.cpp
)

add_executable(debug
debug.cpp
)

target_link_libraries(dcastle PRIVATE DC Qt6::Core pq fmt crypt)
target_link_libraries(debug PRIVATE DC Qt6::Core pq fmt crypt)

install(PROGRAMS dcastle RENAME dcastle.${VERSION} DESTINATION /srv/dcastle_test/bin/)
install(PROGRAMS debug RENAME debug.${VERSION} DESTINATION /srv/dcastle_test/bin/)
install(CODE "execute_process(COMMAND ln -sf dcastle.${VERSION} /srv/dcastle_test/bin/dcastle)")
install(CODE "execute_process(COMMAND ln -sf debug.${VERSION} /srv/dcastle_test/bin/debug)")

install(PROGRAMS dcastle RENAME dcastle.${VERSION} DESTINATION /srv/dcastle/bin/)
install(PROGRAMS debug RENAME debug.${VERSION} DESTINATION /srv/dcastle/bin/)
install(CODE "execute_process(COMMAND ln -sf dcastle.${VERSION} /srv/dcastle/bin/dcastle)")
install(CODE "execute_process(COMMAND ln -sf debug.${VERSION} /srv/dcastle/bin/debug)")

install(PROGRAMS dcastle RENAME dcastle.${VERSION} DESTINATION /srv/dcastle2/bin/)
install(PROGRAMS debug RENAME debug.${VERSION} DESTINATION /srv/dcastle2/bin/)
install(CODE "execute_process(COMMAND ln -sf dcastle.${VERSION} /srv/dcastle2/bin/dcastle)")
install(CODE "execute_process(COMMAND ln -sf debug.${VERSION} /srv/dcastle2/bin/debug)")