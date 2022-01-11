// Gideon Vos Dec 2021

#include <iostream>
#include <string>
#include <regex>
#include <list>
#include <chrono>
#include <assert.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include <sqltypes.h>

using std::cout; 
using std::endl; 
using std::string;

using namespace std;

enum TokenType {Method, Numeric, Constant, Comment, Identifier, Plus, Minus, Divide, Multiply, Equal, NotEqual, LessThan, 
				Not, And, Or, GreaterThan, LessThanOrEqualTo, GreaterThanOrEqualTo, Power, Index, Formula, OuterProduct, 
				KroneckerProduct, Assign, CustomOperator, Sequence, Modulus, Empty, Percentage,
				LeftParenthesis, RightParenthesis, LeftBracket, RightBracket, LeftCurly, RightCurly, 
				Semicolon, Illegal, EndOfFile, StartOfFile, EndOfLine, Nan, Inf, NA, NAInt, NAReal, NAComplex, NAChar,
				Comma, Colon, Null, If, Else, Repeat, While, Function, For, In, Next, Break, True, False, Return,
				Vector, Package, Pipe, FunctionDefinition};

class Token
{
		string 						text					= "";
		TokenType 					type					= TokenType::StartOfFile;
		int							lineNumber				= 1;
		
	public:
		string Text(void) {return text;}
		int LineNumber(void) {return lineNumber;}
		TokenType Type(void) {return type;}
		Token (string newText, TokenType newType, int newLineNumber) {text = newText; type = newType; lineNumber = newLineNumber;};
		Token (string newText, TokenType newType) {text = newText; type = newType;};
};

class LexicalAnalyzer
{
        int							position				= 0;
	    string						source					= "";
	    const int					space					= 32;
		const int					keywordDoubleQuote		= 34;
	    const int					keywordSingleQuote		= 39;
	    const int					keywordComment			= 35;
	    const int					newline					= 10;
	    const string				keywordSemicolon		= ";";
	    const string				keywordLeftParenthesis	= "(";
	    const string				keywordRightParenthesis	= ")";
	    const string				keywordLeftBracket		= "[";
	    const string				keywordRightBracket		= "]";
	    const string				keywordLeftCurly		= "{";
	    const string				keywordRightCurly		= "}";
	    const string				keywordComma			= ",";
	    const string				keywordSequence			= ":";
		const string				keywordIndex			= "$";
		const string				keywordFormula			= "~";
	    const string				keywordPlus				= "+";
	    const string				keywordMinus			= "-";
		const string				keywordPercentage		= "%";
	    const string				keywordMultiply			= "*";
	    const string				keywordDivide			= "/";
		const string				keywordPower			= "^";
	    const string				keywordEqual			= "=";
	    const string				keywordLessThan			= "<";
	    const string				keywordGreaterThan		= ">";
		const string				keywordAssign1			= "=";
		const string				keywordNot				= "!";
		const string				keywordAnd				= "&";
		const string				keywordOr				= "|";
	    const string				keywordIf				= "if";
		const string				keywordElse				= "else";
		const string				keywordRepeat			= "repeat";
		const string				keywordWhile			= "while";
		const string				keywordFunction			= "function";
		const string				keywordFor				= "for";
		const string				keywordIn				= "in";
		const string				keywordNext				= "next";
		const string				keywordBreak			= "break";
	    const string				keywordNA				= "NA";
	    const string				keywordNAint			= "NA_integer_";
	    const string				keywordNAreal			= "NA_real_";
	    const string				keywordNAcomp			= "NA_complex_";
	    const string				keywordNAchar			= "NA_character_";
	    const string				keywordTrue				= "TRUE";
	    const string				keywordFalse			= "FALSE";
	    const string				keywordInf				= "Inf";
	    const string				keywordNan				= "NaN";
        const string    			keywordReturn           = "return";

		const int					identifier[128]			= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //16
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0, //32
												  			   1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, //48
												  			   0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, //64
												  			   1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1, //80
												  			   0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, //96
												  			   1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0}; //112

		const int					number[128]				= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0
												  		   	   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //16
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0, //32
												  			   1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, //48
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //64
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //80
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //96
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //112

		const int					whitespace[128]			= {0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0, //0
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //16
												  			   1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //32
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //48
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //64
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //80
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //96
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};//112

		const int					breakchar[128]			= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //16
												  			   1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1, //32
												  			   0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0, //48
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //64
												  			   0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0, //80
												  			   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //96
												  			   0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0};//112

	 	TokenType isKeyword(string word);
		bool isNumeric(string word);
		bool isIdentifier(string word);

    public:
		 
        LexicalAnalyzer (string newSource) {source = newSource;};
        string Source(void) {return source;}
        int Position(void) {return position;}
		Token NextToken(void);
};

