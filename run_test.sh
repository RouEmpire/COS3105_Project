#!/usr/bin/env bash

# Strip .c extension if it was passed (e.g. test_models.c -> test_models)
TEST_NAME="${1%.c}"

# Compile all necessary source files
clang -I./src "src/tests/${TEST_NAME}.c" src/models.c src/common.c -o "$TEST_NAME"

# Run only if compilation succeeded
if [ $? -eq 0 ]; then
    "./$TEST_NAME"
fi
