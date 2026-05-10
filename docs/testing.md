# Testing

This repository is exempt from the fleet testing standards
(Repository_Management EPIC #1140) because it is primarily a C++
project — Python test infrastructure does not apply. Native testing
is handled by ctest (CMake `enable_testing()` with a custom
`qe_add_test` helper and the in-tree `tests/test_framework.h`).

The Fleet Testing Standards apply to Python repos only:
https://github.com/D-sorganization/Repository_Management/blob/main/docs/FLEET_TESTING_STANDARDS.md.