TokenType LexicalAnalyzer::isKeyword(string word)
{
	if (word.compare(keywordSemicolon) == 0) 		{return TokenType::Semicolon;}
	if (word.compare(keywordLeftParenthesis) == 0) 	{return TokenType::LeftParenthesis;}
	if (word.compare(keywordRightParenthesis) == 0) {return TokenType::RightParenthesis;}
	if (word.compare(keywordLeftBracket) == 0) 		{return TokenType::LeftBracket;}
	if (word.compare(keywordRightBracket) == 0) 	{return TokenType::RightBracket;}
	if (word.compare(keywordLeftCurly) == 0) 		{return TokenType::LeftCurly;}
	if (word.compare(keywordRightCurly) == 0) 		{return TokenType::RightCurly;}
	if (word.compare(keywordComma) == 0) 			{return TokenType::Comma;}
	if (word.compare(keywordSequence) == 0) 		{return TokenType::Sequence;}
	if (word.compare(keywordIndex) == 0) 			{return TokenType::Index;}
	if (word.compare(keywordFormula) == 0) 			{return TokenType::Formula;}
	if (word.compare(keywordPlus) == 0) 			{return TokenType::Plus;}
	if (word.compare(keywordMinus) == 0) 			{return TokenType::Minus;}
	if (word.compare(keywordPercentage) == 0) 		{return TokenType::Percentage;}
	if (word.compare(keywordMultiply) == 0) 		{return TokenType::Multiply;}
	if (word.compare(keywordDivide) == 0) 			{return TokenType::Divide;}
	if (word.compare(keywordPower) == 0) 			{return TokenType::Power;}
	if (word.compare(keywordEqual) == 0) 			{return TokenType::Equal;}
	if (word.compare(keywordLessThan) == 0) 		{return TokenType::LessThan;}
	if (word.compare(keywordGreaterThan) == 0) 		{return TokenType::GreaterThan;}
	if (word.compare(keywordAssign1) == 0) 			{return TokenType::Assign;}
	if (word.compare(keywordNot) == 0) 				{return TokenType::Not;}
	if (word.compare(keywordAnd) == 0) 				{return TokenType::And;}
	if (word.compare(keywordOr) == 0) 				{return TokenType::Or;}
	if (word.compare(keywordIf) == 0) 				{return TokenType::If;}
	if (word.compare(keywordElse) == 0) 			{return TokenType::Else;}
	if (word.compare(keywordRepeat) == 0) 			{return TokenType::Repeat;}
	if (word.compare(keywordWhile) == 0) 			{return TokenType::While;}
	if (word.compare(keywordFunction) == 0) 		{return TokenType::Function;}
	if (word.compare(keywordFor) == 0) 				{return TokenType::For;}
	if (word.compare(keywordIn) == 0) 				{return TokenType::In;}
	if (word.compare(keywordNext) == 0) 			{return TokenType::Next;}
	if (word.compare(keywordBreak) == 0) 			{return TokenType::Break;}
	if (word.compare(keywordNA) == 0) 				{return TokenType::NA;}
	if (word.compare(keywordNAint) == 0) 			{return TokenType::NAInt;}
	if (word.compare(keywordNAreal) == 0) 			{return TokenType::NAReal;}
	if (word.compare(keywordNAcomp) == 0) 			{return TokenType::NAComplex;}
	if (word.compare(keywordNAchar) == 0) 			{return TokenType::NAChar;}
	if (word.compare(keywordTrue) == 0) 			{return TokenType::True;}
	if (word.compare(keywordFalse) == 0) 			{return TokenType::False;}
	if (word.compare(keywordInf) == 0) 				{return TokenType::Inf;}
	if (word.compare(keywordNan) == 0) 				{return TokenType::Nan;}
	if (word.compare(keywordReturn) == 0) 			{return TokenType::Return;}

	return TokenType::Null;
}

