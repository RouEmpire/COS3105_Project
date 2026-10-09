#!/usr/bin/env fish

# Strip .c extension if it was passed (e.g. test_models.c -> test_models)
set TEST_NAME (string replace -r '\.c$' '' -- $argv[1])

# Compile all necessary source files
clang -I./src "src/tests/$TEST_NAME.c" src/models.c src/common.c src/features/*.c -o "$TEST_NAME"

# Run only if compilation succeeded
if test $status -eq 0
    ./$TEST_NAME
end
