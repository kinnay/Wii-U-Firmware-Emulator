
#include "history.h"

#include <readline/history.h>
#include <sys/stat.h>
#include <cstdlib>


namespace History {
	std::string filename;

	void init() {
		using_history();
		
		char *home = getenv("HOME");
		char *xdg_data = getenv("XDG_DATA_HOME");
		
		std::string directory;
		if (xdg_data && xdg_data[0] == '/')
			directory = std::string(xdg_data) + "/wiiu-firmware-emulator";
		else
			directory = std::string(home) + "/.local/share/wiiu-firmware-emulator";
	
		mkdir(directory.c_str(), 0755);
		filename = directory + "/history";
		read_history(filename.c_str());
	}

	void append(char *line) {
		add_history(line);
		write_history(filename.c_str());
	}
}