bool LexicalAnalyzer::isNumeric(string word)
{
	return regex_match(word, regex(("^((-?(?:(?:\\d+|\\d*\\.\\d+)(?:[E|e][+|-]?\\d+)?)))$")));
}

bool LexicalAnalyzer::isIdentifier(string word)
{
	if ((word[0] == 46) || (word[word.length() - 1] == 46))
	{
		return false;
	}
	else
	{
		if (number[word[0]] == 1) return false;
		for (int i = 0; i < word.length(); i++)
		{
			if (identifier[word[i]] != 1) return false;
		}
	}

	return true;
}

Token LexicalAnalyzer::NextToken(void)
{
	if (source.length() == 0) {throw "LexicalAnalyzer::NextToken source length is zero.";}
	if (position >= source.length()) {return Token("", TokenType::EndOfFile);}

	int inConstant = false;
	string word = "";
	TokenType keywordType = TokenType::StartOfFile;

	while (position < source.length())
	{
		if ((source[position] == keywordSingleQuote) || (source[position] == keywordDoubleQuote))
		{
			if (inConstant != false)
			{
				if (source[position] == inConstant) // only return word if we hit the closing constant quote
				{
					word += source[position];
					position++;
					inConstant = false;
					return Token(word, TokenType::Constant);
				}
			}
			else
			{
				inConstant = source[position];
			}
		} 
		if (inConstant)
		{
			word += source[position];
		}
		else
		{
			if (source[position] == keywordComment)
			{
				if (word.length() > 0)
				{
					break;
				}
				else
				{
					while (position < source.length())
					{
						if (source[position] == newline) break;
						word += source[position];
						position++;
					}
					return Token(word, TokenType::Comment);
				}
			}
			if (whitespace[source[position]] == 1)
			{
				if (word.length() > 0)
				{
					break;
				}
				else
				{
					if (source[position] == newline)
					{
						position++;
						return Token("", TokenType::EndOfLine);
					}
				}
			}
			else
			{
				if (breakchar[source[position]] == 1)
				{
					if (word.length() > 0) break;
					if (source[position] != space)
					{
						word = "";
						word += source[position];
						position++;
						break;
					}
				}
				else
				{
					word += source[position];
				}				
			}
		}
		position++;
	}
	if (word.length() == 0) return Token("", TokenType::Empty);
	if (position >= source.length())
	{
		if (word.length() == 0) return Token("", TokenType::EndOfFile);
	}
	if (isNumeric(word)) {return Token(word, TokenType::Numeric);}
	TokenType type = isKeyword(word);
	if (type != TokenType::Null) {return Token(word, type);}
	if (isIdentifier(word)) {return Token(word, TokenType::Identifier);} // can be method, function name, variable at this stage.
	if (word[0] == 46) {return Token(word, TokenType::CustomOperator);} // .N or .I or .anythingelse
	return Token("", TokenType::Illegal);
}

class Parser
{
		string						source					= "";
		int							lineCount				= 0;
		int							commentCount			= 0;
		int							linesSec				= 0;
		int							index					= 0;		
		list<Token>::iterator 		iter;

		Token Forward(int steps);
		Token Backward(int steps);
		Token PeekAhead(int steps);	
		Token PeekBack(int steps);			

    public:
        string Source(void) {return source;}
		int CommentCount(void) {return commentCount;}
		int LineCount(void) {return lineCount;}
		int LinesSecond(void) {return linesSec;}
        list<Token> Parse(void);
		Parser (string newSource) {source = newSource;};
};

Token Parser::Forward(int steps)
{
	assert(steps > 0);
	int i = 0;
	Token token = *iter;
	if (token.Type() == TokenType::EndOfFile) {return token;};
	while (i < steps)
	{
		advance(iter, 1);
		token = *iter;
		if (token.Type() == TokenType::EndOfFile) {return token;};
		i++;
	}
	return token;
}

Token Parser::PeekAhead(int steps)
{
	assert(steps > 0);
	int i = 0;
	Token token = *iter;
	if (token.Type() == TokenType::EndOfFile) {return token;}; //make sure not already at EOF

	while (i < steps)
	{
		advance(iter, 1);
		token = *iter;
		i++;
		if (token.Type() == TokenType::EndOfFile) break;
	}
	while (i > 0)
	{
		advance(iter, -1);
		i--;
	}
	return token;
}

