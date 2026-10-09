#!/usr/bin/env fish

# Compile the server from its source files
set myVar clang -I./src src/server.c src/models.c src/common.c src/features/*.c -o run_server -lpthread -lrt


# Run only if compilation succeeded
if test $status -eq 0
    ./run_server
end
