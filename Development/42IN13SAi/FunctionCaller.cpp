//
//  FunctionCaller.cpp
//  mac_42IN13SAI
//
//  Copyright (c) 2014 42IN13SAI. All rights reserved.
//

#include "FunctionCaller.h"

FunctionCaller::FunctionCaller(VirtualMachine* virtualMachine)
{
    this->virtualMachine = virtualMachine;
}

CompilerNode FunctionCaller::Call(const std::string& functionName, CompilerNode compilerNode)
{
    MFP functionPointer = functions[functionName];
    return (virtualMachine->*functionPointer)(compilerNode);
}

void FunctionCaller::fillFunctionMap()
{
    //functions.insert(functionMap::value_type("$add", &VirtualMachine::Add));
}

FunctionCaller::~FunctionCaller()
{
    delete virtualMachine;
    virtualMachine = nullptr;
}