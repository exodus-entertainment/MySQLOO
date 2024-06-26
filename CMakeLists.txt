cmake_minimum_required(VERSION 3.5)
project(mysqloo)
add_subdirectory(GmodLUA)

file(GLOB_RECURSE MYSQLOO_SRC "src/*.h" "src/*.cpp")
set(SOURCE_FILES ${MYSQLOO_SRC})
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set (CMAKE_CXX_STANDARD 14)

add_library(mysqloo SHARED ${SOURCE_FILES})
target_link_libraries(mysqloo gmod-module-base)

target_include_directories(mysqloo PRIVATE MySQL/include)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (WIN32)
        find_library(MARIADB_CLIENT_LIB mariadbclient HINTS "${PROJECT_SOURCE_DIR}/MySQL/lib64/windows")
    else ()
        find_library(MARIADB_CLIENT_LIB mariadbclient HINTS "${PROJECT_SOURCE_DIR}/MySQL/lib64/linux")
        find_library(CRYPTO_LIB crypto HINTS "${PROJECT_SOURCE_DIR}/MySQL/lib64/linux")
        find_library(SSL_LIB ssl HINTS "${PROJECT_SOURCE_DIR}/MySQL/lib64/linux")
    endif ()
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    if (WIN32)
        find_library(MARIADB_CLIENT_LIB mariadbclient HINTS "${PROJECT_SOURCE_DIR}/MySQL/lib/windows")
    else ()
        find_library(MARIADB_CLIENT_LIB mariadbclient HINTS "${PROJECT_SOURCE_DIR}/MySQL/lib/linux")
        find_library(CRYPTO_LIB crypto HINTS "${PROJECT_SOURCE_DIR}/MySQL/lib/linux")
        find_library(SSL_LIB ssl HINTS "${PROJECT_SOURCE_DIR}/MySQL/lib/linux")
    endif ()
endif ()

if (WIN32)
    target_link_libraries(mysqloo ${MARIADB_CLIENT_LIB} crypt32 ws2_32 shlwapi bcrypt secur32)
else ()
    find_package(Threads REQUIRED)
    target_link_libraries(mysqloo ${MARIADB_CLIENT_LIB} ${SSL_LIB} ${CRYPTO_LIB} Threads::Threads ${CMAKE_DL_LIBS})
    target_link_libraries(mysqloo -static-libstdc++)
endif ()

set_gmod_suffix_prefix(mysqloo)