//
//  TokenType.h
//  part of tokenizer
//  Contains all the possible token types
//
#pragma once

enum class TokenType : int {
    KeyIdentifier,
    Function,
    Identifier,
    If,
    Else,
    ElseIf,
    ForLoop,
    While,
    Return,
    Float,
    Special,
    EOL,
	And,
	Or,
	Seperator,
    OperatorPlus,
    OperatorMinus,
    OperatorDivide,
    OperatorRaised,
    OperatorMultiply,
    UniOperatorPlus,
    UniOperatorMinus,
    GreaterThan,
    LowerThan,
    GreaterOrEqThan,
    LowerOrEqThan,
    Equals,
    OpenMethod,
    CloseMethod,
    OpenCurlyBracket,
    CloseCurlyBracket,
    OpenBracket,
    CloseBracket,
    Comparator,
	Cosine,
	Sine,
	Tangent,
    None
};