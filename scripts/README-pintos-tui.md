# Pintos TUI

Source: https://github.com/Shirokuma0704/pintos-tui
Installed revision: `a6b1f13adbc0b8f10ee34c187e011b072d522c20`

Run inside the dev container from the repository root:

```sh
./pintos-tui
```

This container also exposes `pintos-tui` on PATH.
The project-local launcher works after a container rebuild without that PATH link.
Python 3.10+ and the standard-library curses module are required.

```sh
./pintos-tui --list
./pintos-tui --project threads
```

The default project is `userprog` (Project 2).
Enter/r runs the selected test; a runs the official make-check tests;
d opens GDB; s stops a run; q exits; ? opens help.
Optional extra tests are listed separately and excluded from a.
Runtime logs are written to `.tui/`, excluded locally using `.git/info/exclude`.
Opening the TUI does not run tests automatically.
