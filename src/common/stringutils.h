
#pragma once

#include "common/typeutils.h"

#include <string>
#include <cstdio>
#include <stdexcept>
#include <type_traits>
#include <sstream>
#include <ios>


class StringFormatter {
public:
	StringFormatter(std::string msg);
	
	std::string format();
	
	template <class T, class... Args>
	std::string format(T param, Args... args) {
		std::string s;
		
		while (true) {
			char c = read();
			
			if (!c) {
				throw std::runtime_error("Not all arguments used during string formatting");
			}
			
			if (c == '%') {
				char next = read();
				if (next == '%') {
					s += '%';
				}
				else {
					bool zero = false;
					bool left = false;
					size_t width = 0;
					
					if (next == '-') {
						left = true;
						next = read();
					}
					
					if (next == '0') {
						zero = true;
						next = read();
					}
					
					while (isdigit(next)) {
						if (width > 999) {
							throw std::runtime_error("Malformed format specifier");
						}
						width = width * 10 + next - '0';
						next = read();
					}
					
					std::string fmt = format_param(next, param);
					if (fmt.size() < width) {
						std::string padding(width - fmt.size(), zero ? '0' : ' ');
						if (left) {
							fmt += padding;
						}
						else {
							fmt = padding + fmt;
						}
					}
					return s + fmt + format(args...);
				}
			}
			else {
				s += c;
			}
		}
	}

private:
	char read();
	
	template <class T>
	std::string format_param(char type, T param) {
		switch (type) {
			case 'b': return format_bool(param);
			case 'c': return format_char(param);
			case 's': return format_string(param);
			case 'f': return format_float(param);
			case 'i': return format_number(param);
			case 'x': return format_number(param, true);
			case 'X': return format_number(param, true, true);
			case 'p': return format_ptr(param);
			default:
				throw std::runtime_error("Invalid format specifier");
		}
	}
	
	template <class T, EnableIf(Convertible(T, bool))>
	std::string format_bool(T value) {
		if (value) {
			return "true";
		}
		return "false";
	}
	
	template <class T, EnableIf(Convertible(T, char))>
	std::string format_char(T value) {
		return std::string(1, (char)value);
	}
	
	template <class T, EnableIf(Convertible(T, std::string))>
	std::string format_string(T value) {
		return value;
	}
	
	template <class T, EnableIf(Convertible(T, double))>
	std::string format_float(T value) {
		return std::to_string((double)value);
	}
	
	template <class T, EnableIf(Convertible(T, uint64_t) or IsEnum(T))>
	std::string format_number(T value, bool hex = false, bool upper = false) {
		std::ostringstream s;
		if (hex) {
			s << std::hex;
		}
		if (upper) {
			s << std::uppercase;
		}
		s << (uint64_t)value;
		return s.str();
	}
	
	template <class T, EnableIf(Convertible(T, void *))>
	std::string format_ptr(T value) {
		return "0x" + format_number((uint64_t)value, true);
	}
	
	template <class T, DisableIf(Convertible(T, bool))>
	std::string format_bool(T value) {
		throw std::runtime_error("Expected a boolean");
	}
	
	template <class T, DisableIf(Convertible(T, char))>
	std::string format_char(T value) {
		throw std::runtime_error("Expected a character");
	}
	
	template <class T, DisableIf(Convertible(T, std::string))>
	std::string format_string(T value) {
		throw std::runtime_error("Expected a string");
	}
	
	template <class T, DisableIf(Convertible(T, double))>
	std::string format_float(T value) {
		throw std::runtime_error("Expected a float");
	}
	
	template <class T, DisableIf(Convertible(T, uint64_t) or IsEnum(T))>
	std::string format_number(T value, bool hex = false, bool upper = false) {
		throw std::runtime_error("Expected an integer");
	}
	
	template <class T, DisableIf(Convertible(T, void *))>
	std::string format_ptr(T value) {
		throw std::runtime_error("Expected a pointer");
	}
	
	std::string msg;
	size_t pos;
};


class StringUtils {
public:
	static bool parseint(std::string str, int *value);
	
	static bool is_printable(char c);
	static bool is_printable(std::string str);

	template <class... Args>
	static std::string format(std::string msg, Args... args) {
		StringFormatter formatter(msg);
		return formatter.format(args...);
	}
};
