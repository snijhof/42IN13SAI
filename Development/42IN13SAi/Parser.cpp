#include "Parser.h"

Parser::Parser(Compiler* compiler) : compiler(compiler)
{
}

Parser::Parser(Compiler* compiler, std::vector<Token> tokens) : compiler(compiler)
{
	compiler->SetTokenList(tokens);
}

Parser::~Parser()
{
	//delete(compiler);
}

void Parser::ParseFunction()
{
	Token currentToken = compiler->GetNext();

	SymbolTable symbolTable;

	if (IsNextTokenReturnType())
	{
		Token returnType = compiler->GetNext();
		Token functionName = compiler->GetNext();

		compiler->Match(TokenType::OpenBracket);

		// Set the parameters
		while (compiler->PeekNext()->Type != TokenType::CloseBracket)
		{
			if (compiler->PeekNext()->Type == TokenType::Seperator)
			{
				compiler->GetNext(); // remove seperator so you can add a new parameter
			}
			else
			{
				Token parameter = compiler->GetNext();
				Symbol parameterSymbol = Symbol(parameter.Value, parameter.Type, SymbolKind::Parameter);
				

				if (!symbolTable.HasSymbol(parameterSymbol.name))
				{
					symbolTable.AddSymbol(parameterSymbol);
				}
				else
					throw std::runtime_error("Parameter name is already in use");
			}
		}

		// Check if the functions starts and create a subroutine
		compiler->Match(TokenType::OpenCurlyBracket);
		compiler->SetSubroutine(Subroutine(functionName.Value, returnType.Type, SubroutineKind::Function, symbolTable));

		// Set all the statements inside this subroutine
		while (compiler->PeekNext()->Type != TokenType::CloseCurlyBracket && compiler->PeekNext()->Level > 1)
		{
			compiler->ParseStatement();
		}

		// Check if the subroutine is closed correctly
		// And add the subroutine to the subroutine table
		compiler->Match(TokenType::CloseCurlyBracket);
		compiler->AddSubroutine();
	}
	else
		throw std::runtime_error("Expected return type");
}

// Also check and parse if-else statement
CompilerNode* Parser::ParseIfStatement()
{
	Token currentToken = compiler->GetNext();
	bool hasPartner = false;

	std::unique_ptr<Subroutine> subroutine(compiler->GetSubroutine());
	std::list<CompilerNode>* compilerNodes = subroutine->GetCompilerNodeCollection();
	int compilerNodesPos = compilerNodes->size();

	std::list<CompilerNode> innerIfStatementNodes;
	std::list<CompilerNode> innerElseStatementNodes;
	CompilerNode* statementNode;
	CompilerNode* endNode;

	if (currentToken.Type == TokenType::If)
	{
		if (currentToken.Partner != nullptr)
		{
			hasPartner = true;
		}
	}
	else
	{
		throw std::runtime_error("Expected if keyword");
	}

	compiler->Match(TokenType::OpenBracket);

	statementNode = ParseExpression();

	compiler->Match(TokenType::CloseBracket);
	compiler->Match(TokenType::OpenCurlyBracket);

	while ((currentToken = compiler->GetNext()).Type != TokenType::CloseCurlyBracket)
	{
		switch (currentToken.Type)
		{
		case TokenType::If:
			innerIfStatementNodes.push_back(*ParseIfStatement());
			break;
		case TokenType::While:
			ParseLoopStatement();
			break;
		case TokenType::Identifier:
			innerIfStatementNodes.push_back(*ParseAssignmentStatement());
			break;
		default:
			throw std::runtime_error("No statement found");
			break;
		}
	}

	if (hasPartner)
	{
		compiler->Match(TokenType::Else);
		compiler->Match(TokenType::OpenCurlyBracket);

		while ((currentToken = compiler->GetNext()).Type != TokenType::CloseCurlyBracket)
		{
			switch (currentToken.Type)
			{
			case TokenType::If:
				innerElseStatementNodes.push_back(*ParseIfStatement());
				break;
			case TokenType::While:
				ParseLoopStatement();
				break;
			case TokenType::Identifier:
				innerElseStatementNodes.push_back(*ParseAssignmentStatement());
				break;
			default:
				throw std::runtime_error("No statement found");
				break;
			}
		}
	}

	std::vector<std::string> doNothing;
	CompilerNode jumpTo = CompilerNode("$doNothing", "");

	statementNode->SetJumpTo(jumpTo);

	std::list<CompilerNode>::iterator it;
	it = compilerNodes->begin();
	//make it point to the correct compilernode
	for (int i = 0; i < compilerNodesPos; i++)
	{
		it++;
	}
	compilerNodes->insert(it, *statementNode);

	std::list<CompilerNode>::const_iterator iterator;
	for (iterator = innerIfStatementNodes.begin(); iterator != innerIfStatementNodes.end(); ++iterator) {
		compilerNodes->insert(it, *iterator);
	}

	for (iterator = innerElseStatementNodes.begin(); iterator != innerElseStatementNodes.end(); ++iterator) {
		compilerNodes->insert(it, *iterator);
	}

	compilerNodes->push_back(jumpTo);

	return nullptr;
}

