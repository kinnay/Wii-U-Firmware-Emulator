
#include "common/codegenerator.h"

CodeGenerator::CodeGenerator() {
	tabs = 0;
}

void CodeGenerator::clear() {
	tabs = 0;
	stream.clear();
}

void CodeGenerator::write(std::string str) {
	stream.write(str);
}

std::string CodeGenerator::get() {
	return stream.get();
}

void CodeGenerator::indent() {
	tabs++;
}

void CodeGenerator::unindent() {
	tabs--;
}

void CodeGenerator::beginline() {
	for (int i = 0; i < tabs; i++) {
		write("\t");
	}
}

void CodeGenerator::endline() {
	write("\n");
}
