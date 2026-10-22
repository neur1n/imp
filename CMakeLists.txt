CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

PROJECT(imp VERSION 0.0.1 LANGUAGES CXX)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

ADD_LIBRARY(imp_core
  src/backend.cpp
)

TARGET_INCLUDE_DIRECTORIES(imp_core
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(APPLE)
  ENABLE_LANGUAGE(OBJCXX)
  TARGET_SOURCES(imp_core PRIVATE src/platform/macos_backend.mm)
  TARGET_LINK_LIBRARIES(imp_core PRIVATE "-framework Carbon" "-framework CoreServices")
elseif(WIN32)
  TARGET_SOURCES(imp_core PRIVATE src/platform/windows_backend.cpp)
elseif(UNIX)
  TARGET_SOURCES(imp_core PRIVATE src/platform/linux_backend.cpp)
else()
  TARGET_SOURCES(imp_core PRIVATE src/platform/unavailable_backend.cpp)
endif()

ADD_EXECUTABLE(imp src/main.cpp)
TARGET_COMPILE_DEFINITIONS(imp PRIVATE IMP_VERSION="${PROJECT_VERSION}")
TARGET_LINK_LIBRARIES(imp PRIVATE imp_core)

INSTALL(TARGETS imp RUNTIME DESTINATION bin)

INCLUDE(CTest)
IF(BUILD_TESTING)
  ADD_EXECUTABLE(imp_core_tests test/core_test.cpp)
  TARGET_LINK_LIBRARIES(imp_core_tests PRIVATE imp_core)
  IF(APPLE)
    TARGET_INCLUDE_DIRECTORIES(imp_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    TARGET_LINK_LIBRARIES(imp_core_tests PRIVATE "-framework CoreServices")
  ENDIF()
  ADD_TEST(NAME imp_core_tests COMMAND imp_core_tests)

  # Exercise the production CLI with a deterministic backend, without changing input sources.
  ADD_LIBRARY(imp_cli_entry OBJECT src/main.cpp)
  TARGET_COMPILE_DEFINITIONS(imp_cli_entry PRIVATE
    main=imp_test_main IMP_VERSION="${PROJECT_VERSION}")
  ADD_EXECUTABLE(imp_cli_tests
    test/cli_test.cpp
    src/backend.cpp
    $<TARGET_OBJECTS:imp_cli_entry>
  )
  FOREACH(target imp_cli_entry imp_cli_tests)
    TARGET_INCLUDE_DIRECTORIES(${target} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/inc
      ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    )
  ENDFOREACH()
  ADD_TEST(NAME imp_cli_tests COMMAND imp_cli_tests)

  ADD_TEST(NAME imp_cli_help COMMAND imp --help)
  SET_TESTS_PROPERTIES(imp_cli_help PROPERTIES
    PASS_REGULAR_EXPRESSION "Usage: .*imp <command>")
ENDIF()