CompilerNode* Parser::ParseExpression()
{
	CompilerNode* parsedExpr = ParseRelationalExpression();
	while (IsNextTokenLogicalOp())
	{
		Token logicalOp = compiler->GetNext();
		CompilerNode* secondParsedExpr = ParseRelationalExpression();
		std::vector<CompilerNode> parameters;

		switch (logicalOp.Type)
		{
		case TokenType::And:
			parameters.push_back(*parsedExpr);
			parameters.push_back(*secondParsedExpr);
			parsedExpr = new CompilerNode("$and", parameters, nullptr);
			break;
		case TokenType::Or:
			parameters.push_back(*parsedExpr);
			parameters.push_back(*secondParsedExpr);
			parsedExpr = new CompilerNode("$or", parameters, nullptr);
			break;
		}
	}

	return parsedExpr;
}

CompilerNode* Parser::ParseRelationalExpression()
{
	CompilerNode* parsedExpr = ParseAddExpression();
	while (IsNextTokenRelationalOp())
	{
		Token relOp = compiler->GetNext();
		CompilerNode* secondParsedExpr = ParseAddExpression();
		std::vector<CompilerNode> parameters;

		switch (relOp.Type)
		{
		case TokenType::LowerThan:
			parameters.push_back(*parsedExpr);
			parameters.push_back(*secondParsedExpr);
			parsedExpr = new CompilerNode("$less", parameters, nullptr);
			break;
		case TokenType::LowerOrEqThan:
			parameters.push_back(*parsedExpr);
			parameters.push_back(*secondParsedExpr);
			parsedExpr = new CompilerNode("$lessOrEq", parameters, nullptr);
			break;
		case TokenType::GreaterThan:
			parameters.push_back(*parsedExpr);
			parameters.push_back(*secondParsedExpr);
			parsedExpr = new CompilerNode("$greater", parameters, nullptr);
			break;
		case TokenType::GreaterOrEqThan:
			parameters.push_back(*parsedExpr);
			parameters.push_back(*secondParsedExpr);
			parsedExpr = new CompilerNode("$greaterOrEq", parameters, nullptr);
			break;
		case TokenType::Comparator:
			parameters.push_back(*parsedExpr);
			parameters.push_back(*secondParsedExpr);
			parsedExpr = new CompilerNode("$equals", parameters, nullptr);
			break;
		}
	}

	return parsedExpr;
}

CompilerNode* Parser::ParseAddExpression()
{
	CompilerNode* parsedExpr = ParseMulExpression();
	while (IsNextTokenAddOp())
	{
		Token addOp = compiler->GetNext();
		CompilerNode* secondParsedExpr = ParseMulExpression();
		std::vector<CompilerNode> parameters;

		switch (addOp.Type)
		{
		case TokenType::OperatorPlus:
			parameters.push_back(*parsedExpr);
			parameters.push_back(*secondParsedExpr);
			parsedExpr = new CompilerNode("$add", parameters, nullptr);
			break;
		case TokenType::OperatorMinus:
			parameters.push_back(*parsedExpr);
			parameters.push_back(*secondParsedExpr);
			parsedExpr = new CompilerNode("$min", parameters, nullptr);
			break;
		}
	}

	return parsedExpr;
}

