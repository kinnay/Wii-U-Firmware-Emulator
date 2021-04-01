# Wii U Firmware Emulator
This emulator emulates the Wii U processors and hardware at the lowest level. It's currently able to emulate all the way through boot1, IOSU and Cafe OS until men.rpx crashes somewhere in AXInit.

This emulator used to be written in both Python and C++. You can still find the source code of this emulator in the branch 'old'.

## Instructions
1. Make sure you have a linux system, a g++ compiler that supports c++14 and the OpenSSL library.
2. Dump the following files from your Wii U (with hexFW for example) and put them into the 'files' folder: `boot1.bin`, `otp.bin`, `seeprom.bin`, `mlc.bin`, `slc.bin` and `slccmpt.bin`. This emulator may write to some of these files. Use a backup if you want to keep your original dumps.
3. Create `files/espresso_key.bin` and put the espresso ancast key into it.
4. Run `make` to compile the emulator

## Configuration
`src/config.h` contains a few macros that enable/disable certain features of the emulator, such as breakpoints. Enabling a feature adds interesting commands to the debugger but may slow down the emulator a bit.

| Option | Description |
| --- | --- |
| `STATS` | Tracks some statistics about the emulator, such as the number of instructions that are executed or the number of system calls. |
| `METRICS` | Tracks even more statistics: counts how often each individual PowerPC instruction is executed and how often each system call is taken. |
| `WATCHPOINTS` | Enables memory watchpoints in the debugger. |
| `BREAKPOINTS` | Enables instruction breakpoints in the debugger. |
| `SYSLOG` | Writes the system log into `logs/syslog.txt`. |
| `DSPDMA` | Logs DSP DMA transfers to `logs/dspdma.txt`. |

Additionally, you can adjust the log level in `src/main.cpp`. To disable warnings about unimplemented hardware features set the log level to `ERROR` or `NONE`.

## Debugger
Using this emulator you can actually see what boot1, IOSU and Cafe OS look like at runtime, and even perform debugging operations on them. You can stop execution and show the debugger by pressing Ctrl+C at any point.

| Command | Description |
| --- | --- |
| `help` | Print list of commands. |
| `exit` | Stop the emulator. |
| `quit` | Same as `exit`. |
| `select arm/ppc0/ppc1/ppc2/dsp` | Select a processor to debug. |
| `step (<steps>)` | Execute a fixed number of instructions on the current processor. |
| `run` | Continue emulation normally. |
| `reset` | Reset the emulator to its initial state. |
| `restart` | Restart emulation from the beginning. This is the same as executing `reset` and then `run`. |
| `stats` | Print some interesting statistics, such as the number of instructions that have been executed so far. Only valid if `STATS` is enabled. |
| `metrics ppc0/ppc1/ppc2 category/frequency` | Print how often every PowerPC instruction has been executed on the given core, either sorted by category or sorted by frequency. Only valid if `METRICS` is enabled. |
| `syscalls ppc0/ppc1/ppc2` | Print how often each system call has been executed on the given core, sorted by frequency. Only valid if `METRICS` is enabled. |
| `break add/del <address>` | Add or remove a breakpoint. Only valid if `BREAKPOINTS` is enabled. |
| `break list` | Print breakpoint list. Only valid if `BREAKPOINTS` is enabled. |
| `break clear` | Remove all breakpoints. Only valid if `BREAKPOINTS` is enabled. |
| `watch add/del virt/phys read/write <address>` | Add/remove a memory read/write watchpoint at the given virtual or physical address. Only valid if `WATCHPOINTS` is enabled. |
| `watch list` | Print watchpoint list. Only valid if `WATCHPOINTS` is enabled. |
| `watch clear` | Remove all watchpoints. Only valid if `WATCHPOINTS` is enabled. |
| `state (full)` | Print all general purpose registers on the current processor and some other important registers. If `full` is passed, many other special registers are printed as well. |
| `print <expr>` | Evaluate the given expression and print the result. |
| `trace` | Print stack trace on the current processor. |
| `read virt/phys <address> <length>` | Read `length` bytes at the given virtual or physical `address` and print them both in hex and ascii characters. If a virtual address is given, the MMU of the current processor is used to translate the address. |
| `read code/data <address> <length>` | Read `length` bytes at the given DSP memory address and print them in hex. |
| `translate <address>` | Translate the given virtual address and print the physical address, using the MMU of the current processor. |
| `memmap` | Print the virtual memory map of the current processor. |
| `modules` | Print the list of loaded RPL files and the starting address of their .text segment. |
| `module <name>` | Print more information about a specific module. |
| `threads` | Print thread list for IOSU or COS (depending on the current processor). |
| `thread <id>` | Print more information about a specific thread for IOSU or COS (depending on the current processor). If a thread is waiting, this command tries to figure out what it is waiting for. If a PPC processor is being debugged, this command also tries to print a stack trace for the given thread. |
| `queues` | Print list of message queues in IOSU. |
| `queue <id>` | Print more information about a specific message queue in IOSU. |
| `devices` | Print the state of all ipc devices in IOSU. |
| `hardware` | Print the content of a few hardware registers (`PI`/`GPU`/`LATTE`). |
| `ipc` | Lists pending IPC requests from the PPC cores. |
| `volumes` | Print list of filesystem volumes in IOSU. |
| `fileclients` | Print list of filesystem clients. |
| `slccache` | Print information about SLC cache in IOSU. |
