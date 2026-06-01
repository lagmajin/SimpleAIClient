cmake_minimum_required(VERSION 3.16)
project(SimpleAIClient VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(NOT Qt6_DIR)
    set(_qt6_hint_paths)

    if(DEFINED ENV{Qt6_DIR} AND EXISTS "$ENV{Qt6_DIR}/Qt6Config.cmake")
        list(APPEND _qt6_hint_paths "$ENV{Qt6_DIR}")
    endif()

    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        list(APPEND _qt6_hint_paths "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/Qt6")
    endif()

    if(DEFINED CMAKE_TOOLCHAIN_FILE)
        get_filename_component(_vcpkg_buildsystems_dir "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
        get_filename_component(_vcpkg_scripts_dir "${_vcpkg_buildsystems_dir}" DIRECTORY)
        get_filename_component(_vcpkg_root_dir "${_vcpkg_scripts_dir}" DIRECTORY)
        list(APPEND _qt6_hint_paths "${_vcpkg_root_dir}/installed/x64-windows/share/Qt6")
    endif()

    foreach(_qt6_hint IN LISTS _qt6_hint_paths)
        if(EXISTS "${_qt6_hint}/Qt6Config.cmake")
            set(Qt6_DIR "${_qt6_hint}" CACHE PATH "Path to Qt6Config.cmake" FORCE)
            break()
        endif()
    endforeach()
endif()

if(NOT httplib_DIR)
    set(_httplib_hint_paths)

    if(DEFINED ENV{httplib_DIR} AND EXISTS "$ENV{httplib_DIR}/httplibConfig.cmake")
        list(APPEND _httplib_hint_paths "$ENV{httplib_DIR}")
    endif()

    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        list(APPEND _httplib_hint_paths "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/httplib")
    endif()

    if(DEFINED CMAKE_TOOLCHAIN_FILE)
        get_filename_component(_vcpkg_buildsystems_dir "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
        get_filename_component(_vcpkg_scripts_dir "${_vcpkg_buildsystems_dir}" DIRECTORY)
        get_filename_component(_vcpkg_root_dir "${_vcpkg_scripts_dir}" DIRECTORY)
        list(APPEND _httplib_hint_paths "${_vcpkg_root_dir}/installed/x64-windows/share/httplib")
    endif()

    foreach(_httplib_hint IN LISTS _httplib_hint_paths)
        if(EXISTS "${_httplib_hint}/httplibConfig.cmake")
            set(httplib_DIR "${_httplib_hint}" CACHE PATH "Path to httplibConfig.cmake" FORCE)
            break()
        endif()
    endforeach()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia)
find_package(httplib CONFIG REQUIRED)

qt_standard_project_setup()

qt_add_executable(SimpleAIClient
    src/main.cpp
    src/mainwindow.cpp
    src/apiclient.cpp
)

target_link_libraries(SimpleAIClient PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    httplib::httplib
)

target_include_directories(SimpleAIClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(WIN32)
    set(QT_DEPLOY_DIR "$<TARGET_FILE_DIR:SimpleAIClient>")

    add_custom_command(TARGET SimpleAIClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${QT_DEPLOY_DIR}/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
            "${QT_DEPLOY_DIR}/platforms/"
        COMMENT "Deploying Qt platform plugin..."
    )

    if(DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_BIN "$ENV{VCPKG_ROOT}/installed/x64-windows/bin")
        set(DEPLOY_DLLS
            "${VCPKG_BIN}/libcrypto-3-x64.dll"
            "${VCPKG_BIN}/libssl-3-x64.dll"
            "${VCPKG_BIN}/zlib1.dll"
            "${VCPKG_BIN}/brotlicommon.dll"
            "${VCPKG_BIN}/brotlidec.dll"
        )
        foreach(dll ${DEPLOY_DLLS})
            if(EXISTS ${dll})
                add_custom_command(TARGET SimpleAIClient POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${dll} "${QT_DEPLOY_DIR}/"
                    COMMENT "Deploying DLL: ${dll}"
                )
            endif()
        endforeach()
    endif()
endif()
