CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

PROJECT(imp VERSION 0.0.1 LANGUAGES CXX)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

ADD_LIBRARY(imp_core
  src/backend.cpp
  src/config.cpp
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
# Install the preset only once; subsequent installs must preserve user edits.
INSTALL(CODE "
  IF(NOT EXISTS \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/imp.conf\")
    FILE(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\"
      TYPE FILE FILES \"${CMAKE_CURRENT_SOURCE_DIR}/bin/imp.conf\")
  ENDIF()
")

INCLUDE(CTest)
IF(BUILD_TESTING)
  ADD_EXECUTABLE(imp_config_tests test/config_test.cpp src/config.cpp)
  TARGET_INCLUDE_DIRECTORIES(imp_config_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
  ADD_TEST(NAME imp_config_tests COMMAND imp_config_tests ${CMAKE_CURRENT_BINARY_DIR}/config-test-data)
  ADD_TEST(NAME imp_install_tests COMMAND ${CMAKE_COMMAND}
    "-DBUILD=${CMAKE_CURRENT_BINARY_DIR}" "-DDIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/install-test-data"
    "-DPRESET=${CMAKE_CURRENT_SOURCE_DIR}/bin/imp.conf" "-DCONFIG=$<CONFIG>"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/test/install_test.cmake)

  ADD_EXECUTABLE(imp_core_tests test/core_test.cpp)
  TARGET_LINK_LIBRARIES(imp_core_tests PRIVATE imp_core)
  IF(APPLE)
    TARGET_INCLUDE_DIRECTORIES(imp_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    TARGET_LINK_LIBRARIES(imp_core_tests PRIVATE "-framework CoreServices")
  ENDIF()
  ADD_TEST(NAME imp_core_tests COMMAND imp_core_tests)
  IF(APPLE)
    ADD_TEST(NAME imp_config_cli_tests COMMAND ${CMAKE_COMMAND}
      "-DCLI=$<TARGET_FILE:imp>" "-DDIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/config-cli-test-data"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/test/config_cli_test.cmake)
  ENDIF()

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
    PASS_REGULAR_EXPRESSION "Usage: .*imp .*<command>")

  FIND_PROGRAM(IMP_VIM_EXECUTABLE NAMES vim)
  FIND_PROGRAM(IMP_NVIM_EXECUTABLE NAMES nvim)
  IF(IMP_VIM_EXECUTABLE OR IMP_NVIM_EXECUTABLE)
    ADD_EXECUTABLE(imp_editor_backend test/editor_backend.cpp)
  ENDIF()
  IF(IMP_VIM_EXECUTABLE)
    ADD_TEST(NAME imp_vim_tests COMMAND ${CMAKE_COMMAND} -E env
      "IMP_TEST_BACKEND=$<TARGET_FILE:imp_editor_backend>"
      ${IMP_VIM_EXECUTABLE} -Nu NONE -i NONE -n -es
      -S ${CMAKE_CURRENT_SOURCE_DIR}/test/editor_test.vim)
    SET_TESTS_PROPERTIES(imp_vim_tests PROPERTIES TIMEOUT 30)
  ENDIF()
  IF(IMP_NVIM_EXECUTABLE)
    ADD_TEST(NAME imp_nvim_tests COMMAND ${CMAKE_COMMAND} -E env
      "IMP_TEST_BACKEND=$<TARGET_FILE:imp_editor_backend>"
      ${IMP_NVIM_EXECUTABLE} --headless -u NONE -i NONE -n
      -S ${CMAKE_CURRENT_SOURCE_DIR}/test/editor_test.vim)
    SET_TESTS_PROPERTIES(imp_nvim_tests PROPERTIES TIMEOUT 30)
  ENDIF()
ENDIF()
