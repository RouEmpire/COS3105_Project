#!/usr/bin/env fish

# Compile the server from its source files
clang -I./src src/server.c src/models.c src/common.c -o run_server -lpthread -lrt

# Run only if compilation succeeded
if test $status -eq 0
    ./run_server
end