CompilerNode* Parser::ParseMulExpression()
{
	CompilerNode* term = ParseUniExpression();
	while (IsNextTokenMulOp())
	{
		Token mullOp = compiler->GetNext();
		CompilerNode* secondTerm = ParseUniExpression();
		std::vector<CompilerNode> parameters;

		switch (mullOp.Type)
		{
		case TokenType::OperatorMultiply:
			parameters.push_back(*term);
			parameters.push_back(*secondTerm);
			term = new CompilerNode("$mul", parameters, nullptr);
			break;
		case TokenType::OperatorDivide:
			parameters.push_back(*term);
			parameters.push_back(*secondTerm);
			term = new CompilerNode("$div", parameters, nullptr);
			break;
		case TokenType::OperatorRaised:
			parameters.push_back(*term);
			parameters.push_back(*secondTerm);
			term = new CompilerNode("$raise", parameters, nullptr);
			break;
		}
	}

	return term;
}

CompilerNode* Parser::ParseUniExpression()
{
	CompilerNode* term = ParseTerm();

	while (IsNextTokenUniOp())
	{
		Token uniOp = compiler->GetNext();
		std::vector<CompilerNode> parameters;

		switch (uniOp.Type)
		{
		case TokenType::UniOperatorPlus:
			parameters.push_back(*term);
			term = new CompilerNode("$uniPlus", parameters, nullptr);
			compiler->Match(TokenType::EOL);
			break;
			parameters.push_back(*term);
			term = new CompilerNode("$uniMin", parameters, nullptr);
			compiler->Match(TokenType::EOL);
			break;
		}
	}

	return term;
}

CompilerNode* Parser::ParseTerm()
{
	Token token = compiler->GetNext();

	CompilerNode* node = nullptr;

	if (token.Type == TokenType::Float)
	{
		node = new CompilerNode("$value", token.Value);
		return node;
	}
	else if (token.Type == TokenType::Identifier)
	{
		std::string identifier = token.Value;

		Symbol* symbol = GetSymbol(identifier);

		if (symbol == nullptr)
			throw SymbolNotFoundException("");

		node = new CompilerNode("$getVariable", symbol->name);
		return node;
	}
	else if (token.Type == TokenType::OpenBracket)
	{
		node = ParseExpression();
		compiler->Match(TokenType::CloseBracket);
		return node;
	}

	return node;
}

bool Parser::IsNextTokenLogicalOp()
{
	std::vector<TokenType> operators{ TokenType::And, TokenType::Or };
	return std::find(operators.begin(), operators.end(), compiler->PeekNext()->Type) != operators.end();
}

bool Parser::IsNextTokenRelationalOp()
{
	std::vector<TokenType> operators{ TokenType::GreaterThan, TokenType::GreaterOrEqThan, TokenType::LowerThan, TokenType::LowerOrEqThan };
	return std::find(operators.begin(), operators.end(), compiler->PeekNext()->Type) != operators.end();
}

bool Parser::IsNextTokenAddOp()
{
	std::vector<TokenType> operators{ TokenType::OperatorPlus, TokenType::OperatorMinus };
	return std::find(operators.begin(), operators.end(), compiler->PeekNext()->Type) != operators.end();
}

bool Parser::IsNextTokenMulOp()
{
	std::vector<TokenType> operators{ TokenType::OperatorMultiply, TokenType::OperatorDivide, TokenType::OperatorRaised };
	return std::find(operators.begin(), operators.end(), compiler->PeekNext()->Type) != operators.end();
}

bool Parser::IsNextTokenUniOp()
{
	std::vector<TokenType> operators{ TokenType::UniOperatorPlus, TokenType::UniOperatorMinus };
	return std::find(operators.begin(), operators.end(), compiler->PeekNext()->Type) != operators.end();
}

bool Parser::IsNextTokenReturnType()
{
	std::vector<TokenType> operators{ TokenType::Void, TokenType::None, TokenType::Float };
	return std::find(operators.begin(), operators.end(), compiler->PeekNext()->Type) != operators.end();
}

