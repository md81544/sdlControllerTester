**THIS IS A TEMPLATE REPOSITORY**

To use it, use the github "Use this template" button to create a new repository which will include these files.

## Notes
* this template requires Catch2 testing library to be installed as a system lib.
* Sample tests are included. Remember testable code required in tests should be available in
  the static library (i.e. in lib/).

## What to change
The only changes you should have to make are:
* set the binary's name in BOTH CMakeLists.txt and Makefile (the position placeholder is `<<SET BINARY NAME HERE>>`).
* add the binary's name to .gitignore (the binary is automatically symlinked into the top-level directory on build)
