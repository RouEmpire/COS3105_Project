# Kanban Board TUI

This C terminal interface uses the single-header [RogueUtil](../../lib/rogueutil-1.0/README.md) library for terminal input, cursor movement, color, and screen control. It is an alternative to the prompt-based Fish interface in `frontend/app.fish`.

## Build and run

Start the server from the project root in another terminal:

```fish
./run_server.sh
```

Build and launch the TUI:

```bash
make
./kanban-tui
```

Or use the Makefile shortcut:

```bash
make run
```

Clean the generated executable with `make clean`. The TUI connects directly to `127.0.0.1:3030`; it does not launch the server itself.

## Controls

- `W` / `S`: move the selection up or down.
- `A` / `D`: move the selection left or right between columns.
- `Enter`: select the highlighted action.
- `Q` or `Esc`: quit or cancel text entry.
- `Backspace`: edit text while entering a value.

The menu includes board and task lookup, create/update operations, soft deletes, task status toggling, and bulk data retrieval. Each server action displays the raw server response and extracts the numeric protocol status from its first response line. Mutating menu actions really update server data.

Titles containing commas are rejected because commas separate fields in the existing wire protocol. Use a terminal with ANSI escape support for screen positioning and colors.
