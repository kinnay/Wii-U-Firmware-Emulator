
#include "expression.h"
#include "common/sys.h"


bool is_name_head(char c) {
	if ('a' <= c && c <= 'z') return true;
	if ('A' <= c && c <= 'Z') return true;
	return c == '_';
}

bool is_name_char(char c) {
	if ('0' <= c && c <= '9') return true;
	return is_name_head(c);
}

bool is_special(char c) {
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')';
}

int parse_digit(char c, int base) {
	if ('0' <= c && c <= '9') return c - '0';
	
	if (base == 16) {
		if ('a' <= c && c <= 'f') return c - 'a' + 10;
		if ('A' <= c && c <= 'F') return c - 'A' + 10;
	}
	
	return -1;
}


Token::Token(Type type) {
	this->type = type;
}

Token::Token(Type type, std::string value) {
	this->type = type;
	name = value;
}

Token::Token(Type type, uint32_t value) {
	this->type = type;
	this->value = value;
}


TokenList::TokenList() {
	offset = 0;
}

void TokenList::add(Token::Type type) {
	tokens.push_back(new Token(type));
}

void TokenList::add(Token::Type type, std::string value) {
	tokens.push_back(new Token(type, value));
}

void TokenList::add(Token::Type type, uint32_t value) {
	tokens.push_back(new Token(type, value));
}

Ref<Token> TokenList::read() {
	if (offset < tokens.size()) {
		return tokens[offset++];
	}
	return nullptr;
}

Ref<Token> TokenList::peek() {
	if (offset < tokens.size()) {
		return tokens[offset];
	}
	return nullptr;
}

bool TokenList::eof() {
	return offset >= tokens.size();
}


bool Tokenizer::parse(std::string text, Ref<TokenList> tokens) {
	this->tokens = tokens;
	
	state = NEXT;
	
	for (char c : text) {
		if (!handle(c)) {
			return false;
		}
	}
	
	if (!handle(0)) return false;
	
	return true;
}

bool Tokenizer::handle(char c) {
	switch (state) {
		case NEXT: return state_next(c);
		case NUMBER_BASE: return state_number_base(c);
		case NUMBER_PREFIX: return state_number_prefix(c);
		case NUMBER: return state_number(c);
		case NAME: return state_name(c);
	}
	return false;
}

void Tokenizer::error(char c) {
	if (c) {
		Sys::stdout->write("Unexpected character: %c\n", c);
	}
	else {
		Sys::stdout->write("Unexpected end of input.\n");
	}
}

bool Tokenizer::state_next(char c) {
	if (c == '0') {
		state = NUMBER_BASE;
		return true;
	}
	else if ('1' <= c && c <= '9') {
		base = 10;
		number = c - '0';
		state = NUMBER;
		return true;
	}
	else if (is_name_head(c)) {
		name = c;
		state = NAME;
		return true;
	}
	else if (is_special(c)) {
		tokens->add(Token::SYMBOL, c);
		return true;
	}
	else if (c == 0) {
		return true;
	}
	error(c);
	return false;
}

bool Tokenizer::state_number_base(char c) {
	if (c == 'x') {
		base = 16;
		state = NUMBER_PREFIX;
		return true;
	}
	base = 10;
	number = 0;
	state = NUMBER;
	return state_number(c);
}

bool Tokenizer::state_number_prefix(char c) {
	number = parse_digit(c, base);
	if (number == -1) {
		error(c);
		return false;
	}
	
	state = NUMBER;
	return true;
}

bool Tokenizer::state_number(char c) {
	int digit = parse_digit(c, base);
	if (digit == -1) {
		tokens->add(Token::NUMBER, number);
		
		state = NEXT;
		return state_next(c);
	}
	
	if (number > (0xFFFFFFFF - digit) / base) {
		Sys::stdout->write("Number is too big.\n");
		return false;
	}
	
	number = number * base + digit;
	return true;
}

bool Tokenizer::state_name(char c) {
	if (is_name_char(c)) {
		name += c;
		return true;
	}
	
	tokens->add(Token::NAME, name);
	
	state = NEXT;
	return state_next(c);
}


void EvalContext::add(std::string name, uint32_t value) {
	vars[name] = value;
}

bool EvalContext::get(std::string name, uint32_t *value) {
	if (vars.count(name) > 0) {
		*value = vars[name];
		return true;
	}
	
	Sys::stdout->write("Unknown variable: %s\n", name);
	return false;
}


