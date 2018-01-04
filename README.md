# Wii U Firmware Emulator
## What does this do?
This takes a fw.img and a NAND dump (otp, seeprom, slc and mlc) and tries to emulate the Wii U processors and hardware. It's currently able to emulate all the way through IOSU until it boots the PowerPC kernel, and the PowerPC processor actually loads the system libraries and root.rpx until it gets stuck somewhere in tcl.rpl.

This emulator consists of both Python and C++ code. The C++ code (which is compiled into a python module) contains the ARM and PowerPC interpreters and a few other classes, like a SHA1 calculator. This is kept as generic as possible and can in theory be used in any kind of emulator. The Python code is responsible for the things that don't require high performance. It emulates the hardware devices and provides a debugger for example.

## How to use
Requirements:
* A fw.img file, this contains the first code that's run by the emulator
* A NAND dump (otp, seeprom, slc and mlc)
* Python 3 (tested with 3.6.3)
* PyCrypto / PyCryptodome (for AES hardware)

Before running this, you need to update the path to fw.img in main.py and the paths to your NAND dump files in hardware.py. Also create a file named espresso_key.txt and put the espresso ancast key into it as ascii hex digits.

Pass "noprint" as a command line argument to disable print messages on unimplemented hardware reads/writes. Pass "logsys" to enable IOSU syscall logging. This generates ipc.txt (ipc requests like ioctls), messages.txt (message queue operations) and files.txt (files openend by IOSU). It slows down the code a lot however.

## Debugger
Using this emulator you can actually see what IOSU and the PowerPC kernel/loader look like at runtime (at least, the parts that this emulator is able to emulate accurately), and even perform some debugging operations on them. There's still some room for improvement on this end, but here's a list of commands:

| Command | Description |
| --- | --- |
| select &lt;index&gt; | Select a processor to debug: 0=ARM, 1-3=PPC cores |
| break &lt;add/del&gt; &lt;address&gt; | Add/remove breakpoint |
| watch &lt;add/del&gt; &lt;read/write&gt; &lt;address&gt; | Add/remove memory read/write watchpoint |
| phys read &lt;address&gt; &lt;length&gt; | Read and print physical memory |
| virt read &lt;address&gt; &lt;length&gt; | Read and print memory at translated address |
| translate &lt;address&gt; (&lt;type&gt;) | Translate address with optional type: 0=code, 1=data read (default), 2=data write |
| getreg &lt;reg&gt; | Print general purpose register |
| setreg &lt;reg&gt; &lt;value&gt; | Change general purpose register |
| set thumb &lt;0/1&gt; | Enable/disable thumb mode |
| state | Print all general purpose registers |
| step | Step through code |
| run | Continue emulation |
| eval &lt;...&gt; | Call python's eval, given the processor registers as variables, and print the result |

To enable the debugger, pass "break" as command line argument.

## Wiki
https://github.com/Kinnay/Wii-U-Firmware-Emulator/wiki
