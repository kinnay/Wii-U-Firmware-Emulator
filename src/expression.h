
#pragma once

#include "common/refcountedobj.h"
#include "common/variant.h"
#include <vector>
#include <string>
#include <map>


class Token : public RefCountedObj {
public:
	enum Type {
		NAME,
		SYMBOL,
		NUMBER
	};

	Token(Type type);
	Token(Type type, std::string value);
	Token(Type type, uint32_t value);
	
	Type type;
	std::string name;
	uint32_t value;
};


class TokenList : public RefCountedObj {
public:
	TokenList();
	
	void add(Token::Type type);
	void add(Token::Type type, std::string value);
	void add(Token::Type type, uint32_t value);
	
	Ref<Token> read();
	Ref<Token> peek();
	bool eof();

private:
	std::vector<Ref<Token>> tokens;
	size_t offset;
};


class Tokenizer {
public:
	bool parse(std::string text, Ref<TokenList> tokens);

private:
	enum State {
		NEXT,
		NUMBER_BASE,
		NUMBER_PREFIX,
		NUMBER,
		NAME
	};
	
	bool handle(char c);
	void error(char c);
	
	bool state_next(char c);
	bool state_number_base(char c);
	bool state_number_prefix(char c);
	bool state_number(char c);
	bool state_name(char c);
	
	State state;
	std::string name;
	uint32_t number;
	int base;
	
	Ref<TokenList> tokens;
};


class EvalContext : public RefCountedObj {
public:
	void add(std::string name, uint32_t value);
	
	bool get(std::string name, uint32_t *value);
	
private:
	std::map<std::string, uint32_t> vars;
};


class Node : public RefCountedObj {
public:
	virtual bool evaluate(Ref<EvalContext> context, uint32_t *value) = 0;
};


class Node_Unary : public Node {
public:
	enum Type {
		NEGATE
	};
	
	Node_Unary(Type type, Ref<Node> child);
	
	bool evaluate(Ref<EvalContext> context, uint32_t *value);
	
private:
	Type type;
	Ref<Node> child;
};


class Node_Binary : public Node {
public:
	enum Type {
		ADD, SUB, MUL, DIV
	};
	
	Node_Binary(Type type, Ref<Node> left, Ref<Node> right);
	
	bool evaluate(Ref<EvalContext> context, uint32_t *value);
	
private:
	Type type;
	Ref<Node> left;
	Ref<Node> right;
};


class Node_Const : public Node {
public:
	Node_Const(uint32_t value);
	
	bool evaluate(Ref<EvalContext> context, uint32_t *value);
	
private:
	uint32_t value;
};


class Node_Var : public Node {
public:
	Node_Var(std::string name);
	
	bool evaluate(Ref<EvalContext> context, uint32_t *value);
	
private:
	std::string name;
};


class Parser {
public:
	Ref<Node> parse(Ref<TokenList> tokens);

private:
	Ref<Node> parse_expr(Ref<TokenList> tokens);
	Ref<Node> parse_term(Ref<TokenList> tokens);
	Ref<Node> parse_atom(Ref<TokenList> tokens);
};


class ExpressionParser {
public:
	Ref<Node> parse(std::string text);
};
