#include "VirtualMachine.h"
#include <iostream>

VirtualMachine::VirtualMachine(SymbolTable* symboltable, SubroutineTable* subroutine, std::list<std::shared_ptr<CompilerNode>> compiler_nodes)
	: mainSymboltable(symboltable), subroutineTable(subroutine), compilerNodes(compiler_nodes)
{
    function_caller = std::unique_ptr<FunctionCaller>(new FunctionCaller(this));

    subSymbolTable = nullptr;
    subSubroutine = nullptr;
}

VirtualMachine::VirtualMachine(const VirtualMachine &other) :mainSymboltable(other.mainSymboltable), subSubroutine(other.subSubroutine), subSymbolTable(other.subSymbolTable), subroutineTable(other.subroutineTable), compilerNodes(other.compilerNodes)
{
    
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

CompilerNode VirtualMachine::PeekNext(int _currentIndex, std::list<std::shared_ptr<CompilerNode>> nodes)
{
    std::shared_ptr<CompilerNode> node = std::make_shared<CompilerNode>(CompilerNode());
	if (nodes.size() - 1 > _currentIndex || _currentIndex == -1)
	{
        // release node
        node.reset();
        
		std::list<std::shared_ptr<CompilerNode>>::iterator it = nodes.begin();
		std::advance(it, _currentIndex + 1);
		node = *it;
	}
       /*node = nodes.at(_currentIndex + 1);*/
	return *node;
}

CompilerNode VirtualMachine::PeekPrevious(int _currentIndex, std::list<std::shared_ptr<CompilerNode>> nodes)
{
    std::shared_ptr<CompilerNode> node = std::make_shared<CompilerNode>(CompilerNode());
	if (_currentIndex >= 1)
	{
        // release node
        node.reset();
        
		std::list<std::shared_ptr<CompilerNode>>::iterator it = nodes.begin();
		std::advance(it, _currentIndex - 1);
		node = *it;
	}
        /*node = nodes.at(_currentIndex - 1);*/
    return *node;
}

CompilerNode VirtualMachine::GetNext(int* _currentIndex, std::list<std::shared_ptr<CompilerNode>> nodes)
{
	std::shared_ptr<CompilerNode> node = std::make_shared<CompilerNode>(CompilerNode());
    (*_currentIndex)++;
	if (*_currentIndex <= nodes.size()-1 && nodes.size() > 0)
	{
        // release node
        node.reset();
        
		std::list<std::shared_ptr<CompilerNode>>::iterator it = nodes.begin();
		if (*_currentIndex > 0)
		{
			std::advance(it, *_currentIndex);
		}
		
		node = *it;
		/*node = nodes.at(*_currentIndex);*/
	}
	else
	{
		throw std::runtime_error("Compilernode missing");
	}

	return *node;
}


void VirtualMachine::ExecuteCode()
{
	// Only when there are compilernodes for global vars
	if (compilerNodes.size() > 0)
	{
		// First check all compilernodes for global variables
		do 
		{
			if (PeekNext(currentIndex, compilerNodes).GetExpression() != "")
			{
				CompilerNode node = VirtualMachine::GetNext(&currentIndex,compilerNodes);
				std::string function_call = node.GetExpression();
				function_caller->Call(function_call, node);
			}
		} while (currentIndex < compilerNodes.size()-1);
	}
    // Find main subroutine
    subSubroutine = subroutineTable->GetSubroutine("main");
    if (subSubroutine == nullptr)
        throw std::runtime_error("No main function found");
    subSymbolTable = subSubroutine->GetSymbolTable();
    ExecuteNodes(subSubroutine->GetCompilerNodeCollection());
    
    //std::vector<CompilerNode> mainNodes = *subSubroutine->GetCompilerNodeVector();
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteNodes(std::list<std::shared_ptr<CompilerNode>> nodes)
{
    int* _currentIndex = new int(-1);
    std::list<std::shared_ptr<CompilerNode>> compilernodes = nodes;
    do
    {
        if (PeekNext(*_currentIndex, compilernodes).GetExpression() != "")
        {
            CompilerNode node = VirtualMachine::GetNext(_currentIndex, compilernodes);
            
            CompilerNode previous = PeekPrevious(*_currentIndex, compilernodes);
            if (previous.GetExpression() != "" && previous.GetExpression() == "$whileLoop")
            {
                while (PeekNext(*_currentIndex, compilernodes).GetExpression() != "$doNothing")
                {
                    node = VirtualMachine::GetNext(_currentIndex, compilernodes);
                }
                node = VirtualMachine::GetNext(_currentIndex, compilernodes);
            }
            
            // commentaar
            std::string function_call = node.GetExpression();
            
            if (function_call == "$ret")
            {
                delete _currentIndex;
                _currentIndex = nullptr;
                return function_caller->Call(function_call, node);
            }
            else if (function_call == "$doNothing")
                break;
            else
                function_caller->Call(function_call, node);
        }
    } while (*_currentIndex < compilernodes.size()-1);
    
    delete _currentIndex;
    _currentIndex = nullptr;
    
    return nullptr;
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteNodes(std::list<std::shared_ptr<CompilerNode>> nodes, int currenIndex)
{
    int* _currentIndex = new int(currenIndex);
    std::list<std::shared_ptr<CompilerNode>> compilernodes = nodes;
    do
    {
        if (PeekNext(*_currentIndex, compilernodes).GetExpression() != "")
        {
            CompilerNode node = VirtualMachine::GetNext(_currentIndex, compilernodes);
            std::string function_call = node.GetExpression();
            
            if (function_call == "$ret")
            {
                delete _currentIndex;
                _currentIndex = nullptr;
                return function_caller->Call(function_call, node);
            }
            else if (function_call == "$doNothing")
                break;
            else
                function_caller->Call(function_call, node);
        }
    } while (*_currentIndex < compilernodes.size()-1);
    
    delete _currentIndex;
    _currentIndex = nullptr;
    
    return nullptr;
}

std::shared_ptr<CompilerNode> VirtualMachine::CallFunction(CompilerNode node)
{
    std::string function_call = node.GetExpression();
    // call the compilernode function
    return function_caller->Call(function_call, node);
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
        throw std::runtime_error("Expected function name");
    
    // Get the subroutine table and check if exists
    subSubroutine = subroutineTable->GetSubroutine(functionNode->GetValue());
    if (subSubroutine == nullptr)
        throw std::runtime_error("Subroutine for function " + functionNode->GetValue() + " not found");
    
    // Set the subSymbolTable
    subSymbolTable = subSubroutine->GetSymbolTable();
    
    // Get parameter count and check if enough parameters are given
    int parameterCount = subSymbolTable->ParameterSize();
    if (parameters.size() - 1 != parameterCount)
        throw ParameterException((int)parameters.size() - 1, parameterCount, ParameterExceptionType::IncorrectParameters);
    
    // Set the subSymbolTable symbol values
    int paramNum = 1;
    std::vector<Symbol*> vSymbols = subSymbolTable->GetSymbolVector();
    for (Symbol* symbol : vSymbols)
    {
        std::shared_ptr<CompilerNode> param = parameters.at(paramNum);
        if (param->GetExpression() != "$value")
            param = CallFunction(*param);
        
        float fParam = atof(param->GetValue().c_str());
        symbol->SetValue(fParam);
        paramNum++;
    }
    
    return VirtualMachine::ExecuteNodes(subSubroutine->GetCompilerNodeCollection());
}
std::shared_ptr<CompilerNode> VirtualMachine::ExecuteReturn(CompilerNode compilerNode)
{
    // Check if params is not empty
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(1, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than two parameters
    if (parameters.size() > 1)
        throw ParameterException(1, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    
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
	std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
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
        if (subSymbolTable != nullptr)
            current_symbol = subSymbolTable->GetSymbol(variableName);
        
        // if not in the subSymbolTable get from main symboltable
        if (current_symbol == nullptr)
            current_symbol = mainSymboltable->GetSymbol(variableName);
        
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
    if (subSymbolTable != nullptr)
        current_symbol = subSymbolTable->GetSymbol(parameter);
    
    // if not in the subSymbolTable get from main symboltable
    if (current_symbol == nullptr)
        current_symbol = mainSymboltable->GetSymbol(parameter);
    
    // Create the return node
	std::shared_ptr<CompilerNode> returnNode = std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(current_symbol->GetValue()), false));
    
    return returnNode;
}
#pragma endregion VariableOperations

#pragma region DefaultOperations
std::shared_ptr<CompilerNode> VirtualMachine::ExecutePrint(CompilerNode compilerNode)
{
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(1, ParameterExceptionType::NoParameters);

	std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
	if (parameters.size() > 1)
		throw ParameterException(1, parameters.size(), ParameterExceptionType::IncorrectParameters);

	// Get first and only parameter
	std::shared_ptr<CompilerNode> param1 = parameters.at(0);

	// Check if expression is not value
	if (param1->GetExpression() != "$value")
		param1 = CallFunction(*param1);

	// Get the new value
	std::string valueToPrint = param1->GetValue();

	// Print te new value
	std::cout << valueToPrint << std::endl;
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
			throw std::runtime_error("Unknown expression type");
	}
	else
		throw new ParameterException(0, ParameterExceptionType::IncorrectParameters);
}

#pragma endregion DefaultOperations

#pragma region LoopOperations

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteWhile(CompilerNode compilerNode)
{
    // Check if nodeparams are not empty
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(1, ParameterExceptionType::NoParameters);
    
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    // Check if count of params is not right
    if (parameters.size() > 1)
        throw ParameterException(1, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> condition = parameters.at(0);
    
    if (condition->GetExpression() != "$value")
        condition = CallFunction(*condition);
    
    
    std::vector<std::shared_ptr<CompilerNode>> compilerNodes;
	if (subSubroutine == nullptr)
	{
		subSubroutine = subroutineTable->GetSubroutine("main");
		compilerNodes = subSubroutine->GetCompilerNodeVector();
	}
	else
		compilerNodes = subSubroutine->GetCompilerNodeVector();
    
    
    if (condition->GetValue() == "1")
    {
       /* std::list<std::shared_ptr<CompilerNode>>::iterator iterator;
        std::list<std::shared_ptr<CompilerNode>> whileNodes;
        for (iterator = compilerNodes.begin(); iterator != compilerNodes.end(); ++iterator)
        {
            if (iterator->GetExpression() == "$whileLoop")
            {
                whileNodes = std::list<std::shared_ptr<CompilerNode>> {++iterator, compilerNodes.end()};
                ExecuteNodes(whileNodes);
                CallFunction(compilerNode);
            }
        }*/
        return nullptr;
    }
    else
    {
        return nullptr;
    }
}

#pragma endregion LoopOperations

#pragma region ConditionalStatements
std::shared_ptr<CompilerNode> VirtualMachine::ExecuteLessCondition(CompilerNode compilerNode)
{
	// Check if nodeparams are not empty
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(2, ParameterExceptionType::NoParameters);

	std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
	// Check if count of params is not right
	if (parameters.size() > 2)
		throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);

	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Check if expression is value
	if (param1->GetExpression() != "$value")
		param1 = CallFunction(*param1);
	if (param2->GetExpression() != "$value")
		param2 = CallFunction(*param2);

	// Set numbers / values
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	bool output = num1 < num2;

	// Set boolean to true if num1 < num2, else return false (inside the node)
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteGreaterCondition(CompilerNode compilerNode)
{
	// Check if nodeparams are not empty
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(2, ParameterExceptionType::NoParameters);

	std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
	// Check if count of params is not right
	if (parameters.size() > 2)
		throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);

	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Check if expression is value
	if (param1->GetExpression() != "$value")
		param1 = CallFunction(*param1);
	if (param2->GetExpression() != "$value")
		param2 = CallFunction(*param2);

	// Set numbers / values
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	bool output = num1 > num2;

	// Set boolean to true if num1 > num2, else return false (inside the node)
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteEqualCondition(CompilerNode compilerNode)
{
	// Check if nodeparams are not empty
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(2, ParameterExceptionType::NoParameters);

	std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
	// Check if count of params is not right
	if (parameters.size() > 2)
		throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);

	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Check if expression is value
	if (param1->GetExpression() != "$value")
		param1 = CallFunction(*param1);
	if (param2->GetExpression() != "$value")
		param2 = CallFunction(*param2);

	// Set numbers / values
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	bool output = num1 == num2;

	// Set boolean to true if num1 == num2, else return false (inside the node)
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteNotEqualCondition(CompilerNode compilerNode)
{
	// Check if nodeparams are not empty
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(2, ParameterExceptionType::NoParameters);

	std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
	// Check if count of params is not right
	if (parameters.size() > 2)
		throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);

	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Check if expression is value
	if (param1->GetExpression() != "$value")
		param1 = CallFunction(*param1);
	if (param2->GetExpression() != "$value")
		param2 = CallFunction(*param2);

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
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(2, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than two parameters
    if (parameters.size() > 2)
        throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    std::shared_ptr<CompilerNode> param2 = parameters.at(1);
    
    // Check if the parameters are a value or another function call
    // if function call, execute function
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    if (param2->GetExpression() != "$value")
        param2 = CallFunction(*param2);
    
    // Parse the parameters to a float for mathmatic operation
    float num1 = atof(param1->GetValue().c_str());
    float num2 = atof(param2->GetValue().c_str());
    float output = num1 + num2;
    
    // Create a new value compilernode to return
    return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteMinusOperation(CompilerNode compilerNode)
{
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(2, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than two parameters
    if (parameters.size() > 2)
        throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    std::shared_ptr<CompilerNode> param2 = parameters.at(1);
    
    // Check if the parameters are a value or another function call
    // if function call, execute function
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    if (param2->GetExpression() != "$value")
        param2 = CallFunction(*param2);
    
    // Parse the parameters to a float for mathmatic operation
    float num1 = atof(param1->GetValue().c_str());
    float num2 = atof(param2->GetValue().c_str());
	float output = num1 - num2;
    
    // Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteMultiplyOperation(CompilerNode compilerNode)
{
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(2, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than two parameters
    if (parameters.size() > 2)
        throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    std::shared_ptr<CompilerNode> param2 = parameters.at(1);
    
    // Check if the parameters are a value or another function call
    // if function call, execute function
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    if (param2->GetExpression() != "$value")
        param2 = CallFunction(*param2);
    
    // Parse the parameters to a float for mathmatic operation
    float num1 = atof(param1->GetValue().c_str());
    float num2 = atof(param2->GetValue().c_str());
	float output = num1 * num2;
    
    // Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteDivideOperation(CompilerNode compilerNode)
{
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(2, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than two parameters
    if (parameters.size() > 2)
        throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    std::shared_ptr<CompilerNode> param2 = parameters.at(1);
    
    // Check if the parameters are a value or another function call
    // if function call, execute function
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    if (param2->GetExpression() != "$value")
        param2 = CallFunction(*param2);
    
    // Parse the parameters to a float for mathmatic operation
    float num1 = atof(param1->GetValue().c_str());
    float num2 = atof(param2->GetValue().c_str());
	float output = num1 / num2;
    
    // Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteModuloOperation(CompilerNode compilerNode)
{
	if (compilerNode.GetNodeparameters().empty())
		throw ParameterException(2, ParameterExceptionType::NoParameters);

	// Get the Node parameters
	std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();

	// Check if there aren't more than two parameters
	if (parameters.size() > 2)
		throw ParameterException(2, parameters.size(), ParameterExceptionType::IncorrectParameters);

	std::shared_ptr<CompilerNode> param1 = parameters.at(0);
	std::shared_ptr<CompilerNode> param2 = parameters.at(1);

	// Check if the parameters are a value or another function call
	// if function call, execute function
	if (param1->GetExpression() != "$value")
		param1 = CallFunction(*param1);
	if (param2->GetExpression() != "$value")
		param2 = CallFunction(*param2);

	// Parse the parameters to a float for mathmatic operation
	float num1 = atof(param1->GetValue().c_str());
	float num2 = atof(param2->GetValue().c_str());
	float output = fmod(num1, num2);

	// Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteUniMinOperation(CompilerNode compilerNode)
{
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(1, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than two parameters
    if (parameters.size() > 1)
        throw ParameterException(1, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    
    // Check if the parameters are a value or another function call
    // if function call, execute function
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    
    // Parse the parameters to a float for mathmatic operation
    float num1 = atof(param1->GetValue().c_str());
    float output = num1 - 1;
    
    // Create a new value compilernode to return
    return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteUniPlusOperation(CompilerNode compilerNode)
{
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(1, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than two parameters
    if (parameters.size() > 1)
        throw ParameterException(1, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    
    // Check if the parameters are a value or another function call
    // if function call, execute function
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    
    // Parse the parameters to a float for mathmatic operation
    float num1 = atof(param1->GetValue().c_str());
    float output = num1++;
    
    // Create a new value compilernode to return
    return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

#pragma endregion SimpleMath

#pragma region ComplexMath

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteSinOperation(CompilerNode compilerNode)
{
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(1, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than one parameter
    if (parameters.size() > 1)
        throw ParameterException(1, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    
    // Check if the parameters are a value or another function call
    // if function call, execute function
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    
    // Parse the parameters to a float for mathmatic operation
    float num1 = atof(param1->GetValue().c_str());
    float output = std::sin(num1);
    
    // Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteCosOperation(CompilerNode compilerNode)
{
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(1, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than one parameter
    if (parameters.size() > 1)
        throw ParameterException(1, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    
    // Check if the parameters are a value or another function call
    // if function call, execute function
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    
    // Parse the parameters to a float for mathmatic operation
    float num1 = atof(param1->GetValue().c_str());
	float output = std::cos(num1);
    
    // Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

std::shared_ptr<CompilerNode> VirtualMachine::ExecuteTanOperation(CompilerNode compilerNode)
{
    if (compilerNode.GetNodeparameters().empty())
        throw ParameterException(1, ParameterExceptionType::NoParameters);
    
    // Get the Node parameters
    std::vector<std::shared_ptr<CompilerNode>> parameters = compilerNode.GetNodeparameters();
    
    // Check if there aren't more than one parameter
    if (parameters.size() > 1)
        throw ParameterException(1, parameters.size(), ParameterExceptionType::IncorrectParameters);
    
    std::shared_ptr<CompilerNode> param1 = parameters.at(0);
    
    // Check if the parameters are a value or another function call
    // if function call, execute function
    if (param1->GetExpression() != "$value")
        param1 = CallFunction(*param1);
    
    // Parse the parameters to a float for mathmatic operation
    float num1 = atof(param1->GetValue().c_str());
	float output = std::tan(num1);
    
    // Create a new value compilernode to return
	return std::make_shared<CompilerNode>(CompilerNode("$value", std::to_string(output), false));
}

#pragma endregion ComplexMath