Node_Unary::Node_Unary(Type type, Ref<Node> child) {
	this->type = type;
	this->child = child;
}

bool Node_Unary::evaluate(Ref<EvalContext> context, uint32_t *value) {
	uint32_t temp;
	if (!child->evaluate(context, &temp)) {
		return false;
	}
	
	*value = -temp;
	return true;
}


Node_Binary::Node_Binary(Type type, Ref<Node> left, Ref<Node> right) {
	this->type = type;
	this->left = left;
	this->right = right;
}

bool Node_Binary::evaluate(Ref<EvalContext> context, uint32_t *value) {
	uint32_t left_val, right_val;
	if (!left->evaluate(context, &left_val) || !right->evaluate(context, &right_val)) {
		return false;
	}
	
	if (type == ADD) *value = left_val + right_val;
	else if (type == SUB) *value = left_val - right_val;
	else if (type == MUL) *value = left_val * right_val;
	else if (type == DIV) *value = left_val / right_val;
	return true;
}


Node_Const::Node_Const(uint32_t value) {
	this->value = value;
}

bool Node_Const::evaluate(Ref<EvalContext> context, uint32_t *value) {
	*value = this->value;
	return true;
}


Node_Var::Node_Var(std::string name) {
	this->name = name;
}

bool Node_Var::evaluate(Ref<EvalContext> context, uint32_t *value) {
	return context->get(name, value);
}


Ref<Node> Parser::parse(Ref<TokenList> tokens) {
	Ref<Node> expr = parse_expr(tokens);
	if (!expr || !tokens->eof()) {
		Sys::stdout->write("Syntax error in expression.\n");
		return nullptr;
	}
	return expr;
}


Ref<Node> Parser::parse_expr(Ref<TokenList> tokens) {
	Ref<Node> left = parse_term(tokens);
	if (!left) {
		return nullptr;
	}

	while (!tokens->eof()) {
		Ref<Token> token = tokens->peek();
		if (!token || token->type != Token::SYMBOL) {
			return nullptr;
		}
		
		char op = token->value;
		if (op != '+' && op != '-') {
			return left;
		}
		
		tokens->read();
		
		Ref<Node> right = parse_term(tokens);
		if (!right) {
			return nullptr;
		}
		
		if (op == '+') {
			left = new Node_Binary(Node_Binary::ADD, left, right);
		}
		else if (op == '-') {
			left = new Node_Binary(Node_Binary::SUB, left, right);
		}
	}
	
	return left;
}

Ref<Node> Parser::parse_term(Ref<TokenList> tokens) {
	Ref<Node> left = parse_atom(tokens);
	if (!left) {
		return nullptr;
	}

	while (!tokens->eof()) {
		Ref<Token> token = tokens->peek();
		if (!token || token->type != Token::SYMBOL) {
			return nullptr;
		}
		
		char op = token->value;
		if (op != '*' && op != '/') {
			return left;
		}
		
		tokens->read();
		
		Ref<Node> right = parse_atom(tokens);
		if (!right) {
			return nullptr;
		}
		
		if (op == '*') {
			left = new Node_Binary(Node_Binary::MUL, left, right);
		}
		else if (op == '/') {
			left = new Node_Binary(Node_Binary::DIV, left, right);
		}
	}
	
	return left;
}

Ref<Node> Parser::parse_atom(Ref<TokenList> tokens) {
	Ref<Token> token = tokens->read();
	if (!token) {
		return nullptr;
	}
	
	if (token->type == Token::SYMBOL) {
		if (token->value == '+') {
			return parse_atom(tokens);
		}
		else if (token->value == '-') {
			Ref<Node> child = parse_atom(tokens);
			if (!child) {
				return nullptr;
			}
			
			return new Node_Unary(Node_Unary::NEGATE, child);
		}
		else if (token->value == '(') {
			Ref<Node> expr = parse_expr(tokens);
			if (!expr) {
				return nullptr;
			}
			
			Ref<Token> token = tokens->read();
			if (!token || token->type != Token::SYMBOL || token->value != ')') {
				return nullptr;
			}
			return expr;
		}
	}
	else if (token->type == Token::NUMBER) {
		return new Node_Const(token->value);
	}
	else if (token->type == Token::NAME) {
		return new Node_Var(token->name);
	}
	return nullptr;
}


Ref<Node> ExpressionParser::parse(std::string text) {
	TokenList tokens;
	
	Tokenizer tokenizer;
	Parser parser;
	
	tokenizer.parse(text, &tokens);
	return parser.parse(&tokens);
}