Token Parser::PeekBack(int steps)
{
	assert(steps > 0);
	int i = 0;
	Token token = *iter;

	while (i < steps)
	{

		advance(iter, -1);
		token = *iter;
		if (token.Type() == TokenType::EndOfFile) break;
		i++;
	}
	while (i > 0)
	{
		advance(iter, 1);
		i--;
	}
	return token;
}

list<Token> Parser::Parse(void)
{
	auto start = std::chrono::high_resolution_clock::now();
	lineCount = 0;
	commentCount = 0;
	linesSec = 0;
	int tokenCount = 0;
	list<Token> tokens;
	list<Token> tokensParsed;
	bool handled = false;
    LexicalAnalyzer lexer(source);
	Token token = Token("", TokenType::Null);

	while (token.Type() != TokenType::EndOfFile)
	{
		token = lexer.NextToken();
		tokens.push_back(token);
		tokenCount++;
	}
	iter = tokens.begin();
	for (index=0; index<tokenCount-1; index++)
	{
		handled = false;
		token = *iter;
		if (token.Type() == TokenType::EndOfFile) break;
		if (token.Type() == TokenType::EndOfLine) {lineCount++; Forward(1); continue;}
		if (token.Type() == TokenType::Comment) 
		{
			commentCount++; 
			Token newToken = Token(token.Text(), TokenType::Comment, lineCount+1);
			tokensParsed.push_back(newToken); 
			Forward(1); 
			continue;
		}

		// <-
		if (token.Type() == TokenType::LessThan)
		{
			token = PeekAhead(1);
			if (token.Type() == TokenType::Minus)
			{
				Token newToken = Token("<-", TokenType::Assign, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
				Forward(1);
			}
			else
			{
				token = *iter;
			}
		}
		// ->
		if (token.Type() == TokenType::Minus)
		{
			token = PeekAhead(1);
			if (token.Type() == TokenType::GreaterThan)
			{
				Token newToken = Token("->", TokenType::Assign, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
				Forward(1);
			}
			else
			{
				token = *iter;
			}
		}
		// !=
		if (token.Type() == TokenType::Not)
		{
			token = PeekAhead(1);
			if (token.Type() == TokenType::Equal)
			{
				Token newToken = Token("!=", TokenType::NotEqual, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
				Forward(1);
			}
			else
			{
				token = *iter;
			}
		}
		// <=
		if (token.Type() == TokenType::LessThan)
		{
			token = PeekAhead(1);
			if (token.Type() == TokenType::Equal)
			{
				Token newToken = Token("<=", TokenType::LessThanOrEqualTo, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
				Forward(1);
			}
			else
			{
				token = *iter;
			}
		}
		// >=
		if (token.Type() == TokenType::GreaterThan)
		{
			token = PeekAhead(1);
			if (token.Type() == TokenType::Equal)
			{
				Token newToken = Token(">=", TokenType::GreaterThanOrEqualTo, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
				Forward(1);
			}
			else
			{
				token = *iter;
			}
		}
		// ==
		if (token.Type() == TokenType::Equal)
		{
			token = PeekAhead(1);
			if (token.Type() == TokenType::Equal)
			{
				Token newToken = Token("==", TokenType::Equal, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
				Forward(1);
			}
			else
			{
				token = *iter;
			}
		}
		// %%
		if (token.Type() == TokenType::Percentage)
		{
			token = PeekAhead(1);
			if (token.Type() == TokenType::Percentage)
			{
				Token newToken = Token("%%", TokenType::Modulus, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
				Forward(1);
			}
			else
			{
				token = *iter;
			}
		}
		// &&
		if (token.Type() == TokenType::And)
		{
			token = PeekAhead(1);
			if (token.Type() == TokenType::And)
			{
				Token newToken = Token("&&", TokenType::And, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
				Forward(1);
			}
			else
			{
				token = *iter;
			}
		}
		// ||
		if (token.Type() == TokenType::Or)
		{
			token = PeekAhead(1);
			if (token.Type() == TokenType::Or)
			{
				Token newToken = Token("||", TokenType::Or, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
				Forward(1);
			}
			else
			{
				token = *iter;
			}
		}
		// functions
		if (token.Type() == TokenType::Identifier)
		{
			token = PeekAhead(3);
			if (token.Type() == TokenType::Function)
			{
				token = *iter;
				Token newToken = Token(token.Text(), TokenType::FunctionDefinition, lineCount+1);
				tokensParsed.push_back(newToken);
				handled = true;
			}
			else
			{
				token = *iter;
			}
		}

		// methods
		if (token.Type() == TokenType::Identifier)
		{
			// if next part is parenthesis:
			token = PeekAhead(1);
			if (token.Type() == TokenType::LeftParenthesis)
			{
				token = *iter;
				if (token.Text() == "c")
				{
					Token newToken = Token("c", TokenType::Vector, lineCount+1);
					tokensParsed.push_back(newToken);
				}
				else
				{
					Token newToken = Token(token.Text(), TokenType::Method, lineCount+1);
					tokensParsed.push_back(newToken);
				}
				handled = true;
			}
			else
			{
				if (token.Type() == TokenType::Index)
				{
					token = PeekAhead(2);
					if (token.Type() == TokenType::Identifier)
					{
						string secondPart = token.Text();
						token = *iter;
						Token newToken = Token(token.Text() + "$" + secondPart, TokenType::Identifier, lineCount+1);
						tokensParsed.push_back(newToken);
						Forward(2);
						handled = true;
					}
					else
					{
						token = *iter;
					}
				}
				else
				{
					if (token.Type() == TokenType::Sequence)
					{
						// next must also be sequence
						token = PeekAhead(1);
						if (token.Type() == TokenType::Sequence)
						{
							token = *iter;
							Token newToken = Token(token.Text(), TokenType::Package, lineCount+1);
							tokensParsed.push_back(newToken);
							handled = true;
						}
					}
					else
					{
						token = PeekBack(1);
						if (token.Type() == TokenType::LeftParenthesis)
						{
							token = PeekBack(2);
							if ((token.Type() == TokenType::Identifier) & ((token.Text() == "library") | (token.Text() == "require")))
							{
								token = *iter;
								Token newToken = Token(token.Text(), TokenType::Package, lineCount+1);
								tokensParsed.push_back(newToken);
								handled = true;
							}
						}
						else
						{
							token = *iter;
						}
					}
				}
			}
		}

		// %x% %o% %in% %*% %/% %anything%
		if ((!handled) && (token.Type() == TokenType::Percentage))
		{
			token = PeekAhead(2);
			if (token.Type() == TokenType::Percentage)
			{
				token = PeekAhead(1);
				if (token.Text() == "<") 
				{
					Token newToken = Token("%<%", TokenType::Pipe, lineCount+1);
					tokensParsed.push_back(newToken);
					Forward(1);
					handled = true;
				}
				if (token.Text() == ">") 
				{
					Token newToken = Token("%>%", TokenType::Pipe, lineCount+1);
					tokensParsed.push_back(newToken);
					Forward(1);
					handled = true;
				}
				if (token.Text() == "x") 
				{
					Token newToken = Token("%x%", TokenType::KroneckerProduct, lineCount+1);
					tokensParsed.push_back(newToken);
					Forward(1);
					handled = true;
				}
				if (token.Text() == "o") 
				{
					Token newToken = Token("%o%", TokenType::OuterProduct, lineCount+1);
					tokensParsed.push_back(newToken);
					Forward(1);
					handled = true;
				}				
				if (token.Text() == "in") 
				{
					Token newToken = Token("%in%", TokenType::In, lineCount+1);
					tokensParsed.push_back(newToken);
					Forward(1);
					handled = true;
				}				
				if (token.Text() == "*") 
				{
					Token newToken = Token("%*%", TokenType::Multiply, lineCount+1);
					tokensParsed.push_back(newToken);
					Forward(1);
					handled = true;
				}
				if (token.Text() == "/") 
				{
					Token newToken = Token("%/%", TokenType::Divide, lineCount+1);
					tokensParsed.push_back(newToken);
					Forward(1);
					handled = true;
				}
				if (!handled)
				{
					Token newToken = Token("%" + token.Text() + "%", TokenType::CustomOperator, lineCount+1);
					tokensParsed.push_back(newToken);
					Forward(1);
					handled = true;
				}
			}
		}		
		if (!handled) 
		{
			token = *iter;
			if (token.Type() == TokenType::EndOfLine)
			{
				Token newToken = Token(token.Text(), token.Type(), lineCount+1);
				tokensParsed.push_back(newToken);
			}
			else
			{
				Token newToken = Token(token.Text(), token.Type(), lineCount+1);
				tokensParsed.push_back(newToken);
			}
		}
		Forward(1);
	}
	tokens.clear();
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	if (lineCount > 0)
	{
		linesSec = (int)(((float)lineCount / (float)duration.count()) * 1000);
	}
	return tokensParsed;
}

string TokenToString(TokenType type)
{
	if (type == TokenType::And) return "And";
	if (type == TokenType::Assign) return "Assign";
	if (type == TokenType::Break) return "Break";
	if (type == TokenType::Colon) return "Colon";
	if (type == TokenType::Comma) return "Comma";
	if (type == TokenType::Comment) return "Comment";
	if (type == TokenType::Constant) return "Constant";
	if (type == TokenType::CustomOperator) return "CustomOperator";
	if (type == TokenType::Divide) return "Divide";
	if (type == TokenType::Else) return "Else";
	if (type == TokenType::Empty) return "Empty";
	if (type == TokenType::EndOfFile) return "EndOfFile";
	if (type == TokenType::EndOfLine) return "EndOfLine";
	if (type == TokenType::Equal) return "Equal";
	if (type == TokenType::False) return "False";
	if (type == TokenType::For) return "For";
	if (type == TokenType::Formula) return "Formula";
	if (type == TokenType::Function) return "Function";
	if (type == TokenType::FunctionDefinition) return "FunctionDefinition";
	if (type == TokenType::GreaterThan) return "GreaterThan";
	if (type == TokenType::GreaterThanOrEqualTo) return "GreaterThanOrEqualTo";
	if (type == TokenType::Identifier) return "Identifier";
	if (type == TokenType::If) return "If";
	if (type == TokenType::Illegal) return "Illegal";
	if (type == TokenType::In) return "In";
	if (type == TokenType::Index) return "Index";
	if (type == TokenType::Inf) return "Inf";
	if (type == TokenType::KroneckerProduct) return "KroneckerProduct";
	if (type == TokenType::LeftBracket) return "LeftBracket";
	if (type == TokenType::LeftCurly) return "LeftCurly";
	if (type == TokenType::LeftParenthesis) return "LeftParenthesis";
	if (type == TokenType::LessThan) return "LessThan";
	if (type == TokenType::LessThanOrEqualTo) return "LessThanOrEqualTo";
	if (type == TokenType::Method) return "Method";
	if (type == TokenType::Minus) return "Minus";
	if (type == TokenType::Modulus) return "Modulus";
	if (type == TokenType::Multiply) return "Multiply";
	if (type == TokenType::NA) return "NA";
	if (type == TokenType::NAChar) return "NAChar";
	if (type == TokenType::NAComplex) return "NAComplex";
	if (type == TokenType::NAInt) return "NAInt";
	if (type == TokenType::Nan) return "Nan";
	if (type == TokenType::NAReal) return "NAReal";
	if (type == TokenType::Next) return "Next";
	if (type == TokenType::Not) return "Not";
	if (type == TokenType::NotEqual) return "NotEqual";
	if (type == TokenType::Null) return "Null";
	if (type == TokenType::Numeric) return "Numeric";
	if (type == TokenType::Or) return "Or";
	if (type == TokenType::OuterProduct) return "OuterProduct";
	if (type == TokenType::Package) return "Package";
	if (type == TokenType::Percentage) return "Percentage";
	if (type == TokenType::Pipe) return "Pipe";
	if (type == TokenType::Plus) return "Plus";
	if (type == TokenType::Power) return "Power";
	if (type == TokenType::Repeat) return "Repeat";
	if (type == TokenType::Return) return "Return";
	if (type == TokenType::RightBracket) return "RightBracket";
	if (type == TokenType::RightCurly) return "RightCurly";
	if (type == TokenType::RightParenthesis) return "RightParenthesis";
	if (type == TokenType::Semicolon) return "Semicolon";
	if (type == TokenType::Sequence) return "Sequence";
	if (type == TokenType::StartOfFile) return "StartOfFile";
	if (type == TokenType::True) return "True";
	if (type == TokenType::Vector) return "Vector";
	if (type == TokenType::While) return "While";
	return "";
}

string ListToJSON(list<Token> tokens)
{
	string result = "{\"tokens\": [";
  	list<Token>::iterator iter = tokens.begin();
	for (int count=0; count < tokens.size(); count++)
	{
		Token token = *iter;
		result += "{\"token\":\"" + token.Text() + "\",\"type\":\"" + TokenToString(token.Type()) + "\",\"ln\":" + to_string(token.LineNumber()) + "}";
		if (count < tokens.size()-1) result += ",";
		advance(iter, 1);
	}
	result += "]}";
	return result;
}

list<Token> ParseRInternal(string script)
{
	Parser parser(script);
	list<Token> tokens = parser.Parse();
	return tokens;
}

void StoreDB(string connectionString, string companyName, string scriptName, string scriptVersion, string json)
{
	SQLHENV env;
	SQLHDBC dbc;
	SQLHSTMT stmt;
	SQLRETURN retcode;

	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) 
	{
		retcode = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) 
		{
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) 
			{
				retcode = SQLDriverConnect(dbc, NULL, (SQLCHAR*)connectionString.c_str(), SQL_NTS,NULL, 0, NULL, SQL_DRIVER_COMPLETE);
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
				{
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						SQLCHAR* szCompanyName = (SQLCHAR *)companyName.c_str();
						SQLCHAR* szScriptName = (SQLCHAR *)scriptName.c_str();
						SQLCHAR* szScriptVersion = (SQLCHAR *)scriptVersion.c_str();
						SQLCHAR* szJson = (SQLCHAR *)json.c_str();
						SQLLEN lenCompanyName = companyName.length();
						SQLLEN lenScriptName = scriptName.length();
						SQLLEN lenScriptVersion = scriptVersion.length();
						SQLLEN lenJson = json.length();
						SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 255, 0, szCompanyName, 255, NULL);
						SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 255, 0, szScriptName, 255, NULL);
						SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 10, 0, szScriptVersion, 10, NULL);

						SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, lenJson, 0, szJson, lenJson, &lenJson);
						SQLPrepare(stmt, (SQLCHAR*)"INSERT INTO Scripts(CompanyName, ScriptName, ScriptVersion, ScriptJSON) VALUES (?,?,?,?)", SQL_NTS);
						retcode =  SQLExecute(stmt);
						SQLFreeHandle(SQL_HANDLE_STMT, stmt);  
					}
				}
				SQLDisconnect(dbc); 
			}
			SQLFreeHandle(SQL_HANDLE_STMT, dbc);
		}
		SQLFreeHandle(SQL_HANDLE_ENV, env);  
	}
}

// pipeline:
// from web app, upload script, tokenize via python, store in db
// from R, tokenize, pushed directly to db

// use for Python
extern "C" {
    const char * ParseRPython(char * script)
    {
		list<Token> tokens = ParseRInternal(script);
		static string result = ListToJSON(tokens);
		return result.c_str();
    }
}

// use for R
extern "C" {
    void ParseR(char ** script, char ** companyName, char ** scriptName, char ** version, char ** connectionString)
    {
		list<Token> tokens = ParseRInternal(script[0]);
		static string json = ListToJSON(tokens);
		StoreDB(connectionString[0], companyName[0], scriptName[0], version[0], json);
    }
}

// to create shared lib callable from python/R compile with:
// g++ -fPIC -shared -o ParseR.so ParseR.cpp

// or uncomment below to run and to debug:
/*int main()
{
	string file_contents = "library(dplyr)";
	list<Token> tokens = ParseRInternal(file_contents);
	string result = ListToJSON(tokens);
	//Parser parser(file_contents);
	//list<Token> tokens = parser.Parse();
	StoreDB("Driver={ODBC Driver 17 for SQL Server};Server=tcp:rmetrics.database.windows.net,1433;Database=rmetrics;Uid=abcd;Pwd=<your password>;Encrypt=yes;TrustServerCertificate=no;Connection Timeout=30;",
	 "ABC Inc", "script.R", "0.1", result);
	cout << endl << "HTML Generation Completed." << endl;
}*/

// calling from R:
/*dyn.load("ParseR.so")

.C("ParseR",  as.character("library(dplyr)"), 
                         as.character("ACME INC"),
                         as.character("Script.R"),
                         as.character("1.00"),
                         as.character("Driver={ODBC Driver 17 for SQL Server};Server=tcp:rmetrics.database.windows.net,1433;Database=abcd;Uid=rmetrics;Pwd=<your password>;Encrypt=yes;TrustServerCertificate=no;Connection Timeout=30;")
                         )
*/