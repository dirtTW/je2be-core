# Applies PATCH_FILE to SOURCE_DIR, and treats an already-applied patch as success.
#
# FetchContent/ExternalProject re-runs the patch step whenever the update step
# runs, so a plain `patch` or `git apply` fails the second time round with
# "Reversed (or previously applied) patch detected". This script instead checks
# whether the change is present and reports success either way, which keeps
# repeated configures working.
#
# Invoked from CMakeLists.txt via PATCH_COMMAND with -DPATCH_FILE= and
# -DSOURCE_DIR=.

if(NOT DEFINED PATCH_FILE OR NOT EXISTS "${PATCH_FILE}")
  message(FATAL_ERROR "apply-patch.cmake: PATCH_FILE '${PATCH_FILE}' does not exist")
endif()
if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
  message(FATAL_ERROR "apply-patch.cmake: SOURCE_DIR '${SOURCE_DIR}' is not a directory")
endif()

find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
  message(FATAL_ERROR "apply-patch.cmake: git is required to apply ${PATCH_FILE}")
endif()

set(apply_args apply --ignore-whitespace --whitespace=nowarn)

execute_process(
  COMMAND "${GIT_EXECUTABLE}" ${apply_args} --check "${PATCH_FILE}"
  WORKING_DIRECTORY "${SOURCE_DIR}"
  RESULT_VARIABLE can_apply
  OUTPUT_QUIET
  ERROR_QUIET)

if(can_apply EQUAL 0)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" ${apply_args} "${PATCH_FILE}"
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE applied
    OUTPUT_QUIET)
  if(NOT applied EQUAL 0)
    message(FATAL_ERROR "apply-patch.cmake: failed to apply ${PATCH_FILE} to ${SOURCE_DIR}")
  endif()
  message(STATUS "apply-patch.cmake: applied ${PATCH_FILE}")
  return()
endif()

# Not appliable: either it is already in place, or the dependency moved under us.
execute_process(
  COMMAND "${GIT_EXECUTABLE}" ${apply_args} --reverse --check "${PATCH_FILE}"
  WORKING_DIRECTORY "${SOURCE_DIR}"
  RESULT_VARIABLE already_applied
  OUTPUT_QUIET
  ERROR_QUIET)

if(already_applied EQUAL 0)
  message(STATUS "apply-patch.cmake: ${PATCH_FILE} is already applied")
  return()
endif()

message(FATAL_ERROR
  "apply-patch.cmake: ${PATCH_FILE} does not apply to ${SOURCE_DIR}. "
  "The dependency probably changed; regenerate the patch.")
