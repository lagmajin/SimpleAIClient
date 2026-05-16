cmake_minimum_required(VERSION 3.16)
project(SimpleAIClient VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

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
