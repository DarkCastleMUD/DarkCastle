# Usually compiled with one of the following commands:
# time (rm -rf build && cmake -S src -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER="/usr/bin/g++-14" -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" -GNinja && ninja -C build package)
# or
# cmake -S src -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="/usr/bin/g++-14" -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" -GNinja && ninja -C build
cmake_minimum_required(VERSION 3.14)
#set these two options to ON when using tracy
option(TRACY_ENABLE "" OFF)
#option(TRACY_NO_EXIT "" ON)
#option(TRACY_ON_DEMAND "" ON)
#option(TRACY_ONLY_LOCALHOST "" ON)
#option(TRACY_ONLY_IPV4 "" ON)
include(FetchContent)
FetchContent_Declare(
  tracy
  GIT_REPOSITORY https://github.com/wolfpld/tracy.git
  GIT_TAG v0.10
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable (tracy)
set(CMAKE_CXX_COMPILER "/usr/bin/g++-14")
add_library(tracy SHARED _deps/tracy-src/public/TracyClient.cpp)

set(CMAKE_CXX_FLAGS "-Wno-write-strings -fdiagnostics-color=always -g3 -I_deps/tracy-src/public/")
set_source_files_properties(tests/TestDC.cpp PROPERTIES COMPILE_OPTIONS "-Wwrite-strings")
#-Wpedantic -Wall -Wextra -Wno-change-meaning -Wno-coverage-mismatch -Wno-coverage-invalid-line-number -Wdouble-promotion -Wformat -Wformat-nonliteral -Wformat-overflow -Wformat-security -Wformat-signedness -Wformat-truncation -Wwrite-strings -Wformat-y2k -Wnonnull -Wnonnull-compare -Wnull-dereference -Winit-self -Wfatal-errors -Wdangling-reference
#set(CMAKE_CXX_FLAGS "-Wno-write-strings -fdiagnostics-color=always -Werror -Wfatal-errors -w -g3 -Wdangling-reference -Wextra -I_deps/tracy-src/public/ -DTRACY_ENABLE -DTRACY_ON_DEMAND")
# -DTRACY_ENABLE -DTRACY_ON_DEMAND -DTRACY_NO_EXIT
# -p    prof
# -pg   gprof
# -fsanitize=address
# -Og
# -Wdangling-gsl for clang

# https://stackoverflow.com/a/64294837

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(isMultiConfig)
    if(NOT "Asan" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES SanitizeAddress)
    endif()
else()
    set(allowedBuildTypes Asan Debug Release RelWithDebInfo MinSizeRel)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${allowedBuildTypes}")

    if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST allowedBuildTypes)
        message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
    endif()
endif()

set(CMAKE_C_FLAGS_ASAN
    "${CMAKE_C_FLAGS_DEBUG} -fsanitize-address-use-after-scope -fsanitize=undefined -fsanitize=leak -fsanitize=address -fno-omit-frame-pointer -fsanitize=pointer-compare -fsanitize=pointer-subtract" CACHE STRING
    "Flags used by the C compiler for Asan build type or configuration." FORCE)

set(CMAKE_CXX_FLAGS_ASAN
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize-address-use-after-scope -fsanitize=undefined -fsanitize=leak -fsanitize=address -fno-omit-frame-pointer -fsanitize=pointer-compare -fsanitize=pointer-subtract" CACHE STRING
    "Flags used by the C++ compiler for Asan build type or configuration." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_ASAN
    "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize-address-use-after-scope -fsanitize=undefined -fsanitize=leak -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract" CACHE STRING
    "Linker flags to be used to create executables for Asan build type." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_ASAN
    "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize-address-use-after-scope -fsanitize=undefined -fsanitize=leak -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract" CACHE STRING
    "Linker lags to be used to create shared libraries for Asan build type." FORCE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${BUILD_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_GENERATOR RPM)
set(CPACK_THREADS 0)
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_STRIP_FILES FALSE)
set(CPACK_RPM_INSTALL_WITH_EXEC ON)
set(CPACK_RPM_PACKAGE_RELOCATABLE ON)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-sql-postgresql qt6-sql-sqlite")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/bin/qt.conf)

execute_process(COMMAND git describe --tags --always --abbrev=8 OUTPUT_VARIABLE BUILD_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND date "+%Y-%m-%d %H:%M:%S" OUTPUT_VARIABLE BUILD_TIME OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${BUILD_VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${BUILD_VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\-([0-9]+).*" "\\1" VERSION_PATCH "${BUILD_VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\-[0-9]+\\-g(.*)" "\\1" VERSION_SHA1 "${BUILD_VERSION}")
message("${CMAKE_BUILD_TYPE}")
message("${CMAKE_GENERATOR}")
message("${BUILD_VERSION}")
message("Build timestamp: ${BUILD_TIME}")
message("Major: ${VERSION_MAJOR}")
message("Minor: ${VERSION_MINOR}")
message("Patch: ${VERSION_PATCH}")
message("SHA1: ${VERSION_SHA1}")

project(DC VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} LANGUAGES CXX)
configure_file(buildinfo.h.in ${CMAKE_CURRENT_SOURCE_DIR}/buildinfo.h)
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Network Sql DBus HttpServer Concurrent Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql DBus HttpServer Concurrent Test)

find_program(CLANG_TIDY NAMES "clang-tidy" REQUIRED)
set(CLANG_TIDY_COMMAND "${CLANG_TIDY}" "-extra-arg=-std=c++23" "-extra-arg=-stdlib=libc++" "-checks=-*,modernize-*")

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
    Command.cpp
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
    Version.cpp
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
    bandwidth.h
    buildinfo.h
    character.h
    clan.h
    class.h
    combinables.h
    CommandStack.h
    comm.h
    Command.h
    common.h
    connect.h
    const.h
    corpse.h
    Database.h
    include/DC/db.h
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
    Version.h
    weather.h
    wizard.h
    Zone.h
)
#set_target_properties(DC PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")

target_compile_definitions(DC PRIVATE $<$<CONFIG:Debug>:DEBUG_BUILD>)
target_include_directories(DC PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(DC PRIVATE Qt${QT_VERSION_MAJOR}::Core pq fmt crypt ssh Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::HttpServer Qt${QT_VERSION_MAJOR}::Concurrent)


qt_add_executable(dcastle dcastle.cpp)
target_compile_definitions(dcastle PRIVATE $<$<CONFIG:Debug>:DEBUG_BUILD>)
target_include_directories(dcastle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dcastle PRIVATE DC Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql dl pthread tracy)

qt_add_executable(debug debug.cpp)
target_compile_definitions(debug PRIVATE $<$<CONFIG:Debug>:DEBUG_BUILD>)
target_include_directories(debug PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(debug PRIVATE DC Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql dl pthread tracy)

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

enable_testing(true)
qt_add_executable(testDC tests/TestDC.cpp)
add_test(NAME testDC COMMAND testDC)
target_include_directories(testDC PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(testDC PRIVATE DC Qt${QT_VERSION_MAJOR}::Core pq fmt crypt ssh Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::HttpServer Qt${QT_VERSION_MAJOR}::Concurrent)
target_link_libraries(testDC PRIVATE Qt::Test)

qt_add_executable(benchmarkDC benchmarks/benchmark_utility.cpp)
target_include_directories(benchmarkDC PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(benchmarkDC PRIVATE DC Qt${QT_VERSION_MAJOR}::Core pq fmt crypt ssh Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::HttpServer Qt${QT_VERSION_MAJOR}::Concurrent)
target_link_libraries(benchmarkDC PRIVATE Qt::Test)
