#define _USE_MATH_DEFINES
#include "VirtualMachine.h"
#include <iostream>
#include <math.h>

VirtualMachine::VirtualMachine(SymbolTable* symboltable, SubroutineTable* subroutine, std::shared_ptr<LinkedList> globalsNodes)
: globalsSymboltable(symboltable), subroutineTable(subroutine), globalsList(globalsNodes)
{
	function_caller = std::unique_ptr<FunctionCaller>(new FunctionCaller(this));
	
	currentSymbolTable = nullptr;
	currentSubroutine = nullptr;
}

VirtualMachine::VirtualMachine(const VirtualMachine &other) : globalsSymboltable(other.globalsSymboltable), currentSubroutine(other.currentSubroutine), currentSymbolTable(other.currentSymbolTable), subroutineTable(other.subroutineTable), globalsList(other.globalsList)
{
	function_caller = std::unique_ptr<FunctionCaller>(new FunctionCaller(this));
}

VirtualMachine& VirtualMachine::operator=(const VirtualMachine& other)
{
	if (this != &other)
	{
		VirtualMachine* cVirtualMachine = new VirtualMachine(other);
		return *cVirtualMachine;
		
	}
	return *this;
}

VirtualMachine::~VirtualMachine(){}

std::shared_ptr<CompilerNode> VirtualMachine::GetNext(std::shared_ptr<LinkedList> nodes)
{
	std::shared_ptr<ListNode> node = nodes->GetNext();

	if (!node)
	{
		node.reset();
		throw MissingCompilerNodeException("Compilernode missing");
	}

	return node->GetData();
}


