
#include "common/stringutils.h"
#include "common/textinput.h"
#include "common/sys.h"

int TextInput::askint(const char *msg, int min, int max) {
	int value;
	while (true) {
		Sys::out->write(msg);
	
		std::string text = Sys::in->readline();
		if (StringUtils::parseint(text, &value)) {
			if (value >= min && value <= max) {
				break;
			}
		}
		
		Sys::out->write("Please enter a valid number\n");
	}
	return value;
}
