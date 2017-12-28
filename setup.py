
from distutils.core import setup, Extension
import shutil
import sys
import os

with open("src/module_main.cpp", "rb") as f: data = f.read()
with open("src/module_main.cpp", "wb") as f: f.write(data)

sources = []
for file in os.listdir("src"):
	if file.endswith(".cpp"):
		sources.append(os.path.join("src", file))
		
macros = ["_CRT_SECURE_NO_DEPRECATE"]
if not "MINIMAL" in sys.argv:
	macros += ["BREAKPOINTS", "WATCHPOINTS"]
else:
	sys.argv.remove("MINIMAL")

macros = [(macro, 1) for macro in macros]

module = Extension(
	name = "pyemu",
	sources = sources,
	define_macros = macros,
	extra_compile_args = ["/WX"]
)

setup(
	name = "PyEmu",
	version = "1.0.0",
	author = "Yannik Marchand",
	author_email = "ymarchand@me.com",
	
	ext_modules = [module],
)

shutil.copy("build/lib.win32-3.6/pyemu.cp36-win32.pyd", ".")