void VirtualMachine::ExecuteCode()
{
	// Only when there are compilernodes for global vars
	if (globalsList->size() > 0)
	{
		// First check all compilernodes for global variables
		std::shared_ptr<CompilerNode> node;
		do
		{
			node = VirtualMachine::GetNext(std::shared_ptr<LinkedList>(globalsList));
			std::string function_call = node->GetExpression();
			function_caller->Call(function_call, *node);

		} while (node != globalsList->GetTailData());
	}
	
	// Find main subroutine
	currentSubroutine = subroutineTable->GetSubroutine("main");
	
	if (currentSubroutine == nullptr)
		throw MissingMainFunctionException("No main function found");
	
	currentSymbolTable = currentSubroutine->GetSymbolTable();
	
	// Get the main nodes and execute them
	ExecuteNodes(std::shared_ptr<LinkedList>(currentSubroutine->GetCompilerNodeCollection()));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteNodes(std::shared_ptr<LinkedList> nodes)
{
	// Get the subroutine name
	std::string subroutineName = currentSubroutine->name;
	// Insert the linkedlist for this subroutine
	nodeLists.insert(std::pair<std::string, std::shared_ptr<LinkedList>>
						 (subroutineName, nodes));

	if (nodes->size() > 0)
	{
		std::shared_ptr<CompilerNode> node;
		do
		{
			node = VirtualMachine::GetNext(nodeLists[subroutineName]);

			if (node)
			{
				// Get the function name
				std::string function_call = node->GetExpression();

				// if it is a return function return the CompilerNode
				if (function_call == "$ret")
				{
					return function_caller->Call(function_call, *node);
				}
				else if (function_call == "$doNothing")
				{
					// Set the current node to the partner of the donothing node
					if (node->GetJumpTo())
					{
						nodeLists[subroutineName]->SetCurrent(node->GetJumpTo());
						node = nodeLists[subroutineName]->GetCurrentData();
					}
				}
				else
					function_caller->Call(function_call, *node);
			}

		} while (node != std::shared_ptr<CompilerNode>());
	}
	
	// Erase the LinkedList
	nodeLists.erase(subroutineName);
	
	return nullptr;
}

std::shared_ptr<CompilerNode> VirtualMachine::CallFunction(CompilerNode node)
{
	std::string function_call = node.GetExpression();
	// call the compilernode function
	return function_caller->Call(function_call, node);
}

std::vector<std::shared_ptr<CompilerNode>> VirtualMachine::CheckParameters(CompilerNode &compilerNode, int amount)
{
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(amount, ParameterExceptionType::NoParameters);

	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();

	// Check if there aren't more than one parameter
	if (parameters.size() != amount)
		throw ParameterException(amount, parameters.size(), ParameterExceptionType::IncorrectParameters);

	for (int i = 0; i < parameters.size(); i++)
	{
		std::shared_ptr<CompilerNode> param = parameters.at(i);
		// Check if the parameters are a value or another function call
		// if function call, execute function
		if (param->GetExpression() != "$value")
			parameters.at(i) = CallFunction(*param);
	}
	return parameters;
}

#pragma region FunctionOperations

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteFunction(CompilerNode compilerNode)
{
	// Check if params is not empty
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(1, ParameterExceptionType::NoParameters);

	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
	std::shared_ptr<CompilerNode> functionNode = parameters.at(0);

	// Check if node contains the functionname
	if (functionNode->GetExpression() != "$functionName")
		throw FunctionNameExpectedException("Expected function name");

	// Get the subroutine table and check if exists
	currentSubroutine = subroutineTable->GetSubroutine(functionNode->GetValue());
	if (currentSubroutine == nullptr)
		throw SubroutineNotFoundException("Function " + functionNode->GetValue() + " does not exist");

	// Set the currentSymbolTable
	currentSymbolTable = currentSubroutine->GetSymbolTable();

	// Get parameter count and check if enough parameters are given
	int parameterCount = currentSymbolTable->ParameterSize();
	if (parameters.size() - 1 != parameterCount)
		throw ParameterException((int)parameters.size() - 1, parameterCount, ParameterExceptionType::IncorrectParameters);

	// Set the currentSymbolTable symbol values
	int paramNum = 1;
	std::vector<Symbol*> vSymbols = currentSymbolTable->GetSymbolVector();
	for (Symbol* symbol : vSymbols)
	{
		std::shared_ptr<CompilerNode> param = parameters.at(paramNum);
		if (param->GetExpression() != "$value")
			param = CallFunction(*param);

		float fParam = atof(param->GetValue().c_str());
		symbol->SetValue(fParam);
		paramNum++;
	}

	return VirtualMachine::ExecuteNodes(currentSubroutine->GetCompilerNodeCollection());
}
std::shared_ptr<CompilerNode> VirtualMachine::ExecuteReturn(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Create the return node
	std::shared_ptr<CompilerNode> returnNode = std::make_shared<CompilerNode>(CompilerNode("$value", param1->GetValue(), false));

	return returnNode;
}

#pragma endregion FunctionOperations

#pragma region VariableOperations
std::shared_ptr<CompilerNode> VirtualMachine::ExecuteAssignment(CompilerNode compilerNode)
{
	// Check if params is not empty
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(2, ParameterExceptionType::NoParameters);

	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = compilerNode.GetNodeparameters();

	// Check if there aren't more than two parameters
	if (parameters.size() > 2)
		throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);

	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Only go through when param is identifier
	if (param1->GetExpression() == "$identifier")
	{
		// Get the value of the node -> variable
		std::string variableName = param1->GetValue();
		if (param2->GetExpression() != "$value")
			param2 = CallFunction(*param2);

		// Get the variable from symboltable
		// first check subSymbolTable
		Symbol* current_symbol = nullptr;
		if (currentSymbolTable != nullptr)
			current_symbol = currentSymbolTable->GetSymbol(variableName);

		// if not in the subSymbolTable get from globals symboltable
		if (current_symbol == nullptr)
			current_symbol = globalsSymboltable->GetSymbol(variableName);

		// Get the param value and set in temp var
		float valueToSet = atof(param2->GetValue().c_str());
		current_symbol->SetValue(valueToSet);
	}

	return nullptr;
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteGetVariable(CompilerNode compilerNode)
{
	// Check if params is not empty
	if (compilerNode.GetValue().empty())
		throw ParameterException(1, ParameterExceptionType::NoParameters);

	// Get the Node parameter
	std::string parameter = compilerNode.GetValue();

	// Get the variable from symboltable
	// first check subSymbolTable
	Symbol* current_symbol = nullptr;
	if (currentSymbolTable != nullptr)
		current_symbol = currentSymbolTable->GetSymbol(parameter);

	// if not in the subSymbolTable get from globals symboltable
	if (current_symbol == nullptr)
		current_symbol = globalsSymboltable->GetSymbol(parameter);

	// Create the return node
	std::shared_ptr<CompilerNode> returnNode = std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(current_symbol->GetValue()), false));

	return returnNode;
}
#pragma endregion VariableOperations

