cmake_minimum_required(VERSION 3.20)

project(ksystemstats-intel-npu VERSION 0.1.0 LANGUAGES CXX)

set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Test)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n)
find_package(KSysGuard REQUIRED)

add_library(intelnpubackend STATIC
    src/intelnpubackend.cpp
    src/intelnpubackend.h
)
target_link_libraries(intelnpubackend
    PUBLIC
        Qt6::Core
)
target_include_directories(intelnpubackend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(intelnpubackend PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(ksystemstats_plugin_intelnpu MODULE
    src/intelnpuplugin.cpp
    src/intelnpuplugin.h
    src/intelnpuplugin.json
)
target_link_libraries(ksystemstats_plugin_intelnpu
    PRIVATE
        intelnpubackend
        Qt6::Core
        KF6::CoreAddons
        KF6::I18n
        KSysGuard::SystemStats
)

install(TARGETS ksystemstats_plugin_intelnpu DESTINATION ${KDE_INSTALL_PLUGINDIR}/ksystemstats)

if(BUILD_TESTING)
    ecm_add_test(
        autotests/intelnpubackendtest.cpp
        TEST_NAME intelnpubackendtest
        LINK_LIBRARIES intelnpubackend Qt6::Test Qt6::Core
    )
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
