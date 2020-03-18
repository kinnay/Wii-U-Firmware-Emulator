
#include "common/stringutils.h"
#include "common/textinput.h"
#include "common/sys.h"

int TextInput::askint(const char *msg, int min, int max) {
	int value;
	while (true) {
		Sys::stdout->write(msg);
	
		std::string text = Sys::stdin->readline();
		if (StringUtils::parseint(text, &value)) {
			if (value >= min && value <= max) {
				break;
			}
		}
		
		Sys::stdout->write("Please enter a valid number\n");
	}
	return value;
}