#pragma region DefaultOperations
std::shared_ptr<CompilerNode> VirtualMachine::ExecutePrint(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Get the new value
	std::string valueToPrint = param1->GetValue();

	//Add the value to print to the output
	output.push_back(valueToPrint);

	// Print te new value
	//std::cout << valueToPrint << std::endl;
	//std::cin.get();

	return nullptr;
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteStop(CompilerNode compilerNode)
{
	if (compilerNode.GetNodeparameters().empty())
	{
		if (compilerNode.GetExpression() == "$stop")
			std::exit(1);
		else
			throw UnknownExpressionException("Unknown expression type");
	}
	else
		throw new ParameterException(0, ParameterExceptionType::IncorrectParameters);
}

#pragma endregion DefaultOperations

#pragma region LoopOperations

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteWhile(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> condition = parameters.at(0);

	// Check if condition is true.
	if (condition->GetValue() == "1")
	{
		return nullptr;
	}
	else
	{
		// Condition is false, move linkedlist to donothing node
		nodeLists[currentSubroutine->name]->SetCurrent(compilerNode.GetJumpTo(), true);
		return nullptr;
	}
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteFor(CompilerNode compilerNode)
{
	// Check if nodeparams are not empty
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(3, ParameterExceptionType::NoParameters);

	std::vector<std::shared_ptr<CompilerNode> > parameters = compilerNode.GetNodeparameters();
	// Check if count of params is not right
	if (parameters.size() != 3)
		throw ParameterException(3, parameters.size(), ParameterExceptionType::IncorrectParameters);

	std::shared_ptr<CompilerNode> assignment = parameters.at(0);
	if (assignment->GetExpression() == "$assignment")
	{
		std::shared_ptr<CompilerNode> identifier = assignment->GetNodeparameters().at(0);
		// If the symboltable doesn't contain the variable it's the first time in the for loop.
		// Add the symbol to the table so it doesn't keep getting reset
		if (currentSymbolTable->GetSymbol(identifier->GetValue()) == nullptr)
		{
			currentSymbolTable->AddSymbol(Symbol(identifier->GetValue(), MyTokenType::Float, SymbolKind::Local));
			CallFunction(*assignment);
		}
	}

	std::shared_ptr<CompilerNode> condition = parameters.at(1);
	if (condition->GetExpression() != "$value")
		condition = CallFunction(*condition);

	// Check if condition is true.
	if (condition->GetValue() == "1")
	{
		std::shared_ptr<CompilerNode> expression = parameters.at(2);
		if (expression->GetExpression() != "$value")
			expression = CallFunction(*expression);

		return nullptr;
	}
	else
	{
		// Condition is false, move linkedlist to donothing node
		nodeLists[currentSubroutine->name]->SetCurrent(compilerNode.GetJumpTo(), true);
		return nullptr;
	}
}

#pragma endregion LoopOperations

#pragma region ConditionalOperations

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteIf(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> condition = parameters.at(0);

	if (condition->GetValue() == "1")
	{
		return nullptr;
	}
	else
	{
		// Condition is false, move linkedlist to donothing node
		nodeLists[currentSubroutine->name]->SetCurrent(compilerNode.GetJumpTo(), true);
		return nullptr;
	}
}

#pragma endregion ConditionalOperations

#pragma region ConditionalStatements
std::shared_ptr<CompilerNode> VirtualMachine::ExecuteLessCondition(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Set numbers / values
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	bool output = num1 < num2;

	// Set boolean to true if num1 < num2, else return false (inside the node)
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteLessOrEqCondition(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Set numbers / values
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	bool output = num1 <= num2;

	// Set boolean to true if num1 < num2, else return false (inside the node)
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteGreaterCondition(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Set numbers / values
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	bool output = num1 > num2;

	// Set boolean to true if num1 > num2, else return false (inside the node)
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteGreaterOrEqCondition(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Set numbers / values
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	bool output = num1 >= num2;

	// Set boolean to true if num1 > num2, else return false (inside the node)
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteEqualCondition(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Set numbers / values
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	bool output = num1 == num2;

	// Set boolean to true if num1 == num2, else return false (inside the node)
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteNotEqualCondition(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Set numbers / values
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	bool output = num1 != num2;

	// Set boolean to true if num1 != num2, else return false (inside the node)
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}
#pragma endregion ConditionalStatements

#pragma region SimpleMath
std::shared_ptr<CompilerNode> VirtualMachine::ExecuteAddOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	float output = num1 + num2;

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteMinusOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	float output = num1 - num2;

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteMultiplyOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	float output = num1 * num2;

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteDivideOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	float output = num1 / num2;

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteUniMinOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float output = num1 - 1;

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteUniPlusOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float output = num1 + 1;

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

#pragma endregion SimpleMath

#pragma region ComplexMath
std::shared_ptr<CompilerNode> VirtualMachine::ExecuteSqrOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());

	float output = std::pow(num1, 2);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteCbcOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());

	float output = std::pow(num1, 3);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecutePowOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());

	// TODO CHECKS

	float output = std::pow(num1, num2);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteSqrtOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());

	float output = std::sqrt(num1);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteCbrtOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());

	float output = std::cbrt(num1);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}


std::shared_ptr<CompilerNode> VirtualMachine::ExecuteSinOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float output = std::sin(num1);

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteCosOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float output = std::cos(num1);

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteTanOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float output = std::tan(num1);

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteDegreeOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float output = num1 * (180 / M_PI);

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteRadiantOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float output = num1 * (M_PI / 180);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecutePercentOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	if (num2 == 0)
	{
		//TODO: Throw division by zero exception
	}
	float output = (num1 / num2) * 100;
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecutePermillageOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	if (num2 == 0)
	{
		//TODO: Throw division by zero exception
	}
	float output = (num1 / num2) * 1000;
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteCommonLogOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	
	if (num1 == 0)
		throw InvalidInputException("Invalid Input");


	float output = std::log10(num1);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteBinaryLogOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());

	if (num1 == 0)
		throw InvalidInputException("Invalid Input");

	float output = std::log2(num1);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteNaturalLogOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 1);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());

	if (num1 == 0)
		throw InvalidInputException("Invalid Input");

	float output = std::log(num1);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteLogOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());

	if (num1 == 0 || num2 == 0)
		throw InvalidInputException("Invalid Input");

	float output = std::log(num1) / std::log(num2);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteModuloOperation(CompilerNode compilerNode)
{
	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode> > parameters = CheckParameters(compilerNode, 2);
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());

	if (num2 == 0)
		throw ZeroDivideException("Division by 0 exception occured.");

	float output = std::fmod(num1,num2);
	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}


#pragma endregion ComplexMath