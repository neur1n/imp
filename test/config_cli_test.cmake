# Exercise production configuration discovery without selecting any input sources.
FILE(MAKE_DIRECTORY "${DIRECTORY}/bin" "${DIRECTORY}/cwd")
SET(binary "${DIRECTORY}/bin/imp")
CONFIGURE_FILE("${CLI}" "${binary}" COPYONLY)
FILE(REMOVE "${DIRECTORY}/bin/imp.conf")

FUNCTION(expect_error pattern)
  EXECUTE_PROCESS(COMMAND "${binary}" ${ARGN}
    WORKING_DIRECTORY "${DIRECTORY}/cwd"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  IF(NOT result EQUAL 1 OR NOT output STREQUAL "" OR NOT error MATCHES "${pattern}")
    MESSAGE(FATAL_ERROR "Unexpected result ${result}, stdout=${output}, stderr=${error}")
  ENDIF()
ENDFUNCTION()

# A file in the current working directory must not affect executable-relative discovery.
FILE(WRITE "${DIRECTORY}/cwd/imp.conf" "malformed\n")
expect_error("Unknown macOS input-source pair: absent" --pair absent status)

# Use the observed ID as a fixture, so the default-pair mapping is testable without switching.
EXECUTE_PROCESS(COMMAND "${binary}" status RESULT_VARIABLE baseline_result OUTPUT_VARIABLE baseline)
IF(baseline_result EQUAL 0)
  STRING(REGEX MATCH "source_id=([^\n]+)" matched "${baseline}")
  SET(observed_id "${CMAKE_MATCH_1}")
  FILE(WRITE "${DIRECTORY}/bin/imp.conf"
    "first missing.alpha ${observed_id}\nsecond missing.alpha missing.native\n")
  EXECUTE_PROCESS(COMMAND "${binary}" status RESULT_VARIABLE result OUTPUT_VARIABLE output)
  IF(NOT result EQUAL 0 OR NOT output MATCHES "mode=native")
    MESSAGE(FATAL_ERROR "The first pair must be the default: ${output}")
  ENDIF()
  EXECUTE_PROCESS(COMMAND "${binary}" --pair second status RESULT_VARIABLE result OUTPUT_VARIABLE output)
  IF(NOT result EQUAL 0 OR NOT output MATCHES "mode=unknown")
    MESSAGE(FATAL_ERROR "Explicit pair selection must override the default: ${output}")
  ENDIF()
  FILE(WRITE "${DIRECTORY}/bin/imp.conf"
    "first missing.alpha missing.native\nsecond missing.alpha ${observed_id}\n")
  EXECUTE_PROCESS(COMMAND "${binary}" status RESULT_VARIABLE result OUTPUT_VARIABLE output)
  IF(NOT result EQUAL 0 OR NOT output MATCHES "mode=unknown")
    MESSAGE(FATAL_ERROR "An unavailable first pair must not be silently skipped: ${output}")
  ENDIF()
ENDIF()

FILE(WRITE "${DIRECTORY}/bin/imp.conf" "custom missing.alpha missing.native\nother other.alpha other.native\n")
expect_error("Unknown macOS input-source pair: wetype" --pair wetype status)
EXECUTE_PROCESS(COMMAND "${binary}" status RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
IF(result EQUAL 0 AND NOT output MATCHES "mode=unknown")
  MESSAGE(FATAL_ERROR "The first configured pair must determine status interpretation: ${output}")
ENDIF()
IF(NOT result EQUAL 0 AND NOT error MATCHES "macOS did not return")
  MESSAGE(FATAL_ERROR "Unexpected status error: ${error}")
ENDIF()

# Explicit files override automatic discovery and must not merge built-in/adjacent pairs.
FILE(WRITE "${DIRECTORY}/override with spaces.conf" "override missing.alpha missing.native\n")
expect_error("Unknown macOS input-source pair: custom"
  --config "${DIRECTORY}/override with spaces.conf" --pair custom status)
expect_error("could not open configuration" --config "${DIRECTORY}/missing.conf" status)
expect_error("configuration path is not a regular file" --config "${DIRECTORY}/cwd" status)

FILE(REMOVE "${DIRECTORY}/cwd/imp-link")
FILE(CREATE_LINK "${binary}" "${DIRECTORY}/cwd/imp-link" SYMBOLIC)
EXECUTE_PROCESS(COMMAND "${DIRECTORY}/cwd/imp-link" --pair wetype status
  WORKING_DIRECTORY "${DIRECTORY}/cwd"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
IF(NOT result EQUAL 1 OR NOT output STREQUAL "" OR NOT error MATCHES "Unknown macOS input-source pair: wetype")
  MESSAGE(FATAL_ERROR "Symlink invocation must use configuration beside the actual executable: ${error}")
ENDIF()

# Invalid or empty configuration must fail even before a set operation.
FILE(WRITE "${DIRECTORY}/bin/imp.conf" "# header\nshort one\n")
expect_error("imp.conf:2: expected three columns" set native)
FILE(WRITE "${DIRECTORY}/bin/imp.conf" "# no pairs\n")
expect_error("configuration must contain at least one pair" status)
FILE(WRITE "${DIRECTORY}/bin/imp.conf" "same a b\nsame c d\n")
expect_error("imp.conf:2: duplicate pair name: same" status)

# Help is available even when configuration is invalid.
EXECUTE_PROCESS(COMMAND "${binary}" --help RESULT_VARIABLE result OUTPUT_VARIABLE output)
IF(NOT result EQUAL 0 OR NOT output MATCHES "--config")
  MESSAGE(FATAL_ERROR "Help must bypass configuration loading")
ENDIF()
