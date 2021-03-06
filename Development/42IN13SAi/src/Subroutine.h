//
//  Subroutine.h
//  mac_42IN13SAI
//
//  Copyright (c) 2014 42IN13SAI. All rights reserved.
//

#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include "LinkedList.h"
#include "Symbol.h"
#include "SymbolTable.h"
#include "SubroutineKind.h"
#include "Token.h"

class Subroutine {
private:
	SymbolTable symbolTable;
	std::shared_ptr<LinkedList> subroutineCompilerNodes;

public:	
	SubroutineKind kind;
	MyTokenType returnType;
	std::string name;
	bool isEmpty;
	
	Subroutine();
	Subroutine(std::string p_name, MyTokenType p_return, SubroutineKind p_kind, SymbolTable p_parameters);
	Subroutine& operator=(const Subroutine& other);
	virtual ~Subroutine();
	
	void AddLocal(Symbol symbol);
	bool HasLocal(std::string name);
	Symbol* GetLocal(std::string name);
	SymbolTable* GetSymbolTable();

	std::shared_ptr<LinkedList> GetCompilerNodeCollection();
	std::vector<std::shared_ptr<CompilerNode>> GetCompilerNodeVector();
	void AddCompilerNode(std::shared_ptr<CompilerNode> node);
};


