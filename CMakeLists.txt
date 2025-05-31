# Usually compiled with one of the following commands:
# time (rm -rf build && cmake -S src -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER="/usr/bin/g++-15" -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" -GNinja && ninja -C build package)
# or
# cmake -S src -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="/usr/bin/g++-15" -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" -GNinja && ninja -C build
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
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable (tracy)

if (NOT "${CMAKE_BUILD_TYPE}" MATCHES "RelWithDebInfo" AND NOT "${CMAKE_BUILD_TYPE}" MATCHES "Release")
    set(CMAKE_CXX_COMPILER "/usr/bin/g++-15")
    add_library(tracy SHARED _deps/tracy-src/public/TracyClient.cpp)
    set(TRACY "tracy")
endif()

set(CMAKE_CXX_FLAGS "-Wno-write-strings -fdiagnostics-color=always -g3 -I_deps/tracy-src/public/")
set_source_files_properties(tests/testDC.cpp benchmarks/benchmarkDC.cpp PROPERTIES COMPILE_OPTIONS "-Wwrite-strings")
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

# Removed -fsanitize=leak
# Removed -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract
set(ASAN_COMMON_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize-address-use-after-scope -fsanitize=undefined -fno-omit-frame-pointer" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_ASAN "${ASAN_COMMON_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_ASAN "${ASAN_COMMON_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_ASAN "${ASAN_COMMON_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_ASAN "${ASAN_COMMON_FLAGS}" CACHE STRING "" FORCE)
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
configure_file(src/buildinfo.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/include/DC/buildinfo.h)
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Network Sql HttpServer Concurrent Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql HttpServer Concurrent Test)

find_program(CLANG_TIDY NAMES "clang-tidy" REQUIRED)
set(CLANG_TIDY_COMMAND "${CLANG_TIDY}" "-extra-arg=-std=c++23" "-extra-arg=-stdlib=libc++" "-checks=-*,modernize-*")

add_library(DC SHARED
    src/act_comm.cpp
    src/act.cpp
    src/act_info.cpp
    src/alias.cpp
    src/arena.cpp
    src/assign_proc.cpp
    src/auction.cpp
    src/ban.cpp
    src/board.cpp
    src/casino.cpp
    src/channel.cpp
    src/character.cpp
    src/clan.cpp
    src/class.cpp
    src/cl_barbarian.cpp
    src/cl_mage.cpp
    src/cl_monk.cpp
    src/cl_paladin.cpp
    src/cl_ranger.cpp
    src/cl_thief.cpp
    src/cl_warrior.cpp
    src/combinables.cpp
    src/CommandStack.cpp
    src/Command.cpp
    src/comm.cpp
    src/const.cpp
    src/corpse.cpp
    src/Database.cpp
    src/db.cpp
    src/DC.cpp
    src/fight.cpp
    src/fount.cpp
    src/game_portal.cpp
    src/golem.cpp
    src/group.cpp
    src/guild.cpp
    src/handler.cpp
    src/help.cpp
    src/info.cpp
    src/innate.cpp
    src/interp.cpp
    src/inventory.cpp
    src/ki.cpp
    src/Leaderboard.cpp
    src/limits.cpp
    src/magic.cpp
    src/memory.cpp
    src/meta.cpp
    src/mob_act.cpp
    src/MobActivity.cpp
    src/mob_commands.cpp
    src/mob_proc2.cpp
    src/mob_proc.cpp
    src/mob_prog.cpp
    src/modify.cpp
    src/move.cpp
    src/nanny.cpp
    src/new_edit.cpp
    src/news.cpp
    src/nlog.cpp
    src/non_off.cpp
    src/objects.cpp
    src/obj_proc.cpp
    src/offense.cpp
    src/quest.cpp
    src/save.cpp
    src/SSH.cpp
    src/shop.cpp
    src/Shops.cpp
    src/sing.cpp
    src/social.cpp
    src/spec.cpp
    src/spells.cpp
    src/stat.cpp
    src/Timer.cpp
    src/token.cpp
    src/Trace.cpp
    src/utility.cpp
    src/vault.cpp
    src/Version.cpp
    src/weather.cpp
    src/who.cpp
    src/wiz_101.cpp
    src/wiz_102.cpp
    src/wiz_103.cpp
    src/wiz_104.cpp
    src/wiz_105.cpp
    src/wiz_106.cpp
    src/wiz_107.cpp
    src/wiz_108.cpp
    src/wiz_109.cpp
    src/wiz_110.cpp
    src/wizard.cpp
    src/Zone.cpp
    src/include/DC/act.h
    src/include/DC/affect.h
    src/include/DC/alias.h
    src/include/DC/bandwidth.h
    src/include/DC/buildinfo.h
    src/include/DC/character.h
    src/include/DC/clan.h
    src/include/DC/class.h
    src/include/DC/combinables.h
    src/include/DC/CommandStack.h
    src/include/DC/comm.h
    src/include/DC/Command.h
    src/include/DC/common.h
    src/include/DC/connect.h
    src/include/DC/const.h
    src/include/DC/corpse.h
    src/include/DC/Database.h
    src/include/DC/db.h
    src/include/DC/DC.h
    src/include/DC/DC_global.h
    src/include/DC/dc_xmlrpc.h
    src/include/DC/event.h
    src/include/DC/fight.h
    src/include/DC/fileinfo.h
    src/include/DC/game_portal.h
    src/include/DC/guild.h
    src/include/DC/handler.h
    src/include/DC/help.h
    src/include/DC/innate.h
    src/include/DC/interp.h
    src/include/DC/inventory.h
    src/include/DC/isr.h
    src/include/DC/ki.h
    src/include/DC/language.h
    src/include/DC/Leaderboard.h
    src/include/DC/levels.h
    src/include/DC/magic.h
    src/include/DC/memory.h
    src/include/DC/MobActivity.h
    src/include/DC/mobile.h
    src/include/DC/move.h
    src/include/DC/newedit.h
    src/include/DC/news.h
    src/include/DC/obj.h
    src/include/DC/player.h
    src/include/DC/punish.h
    src/include/DC/quest.h
    src/include/DC/race.h
    src/include/DC/returnvals.h
    src/include/DC/room.h
    src/include/DC/set.h
    src/include/DC/shop.h
    src/include/DC/Shops.h
    src/include/DC/sing.h
    src/include/DC/social.h
    src/include/DC/spells.h
    src/include/DC/SSH.h
    src/include/DC/stat.h
    src/include/DC/structs.h
    src/include/DC/terminal.h
    src/include/DC/timeinfo.h
    src/include/DC/Timer.h
    src/include/DC/token.h
    src/include/DC/Trace.h
    src/include/DC/utility.h
    src/include/DC/vault.h
    src/include/DC/Version.h
    src/include/DC/weather.h
    src/include/DC/wizard.h
    src/include/DC/Zone.h
)
#set_target_properties(DC PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")

target_compile_definitions(DC PRIVATE $<$<CONFIG:Debug>:DEBUG_BUILD>)
target_include_directories(DC PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_link_libraries(DC PRIVATE Qt${QT_VERSION_MAJOR}::Core pq fmt crypt ssh Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::HttpServer Qt${QT_VERSION_MAJOR}::Concurrent)

qt_add_executable(dcastle src/dcastle.cpp)
target_compile_definitions(dcastle PRIVATE $<$<CONFIG:Debug>:DEBUG_BUILD>)
target_include_directories(dcastle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_link_libraries(dcastle PRIVATE DC Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql dl pthread ${TRACY})

qt_add_executable(debug src/debug.cpp)
target_compile_definitions(debug PRIVATE $<$<CONFIG:Debug>:DEBUG_BUILD>)
target_include_directories(debug PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_link_libraries(debug PRIVATE DC Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql dl pthread ${TRACY})

qt_add_executable(benchmarkDC src/benchmarks/benchmarkDC.cpp)
target_include_directories(benchmarkDC PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_link_libraries(benchmarkDC PRIVATE DC Qt${QT_VERSION_MAJOR}::Core pq fmt crypt ssh Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::HttpServer Qt${QT_VERSION_MAJOR}::Concurrent)
target_link_libraries(benchmarkDC PRIVATE Qt::Test)

enable_testing(true)
qt_add_executable(testDC src/tests/testDC.cpp)
add_test(NAME testDC COMMAND testDC)
target_include_directories(testDC PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_link_libraries(testDC PRIVATE DC Qt${QT_VERSION_MAJOR}::Core pq fmt crypt ssh Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::HttpServer Qt${QT_VERSION_MAJOR}::Concurrent)
target_link_libraries(testDC PRIVATE Qt::Test)

install (TARGETS dcastle debug testDC benchmarkDC
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install (DIRECTORY archive DESTINATION /var/lib/dcastle)
install (DIRECTORY blog DESTINATION /var/lib/dcastle)
install (DIRECTORY bsave DESTINATION /var/lib/dcastle)
install (DIRECTORY docs DESTINATION /var/lib/dcastle)
install (DIRECTORY familiar DESTINATION /var/lib/dcastle)
install (DIRECTORY follower DESTINATION /var/lib/dcastle)
install (DIRECTORY lib DESTINATION /var/lib/dcastle)
install (DIRECTORY log DESTINATION /var/lib/dcastle)
install (DIRECTORY save DESTINATION /var/lib/dcastle)
install (DIRECTORY scripts DESTINATION /var/lib/dcastle)
install (DIRECTORY vaults DESTINATION /var/lib/dcastle)
qt_generate_deploy_app_script(
    TARGET dcastle
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})
include(CPack)