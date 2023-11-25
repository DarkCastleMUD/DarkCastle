execute_process(COMMAND git describe --tags --always --abbrev=8 OUTPUT_VARIABLE BUILD_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND date "+%Y-%m-%d %H:%M:%S" OUTPUT_VARIABLE BUILD_TIME OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${BUILD_VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${BUILD_VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\-([0-9]+).*" "\\1" VERSION_PATCH "${BUILD_VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\-[0-9]+\\-g(.*)" "\\1" VERSION_SHA1 "${BUILD_VERSION}")
message("${BUILD_VERSION}")
message("Build timestamp: ${BUILD_TIME}")
message("Major: ${VERSION_MAJOR}")
message("Minor: ${VERSION_MINOR}")
message("Patch: ${VERSION_PATCH}")
message("SHA1: ${VERSION_SHA1}")

cmake_minimum_required(VERSION 3.14)
project(DC VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wno-write-strings -fdiagnostics-color=always -Werror -w")
#set(CMAKE_CXX_FLAGS "-O3 -Wfatal-errors -ggdb -Wno-write-strings -fdiagnostics-color=always")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${BUILD_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_GENERATOR RPM)
set(CPACK_THREADS 0)
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_STRIP_FILES FALSE)
set(CPACK_RPM_INSTALL_WITH_EXEC ON)
set(CPACK_RPM_PACKAGE_RELOCATABLE ON)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_REQUIRES qt6-sql-postgresql)
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/bin/qt.conf)

configure_file(version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/version.h)
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Network Sql DBus HttpServer Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql DBus HttpServer Concurrent)
add_library(DC SHARED
    act_comm.cpp
    act.cpp
    act_info.cpp
    alias.cpp
    arena.cpp
    assign_proc.cpp
    auction.cpp
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
    objects.cpp
    obj_proc.cpp
    offense.cpp
    quest.cpp
    save.cpp
    SSH.cpp
    shop.cpp
    Shops.cpp
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
    Zone.cpp
    act.h
    affect.h
    alias.h
    arena.h
    bandwidth.h
    character.h
    clan.h
    class.h
    combinables.h
    CommandStack.h
    comm.h
    common.h
    connect.h
    const.h
    corpse.h
    Database.h
    db.h
    DC.h
    DC_global.h
    dc_xmlrpc.h
    event.h
    fight.h
    fileinfo.h
    game_portal.h
    guild.h
    handler.h
    help.h
    innate.h
    interp.h
    inventory.h
    isr.h
    ki.h
    language.h
    Leaderboard.h
    levels.h
    magic.h
    memory.h
    MobActivity.h
    mobile.h
    move.h
    newedit.h
    news.h
    obj.h
    player.h
    punish.h
    quest.h
    race.h
    returnvals.h
    room.h
    set.h
    shop.h
    Shops.h
    sing.h
    social.h
    spells.h
    SSH.h
    stat.h
    structs.h
    terminal.h
    timeinfo.h
    Timer.h
    token.h
    Trace.h
    utility.h
    vault.h
    version.h
    weather.h
    wizard.h
    Zone.h
)
target_link_libraries(DC PRIVATE Qt${QT_VERSION_MAJOR}::Core pq fmt crypt ssh Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::HttpServer Qt${QT_VERSION_MAJOR}::Concurrent)
target_include_directories(DC INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(dcastle dcastle.cpp)
target_link_libraries(dcastle PRIVATE DC Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql)

qt_add_executable(debug debug.cpp)
target_link_libraries(debug PRIVATE DC Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql)

install (TARGETS dcastle debug
BUNDLE DESTINATION .
RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET dcastle
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})
include(CPack)