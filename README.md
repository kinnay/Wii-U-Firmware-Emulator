# Wii U Firmware Emulator
## What does this do?
This takes a fw.img and a NAND dump and tries to emulate the Wii U processors and hardware. It's currently able to emulate all the way through IOSU and the PowerPC kernel until it gets stuck when nn_acp.rpl tries to open /dev/ndm.

This emulator consists of both Python and C++ code. The C++ code (which is compiled into a python module) contains the ARM and PowerPC interpreters and a few other classes, like a SHA1 calculator. The Python code is responsible for the things that don't require high performance. It emulates the hardware devices and provides a debugger for example.

## How to use
Requirements:
* A fw.img file, this contains the first code that's run by the emulator
* A NAND dump (otp, seeprom, slc and mlc)
* Python 3 (tested with 3.6.4)
* PyCrypto / PyCryptodome (for AES hardware)

Before running this, you need to update the path to fw.img in main.py and the paths to your NAND dump files in hardware.py. This emulator writes to these files if they're accessed by IOSU, so it may be a good idea to use a backup. Also create a file named espresso_key.txt and put the espresso ancast key into it as ascii hex digits.

Pass "noprint" as a command line argument to disable print messages on unimplemented hardware reads/writes. Pass "logall" to enable hack that sets the COS log level to the highest possible value. Pass "logsys" to enable IOSU syscall logging. This generates ipc.txt (ipc requests like ioctls), messages.txt (message queue operations) and files.txt (files openend by IOSU). It slows down the code a lot however.

## Debugger
Using this emulator you can actually see what IOSU and the PowerPC kernel/loader look like at runtime (at least, the parts that this emulator is able to emulate accurately), and even perform some debugging operations on them. To enable the debugger, pass "break" as command line argument. You can stop execution and show the debugger by pressing Ctrl+C at any point, but sometimes this causes an instruction to be executed only partially, which might put the emulator into a broken state. Here's a list of commands:

### General
| Command | Description |
| --- | --- |
| help (&lt;command&gt;) | Print list of commands or information about a command |
| select &lt;index&gt; | Select a processor to debug: 0=ARM, 1-3=PPC cores |
| break add/del &lt;address&gt; | Add/remove breakpoint |
| watch add/del read/write &lt;address&gt; | Add/remove memory read/write watchpoint |
| read phys/virt &lt;address&gt; &lt;length&gt; | Read memory and print to console |
| dump phys/virt &lt;address&gt; &lt;length&gt; &lt;filename&gt; | Dump memory to file |
| translate &lt;address&gt; (&lt;type&gt;) | Translate address with optional type: 0=code, 1=data read (default), 2=data write |
| getreg &lt;reg&gt; | Print general purpose register |
| setreg &lt;reg&gt; &lt;value&gt; | Change general purpose register |
| step | Step through code |
| run | Continue emulation |
| eval &lt;expr&gt; | Call python's eval, given the processor registers as variables, and print the result |

### ARM only
| Command | Description |
| --- | --- |
| state | Print all general purpose registers |

### PPC only
| Command | Description |
| --- | --- |
| state (all) | Print all GPRs and the most important other registers. When 'all' is passed, the segment registers are printed as well. |
| getspr &lt;spr&gt; | Print a special purpose register |
| setspr &lt;spr&gt; &lt;value&gt; | Change a special purpose register |
| setpc &lt;value&gt; | Change the program counter |
| modules | Print list of loaded RPL files |
| module &lt;name&gt; | Print information about a specific module |
| threads | Print thread list |
| thread &lt;addr&gt; | Print information about a specific thread |
| find &lt;addr&gt; | Find and print the module that contains `addr` |
| trace | Print stack trace |
| memmap | Print the virtual memory map |
