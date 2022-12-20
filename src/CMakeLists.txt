cmake_minimum_required(VERSION 3.16...3.25)
project(dcastle VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 -ggdb -Wno-write-strings -fdiagnostics-color=always")

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

execute_process(COMMAND git describe --tags --always --abbrev=8 OUTPUT_VARIABLE VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND date "+%Y-%m-%d %H:%M:%S" OUTPUT_VARIABLE BUILD_TIME OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
message("${VERSION}")
message("${BUILD_TIME}")
configure_file(version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/version.h)
add_library(DC STATIC
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
    SSH.cpp
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
target_link_libraries(DC PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network pq fmt crypt ssh)
target_include_directories(DC INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(dcastle dcastle.cpp)
qt_add_executable(debug debug.cpp)

target_link_libraries(dcastle PRIVATE DC)
target_link_libraries(debug PRIVATE DC)

set(DESTINATIONS /srv/dcastle_test/bin/ /srv/dcastle/bin/ /srv/dcastle2/bin/)
set(TARGETS dcastle debug)
foreach(D IN LISTS DESTINATIONS)
    foreach(T IN LISTS TARGETS)
        install(TARGETS ${T} RUNTIME DESTINATION ${D})
        install(CODE "execute_process(COMMAND cp -a ${D}/${T} ${D}/${T}.${VERSION})")
    endforeach()
endforeach()