/*
Parse while and for loops
@param standard compilerNodes.size()
*/
void Parser::ParseLoopStatement()
{
	Token currentToken = compiler->GetNext();
	bool forLoop = false;

	std::unique_ptr<Subroutine> subroutine(compiler->GetSubroutine());
	std::list<CompilerNode>* compilerNodes = subroutine->GetCompilerNodeCollection();
	int compilerNodesPos = compilerNodes->size();

	std::vector<CompilerNode> nodeParameters;
	std::string statementExpression;

	CompilerNode statementNode;
	std::list<CompilerNode> innerStatementNodes;

	if (currentToken.Type != TokenType::While || currentToken.Type != TokenType::ForLoop)
	{
		throw std::runtime_error("Expected a loop keyword");
	}
	else
	{
		if (currentToken.Type == TokenType::ForLoop)
		{
			forLoop = true;
		}
	}

	compiler->Match(TokenType::OpenBracket);

	if (forLoop)
	{
		nodeParameters.push_back(*ParseAssignmentStatement());
		nodeParameters.push_back(*ParseExpression());
		nodeParameters.push_back(*ParseAddExpression());

		statementExpression = "$forLoop";
	}
	else
	{
		nodeParameters.push_back(*ParseExpression());

		statementExpression = "$whileLoop";
	}

	compiler->Match(TokenType::CloseBracket);
	compiler->Match(TokenType::OpenCurlyBracket);

	while ((currentToken = compiler->GetNext()).Type != TokenType::CloseCurlyBracket)
	{
		switch (currentToken.Type)
		{
		case TokenType::If:
			innerStatementNodes.push_back(*ParseIfStatement());
			break;
		case TokenType::While:
			ParseLoopStatement();
			break;
		case TokenType::Identifier:
			innerStatementNodes.push_back(*ParseAssignmentStatement());
			break;
		default:
			throw std::runtime_error("No statement found");
			break;
		}
	}

	std::vector<std::string> doNothing;
	CompilerNode jumpTo = CompilerNode("$doNothing", "");

	statementNode = CompilerNode(statementExpression, nodeParameters, &jumpTo);

	std::list<CompilerNode>::iterator it;
	it = compilerNodes->begin();
	//make it point to the correct compilernode
	for (int i = 0; i < compilerNodesPos; i++)
	{
		it++;
	}
	compilerNodes->insert(it, statementNode);

	std::list<CompilerNode>::const_iterator iterator;
	for (iterator = innerStatementNodes.begin(); iterator != innerStatementNodes.end(); ++iterator) {
		compilerNodes->insert(it, *iterator);
	}

	compilerNodes->push_back(jumpTo);
}

/*
Also parse (standard) Arithmetical operations
*/
CompilerNode* Parser::ParseAssignmentStatement()
{
	std::string expression = "";
	std::vector<CompilerNode*> nodeParameters;
	CompilerNode endNode;

	bool newIdentifier = false;

	Token currentToken = compiler->GetNext();
	if (currentToken.Type == TokenType::Var)
	{
		newIdentifier = true;
		currentToken = compiler->GetNext();
	}
	if (currentToken.Type == TokenType::Identifier)
	{
		CompilerNode* node = new CompilerNode("$identifier", currentToken.Value);
		nodeParameters.push_back(node);

		if (newIdentifier)
		{
			//std::unique_ptr<Subroutine> subroutine(compiler->GetSubroutine());

			//TODO check if global or local

			Symbol identifierSymbol = Symbol(currentToken.Value, currentToken.Type, SymbolKind::Global);

			if (!compiler->HasSymbol(identifierSymbol.name))
			{
				compiler->AddSymbol(identifierSymbol);
			}
			else
				throw std::runtime_error("Identifier name is already in use");
		}
	}
	else
	{
		throw std::runtime_error("Identifier expected");
	}

	currentToken = compiler->GetNext();

	if (currentToken.Type == TokenType::Equals)
	{
		expression = "$assignment";
		CompilerNode* node = ParseExpression();
		nodeParameters.push_back(node);
	}

	compiler->Match(TokenType::EOL);

	//endNode = CompilerNode(expression, nodeParameters, nullptr);

	return &endNode;
}

// This function will return a symbol based on the identifier parameter.
// It will not just check the global symboltable but will first check the 
// symboltable of the current subroutine.
Symbol* Parser::GetSymbol(std::string identifier)
{
	Symbol* symbol;
	std::unique_ptr<Subroutine> subroutine(compiler->GetSubroutine());

	if (!subroutine->isEmpty)
	{
		if (subroutine->HasLocal(identifier))
			symbol = subroutine->GetLocal(identifier);
		else
			symbol = compiler->GetSymbol(identifier);
	}
	else
	{
		symbol = compiler->GetSymbol(identifier);
	}

	return symbol;
}
