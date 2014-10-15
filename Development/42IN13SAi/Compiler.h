#pragma once
#include <list>
#include <iterator>
#include <string>
#include "CompilerNode.h"
#include "Token.h"
#include "Symbol.h"
#include "SymbolTable.h"
#include "Subroutine.h"
#include "SubroutineTable.h"

class Compiler
{
public:
	Compiler();
	Compiler(std::vector<Token> tokens);
	void Compile();
	virtual ~Compiler() throw();

private:
	const std::list<Token>::iterator it;
	void ParseFunctionOrGlobal();

protected:
	// Variables
	Parser* parser;
	std::list<Token> tokenizerTokens;
	std::list<CompilerNode> compilerNodes;
protected:
	// Variables
	
	std::vector<Token> tokenizerTokens;
	std::list<CompilerNode> *compilerNodes;
	SymbolTable symbolTable;
	SubroutineTable subroutineTable;
	Subroutine currentSubroutine;

	// Functions
	Token* PeekNext();
	Token  GetNext();
	void Match(TokenType type);
	void ParseGlobalStatement();
	void ParseStatement();

private:
	int currentToken = 0;
	int currentIndex = 0;
};