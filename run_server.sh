#!/usr/bin/env fish

# Kill any existing server process on port 3030
fuser -k 3030/tcp >/dev/null 2>&1; or true

# Compile the server from its source files
clang -I./src src/server.c src/models.c src/common.c src/features/*.c -o run_server -lpthread -lrt

# Run only if compilation succeeded
if test $status -eq 0
    ./run_server
end
