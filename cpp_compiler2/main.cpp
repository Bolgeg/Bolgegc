#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <exception>
#include <algorithm>
#include <random>
#include <fstream>
#include <iostream>
#include <cmath>
#include <filesystem>

using namespace std;

string fileToString(const string& filepath)
{
	string str(filesystem::file_size(filepath),0);
	ifstream file(filepath,ios::binary);
	file.read(str.data(),str.size());
	return str;
}

void stringToFile(const string& str,const string& filepath)
{
	ofstream file(filepath,ios::binary);
	file.write(str.data(),str.size());
}



class Error
{
	public:
	
	string message;
	
	Error(){}
	Error(const string& _message)
	{
		message=_message;
	}
};



template <class T>
class NamedVector
{
	private:
	
	vector<T> elements;
	map<string,size_t> nameToIndex;
	
	public:
	
	bool add(const T& newElement,bool addName=true)
	{
		if(addName)
		{
			if(contains(newElement.name)) return false;
		}
		
		elements.push_back(newElement);
		if(addName) nameToIndex[newElement.name]=elements.size()-1;
		
		return true;
	}
	
	bool contains(const string& name) const
	{
		try
		{
			nameToIndex.at(name);
			return true;
		}
		catch(...)
		{
			return false;
		}
	}
	
	int getIndexOf(const string& name)
	{
		try
		{
			return nameToIndex.at(name);
		}
		catch(...)
		{
			throw nameMapAtError(name);
		}
	}
	
	T& operator [](size_t index)
	{
		try
		{
			return elements.at(index);
		}
		catch(...)
		{
			string mapContent;
			for(int i=0;i<elements.size();i++)
			{
				mapContent+=string("- ")+elements[i].name+"\n";
			}
			throw string("FATAL ERROR - Exception on NamedVector's map::at("+to_string(index)+"). NamedVector content:\n"+mapContent);
		}
	}
	private:
		string nameMapAtError(const string& name)
		{
			string mapContent;
			for(int i=0;i<elements.size();i++)
			{
				mapContent+=string("- ")+elements[i].name+"\n";
			}
			return string("FATAL ERROR - Exception on NamedVector's map::at(\""+name+"\"). NamedVector content:\n"+mapContent);
		}
	public:
	T& operator [](const string& name)
	{
		try
		{
			return elements[nameToIndex.at(name)];
		}
		catch(...)
		{
			throw nameMapAtError(name);
		}
	}
	
	size_t size() const
	{
		return elements.size();
	}
};



int64_t regToInteger(const string& reg)
{
	if(reg=="rax") return 0;
	else if(reg=="rbx") return 1;
	else if(reg=="rcx") return 2;
	else if(reg=="rdx") return 3;
	else if(reg=="rsi") return 4;
	else if(reg=="rdi") return 5;
	else if(reg=="rsp") return 6;
	else if(reg=="rbp") return 7;
	else if(reg=="r8") return 8;
	else if(reg=="r9") return 9;
	else if(reg=="r10") return 10;
	else if(reg=="r11") return 11;
	else if(reg=="r12") return 12;
	else if(reg=="r13") return 13;
	else if(reg=="r14") return 14;
	else if(reg=="r15") return 15;
	else return -1;
}

string integerToReg(int64_t i)
{
	if(i==0) return "rax";
	else if(i==1) return "rbx";
	else if(i==2) return "rcx";
	else if(i==3) return "rdx";
	else if(i==4) return "rsi";
	else if(i==5) return "rdi";
	else if(i==6) return "rsp";
	else if(i==7) return "rbp";
	else if(i==8) return "r8";
	else if(i==9) return "r9";
	else if(i==10) return "r10";
	else if(i==11) return "r11";
	else if(i==12) return "r12";
	else if(i==13) return "r13";
	else if(i==14) return "r14";
	else if(i==15) return "r15";
	else return "";
}

class IasmTokenizer
{
	public:
	vector<string> tokenize(const string& input)
	{
		vector<string> output;
		
		size_t p=0;
		
		bool insideToken=false;
		bool insideStringToken=false;
		
		while(true)
		{
			if(p>=input.size()) break;
			
			uint8_t c=input[p];
			
			if(insideToken)
			{
				if(insideStringToken)
				{
					if(c=='\"')
					{
						output.back().push_back(c);
						insideToken=false;
					}
					else
					{
						output.back().push_back(c);
					}
				}
				else
				{
					if(isCharacterOfWordToken(c))
					{
						output.back().push_back(c);
					}
					else if(c=='\"')
					{
						output.push_back(string(1,c));
						insideStringToken=true;
					}
					else if(isSpaceCharacter(c))
					{
						insideToken=false;
					}
					else
					{
						output.push_back(string(1,c));
						insideToken=false;
					}
				}
			}
			else
			{
				if(isCharacterOfWordToken(c))
				{
					output.push_back(string(1,c));
					insideToken=true;
					insideStringToken=false;
				}
				else if(c=='\"')
				{
					output.push_back(string(1,c));
					insideToken=true;
					insideStringToken=true;
				}
				else if(isSpaceCharacter(c)){}
				else
				{
					output.push_back(string(1,c));
				}
			}
			
			p++;
		}
		
		return output;
	}
	bool isCharacterOfWordToken(uint8_t c)
	{
		return c>='a' && c<='z' || c>='A' && c<='Z' || c>='0' && c<='9' || c=='_';
	}
	bool isSpaceCharacter(uint8_t c)
	{
		return c==' ' || c=='\t' || c=='\n' || c=='\r';
	}
};

enum class IasmArgumentType
{
	none,
	empty,
	var,
	globalvar,
	imm,
	reg,
	label,
	str
};

class IasmArgument
{
	public:
	
	IasmArgumentType type=IasmArgumentType::none;
	int dereferenceLevels=0;
	int64_t dereferenceOffset=0;
	
	int64_t value=0;
	string strValue;
	
	IasmArgument(){}
	explicit IasmArgument(const vector<string>& tokens)
	{
		size_t t=0;
		if(tokens.at(t)=="empty")
		{
			type=IasmArgumentType::empty;
			return;
		}
		else if(tokens.at(t)[0]=='\"')
		{
			type=IasmArgumentType::str;
			strValue=string(tokens.at(t).begin()+1,tokens.at(t).end()-1);
			return;
		}
		
		while(tokens.at(t)=="[")
		{
			dereferenceLevels++;
			t++;
		}
		
		if(tokens.at(t)=="var") type=IasmArgumentType::var;
		else if(tokens.at(t)=="globalvar") type=IasmArgumentType::globalvar;
		else if(tokens.at(t)=="imm") type=IasmArgumentType::imm;
		else if(tokens.at(t)=="reg") type=IasmArgumentType::reg;
		else if(tokens.at(t)=="label") type=IasmArgumentType::label;
		else
		{
			type=IasmArgumentType::none;
			t--;
		}
		t++;
		if(tokens.at(t)!="(") throw Error();
		t++;
		
		if(type==IasmArgumentType::reg)
		{
			value=regToInteger(tokens.at(t));
			if(value==-1) throw Error();
			t++;
		}
		else
		{
			bool negative=false;
			if(tokens.at(t)=="-")
			{
				negative=true;
				t++;
			}
			
			try
			{
				value=int64_t(stoull(tokens.at(t)));
			}
			catch(...)
			{
				throw Error();
			}
			t++;
			
			if(negative) value=-value;
		}
		
		if(tokens.at(t)!=")") throw Error();
		t++;
		
		if(dereferenceLevels>0)
		{
			if(tokens.at(t)!="]")
			{
				bool negative=false;
				if(tokens.at(t)=="+") t++;
				else if(tokens.at(t)=="-")
				{
					t++;
					negative=true;
				}
				else throw Error();
				
				try
				{
					dereferenceOffset=int64_t(stoull(tokens.at(t)));
				}
				catch(...)
				{
					throw Error();
				}
				t++;
				
				if(negative) dereferenceOffset=-dereferenceOffset;
			}
			for(int i=0;i<dereferenceLevels;i++)
			{
				if(tokens.at(t)!="]") throw Error();
				t++;
			}
		}
	}
	string toString()
	{
		if(type==IasmArgumentType::empty) return "empty";
		else if(type==IasmArgumentType::str) return string("\"")+strValue+"\"";
		
		string str;
		
		for(int i=0;i<dereferenceLevels;i++) str+="[";
		
		if(type==IasmArgumentType::var) str+="var";
		else if(type==IasmArgumentType::globalvar) str+="globalvar";
		else if(type==IasmArgumentType::imm) str+="imm";
		else if(type==IasmArgumentType::reg) str+="reg";
		else if(type==IasmArgumentType::label) str+="label";
		
		str+="(";
		if(type==IasmArgumentType::reg) str+=integerToReg(value);
		else str+=to_string(value);
		str+=")";
		
		if(dereferenceLevels>0)
		{
			if(dereferenceOffset!=0) str+=string("+")+to_string(dereferenceOffset);
			for(int i=0;i<dereferenceLevels;i++) str+="]";
		}
		
		return str;
	}
};

enum class IasmLineType
{
	none,
	instruction,
	label,
	blockStart,
	blockEnd
};

class IasmInstruction
{
	public:
	
	string name;
	vector<IasmArgument> outputArguments;
	vector<IasmArgument> inputArguments;
	
	IasmInstruction(){}
	explicit IasmInstruction(const vector<string>& tokens)
	{
		size_t t=0;
		name=tokens.at(t);
		t++;
		
		while(true)
		{
			if(tokens.at(t)==";") break;
			
			size_t i=t;
			for(;;i++)
			{
				if(tokens.at(i)=="," || tokens.at(i)==";") break;
			}
			
			vector<string> argTokens(tokens.begin()+t,tokens.begin()+i);
			
			outputArguments.emplace_back(argTokens);
			
			t=i;
			if(tokens.at(t)==",") t++;
		}
		
		if(tokens.at(t)!=";") throw Error();
		t++;
		
		if(t<tokens.size())
		{
			while(true)
			{
				size_t i=t;
				for(;;i++)
				{
					if(i>=tokens.size()) break;
					if(tokens.at(i)==",") break;
				}
				
				vector<string> argTokens(tokens.begin()+t,tokens.begin()+i);
				
				inputArguments.emplace_back(argTokens);
				
				t=i;
				if(t>=tokens.size()) break;
				if(tokens.at(t)==",") t++;
			}
		}
	}
	string toString()
	{
		string str=name+" ";
		for(int i=0;i<outputArguments.size();i++)
		{
			if(i>0) str+=",";
			str+=outputArguments[i].toString();
		}
		str+=";";
		for(int i=0;i<inputArguments.size();i++)
		{
			if(i>0) str+=",";
			str+=inputArguments[i].toString();
		}
		return str;
	}
};

class IasmLabel
{
	public:
	
	uint64_t index=0;
	
	IasmLabel(){}
	explicit IasmLabel(const vector<string>& tokens)
	{
		size_t t=0;
		if(tokens.at(t)!="L") throw Error();
		t++;
		if(tokens.at(t)!="(") throw Error();
		t++;
		
		try
		{
			index=stoull(tokens.at(t));
			t++;
		}
		catch(...)
		{
			throw Error();
		}
		
		if(tokens.at(t)!=")") throw Error();
		t++;
		if(tokens.at(t)!=":") throw Error();
		t++;
	}
	string toString()
	{
		return string("L(")+to_string(index)+"):";
	}
};

class IasmBlockStart
{
	public:
	
	string name;
	vector<IasmArgument> arguments;
	
	IasmBlockStart(){}
	explicit IasmBlockStart(const vector<string>& tokens)
	{
		size_t t=0;
		name=tokens.at(t);
		t++;
		
		while(true)
		{
			if(tokens.at(t)=="{") break;
			
			size_t i=t;
			for(;;i++)
			{
				if(tokens.at(i)=="," || tokens.at(i)=="{") break;
			}
			
			vector<string> argTokens(tokens.begin()+t,tokens.begin()+i);
			
			arguments.emplace_back(argTokens);
			
			t=i;
			if(tokens.at(t)==",") t++;
		}
		
		if(tokens.at(t)!="{") throw Error();
		t++;
	}
	string toString()
	{
		string str;
		for(int i=0;i<arguments.size();i++)
		{
			if(i>0) str+=",";
			str+=arguments[i].toString();
		}
		return name+" "+str+" {";
	}
};

class IasmBlockEnd
{
	public:
	
	IasmBlockEnd(){}
	explicit IasmBlockEnd(const vector<string>& tokens)
	{
		if(tokens.size()!=1) throw Error();
		if(tokens[0]!="}") throw Error();
	}
	string toString()
	{
		return "}";
	}
};

class IasmLine
{
	public:
	
	IasmLineType type=IasmLineType::none;
	
	IasmInstruction instruction;
	IasmLabel label;
	IasmBlockStart blockStart;
	IasmBlockEnd blockEnd;
	
	IasmLine(){}
	explicit IasmLine(const string& lineStr)
	{
		IasmTokenizer tokenizer;
		vector<string> tokens=tokenizer.tokenize(lineStr);
		
		try
		{
			if(tokens.size()==0) type=IasmLineType::none;
			else
			{
				string last=tokens.back();
				if(last==":")
				{
					type=IasmLineType::label;
					label=IasmLabel(tokens);
				}
				else if(last=="{")
				{
					type=IasmLineType::blockStart;
					blockStart=IasmBlockStart(tokens);
				}
				else if(last=="}")
				{
					type=IasmLineType::blockEnd;
					blockEnd=IasmBlockEnd(tokens);
				}
				else
				{
					type=IasmLineType::instruction;
					instruction=IasmInstruction(tokens);
				}
			}
		}
		catch(...)
		{
			throw Error();
		}
	}
	string toString()
	{
		if(type==IasmLineType::instruction) return instruction.toString();
		else if(type==IasmLineType::label) return label.toString();
		else if(type==IasmLineType::blockStart) return blockStart.toString();
		else if(type==IasmLineType::blockEnd) return blockEnd.toString();
		else return "";
	}
};

class IasmCode
{
	public:
	
	vector<IasmLine> lines;
	
	void initialize()
	{
		//TODO----
	}
	void addLine(const string& lineStr)
	{
		lines.emplace_back(lineStr);
	}
	string toString()
	{
		string str;
		for(int i=0;i<lines.size();i++)
		{
			str+=lines[i].toString()+"\n";
		}
		return str;
	}
};



class CodeError
{
	public:
	
	string message;
	
	CodeError(){}
	CodeError(const string& _message)
	{
		message=_message;
	}
};

class CodeToken
{
	public:
	
	string content;
	
	int line=0;
	int column=0;
	
	CodeToken(){}
	CodeToken(const string& _content,int _line,int _column)
	{
		content=_content;
		line=_line;
		column=_column;
	}
	string toString()
	{
		return content+" "+to_string(line)+":"+to_string(column);
	}
};

class TokenizedCode
{
	public:
	
	vector<CodeToken> tokens;
	
	vector<CodeError> errors;
	
	TokenizedCode(){}
	TokenizedCode(const string& inputCode)
	{
		size_t p=0;
		int lineNumber=1;
		size_t lineStart=0;
		
		bool insideToken=false;
		bool insideStringToken=false;
		bool stringTypeDoubleQuotes=false;
		
		bool insideComment=false;
		
		while(true)
		{
			if(p>=inputCode.size()) break;
			
			uint8_t c=inputCode[p];
			
			if(c=='\n')
			{
				insideToken=false;
				insideComment=false;
				lineNumber++;
				lineStart=p+1;
			}
			else if(insideComment){}
			else if(insideToken)
			{
				if(insideStringToken)
				{
					if(stringTypeDoubleQuotes && c=='\"' || !stringTypeDoubleQuotes && c=='\'')
					{
						tokens.back().content.push_back(c);
						insideToken=false;
					}
					else
					{
						tokens.back().content.push_back(c);
					}
				}
				else
				{
					if(isCharacterOfWordToken(c))
					{
						tokens.back().content.push_back(c);
					}
					else if(c=='\'' || c=='\"')
					{
						tokens.emplace_back(string(1,c),lineNumber,1+p-lineStart);
						insideStringToken=true;
						stringTypeDoubleQuotes=(c=='\"');
					}
					else if(isSpaceCharacter(c))
					{
						insideToken=false;
					}
					else
					{
						insideToken=false;
						
						bool isCommentStart=false;
						if(c=='/')
						{
							if(p+1<inputCode.size())
							{
								if(inputCode[p+1]=='/')
								{
									isCommentStart=true;
								}
							}
						}
						if(isCommentStart)
						{
							insideComment=true;
						}
						else
						{
							tokens.emplace_back(string(1,c),lineNumber,1+p-lineStart);
						}
					}
				}
			}
			else
			{
				if(isCharacterOfWordToken(c))
				{
					tokens.emplace_back(string(1,c),lineNumber,1+p-lineStart);
					insideToken=true;
					insideStringToken=false;
				}
				else if(c=='\'' || c=='\"')
				{
					tokens.emplace_back(string(1,c),lineNumber,1+p-lineStart);
					insideToken=true;
					insideStringToken=true;
					stringTypeDoubleQuotes=(c=='\"');
				}
				else if(isSpaceCharacter(c)){}
				else
				{
					bool isCommentStart=false;
					if(c=='/')
					{
						if(p+1<inputCode.size())
						{
							if(inputCode[p+1]=='/')
							{
								isCommentStart=true;
							}
						}
					}
					if(isCommentStart)
					{
						insideComment=true;
					}
					else
					{
						tokens.emplace_back(string(1,c),lineNumber,1+p-lineStart);
					}
				}
			}
			
			p++;
		}
	}
	bool isCharacterOfWordToken(uint8_t c)
	{
		return c>='a' && c<='z' || c>='A' && c<='Z' || c>='0' && c<='9' || c=='_';
	}
	bool isSpaceCharacter(uint8_t c)
	{
		return c==' ' || c=='\t' || c=='\n' || c=='\r';
	}
	
	void error(size_t tokenIndex,int errorCode,bool addError=true)
	{
		string errorMessage;
		if(tokenIndex<tokens.size())
		{
			errorMessage=string("Error:")+to_string(tokens[tokenIndex].line)+":"+to_string(tokens[tokenIndex].column)+": ERROR("+to_string(errorCode)+")";
		}
		else
		{
			errorMessage=string("Error:?:?: ERROR(")+to_string(errorCode)+")";
		}
		if(addError)
		{
			errors.emplace_back(errorMessage);
			throw Error(string("code error added to list: ")+errorMessage);
		}
		else throw Error(string("code error without adding it to list: ")+errorMessage);
	}
	bool hasErrors()
	{
		return errors.size()>0;
	}
	void printErrors()
	{
		for(int i=0;i<errors.size() && i<100;i++)
		{
			cout<<errors[i].message<<endl;
		}
	}
	
	bool contains(size_t tokenIndex)
	{
		return tokenIndex<tokens.size();
	}
	string get(size_t tokenIndex)
	{
		if(tokenIndex>=tokens.size())
		{
			error(tokenIndex,__LINE__);
			return "";
		}
		else
		{
			return tokens[tokenIndex].content;
		}
	}
	
	void assert(const string& referenceString,size_t tokenIndex,int errorCode,bool addError=true)
	{
		if(get(tokenIndex)!=referenceString)
		{
			error(tokenIndex,errorCode,addError);
		}
	}
	string getIdentifier(size_t tokenIndex,int errorCode,bool addError=true)
	{
		string token=get(tokenIndex);
		if(!isValidIdentifierName(token))
		{
			error(tokenIndex,errorCode,addError);
		}
		return token;
	}
	bool isValidIdentifierName(const string& name)
	{
		if(name.size()==0) return false;
		if(!isCharacterOfIdentifier(name[0],true)) return false;
		for(size_t i=0;i<name.size();i++)
		{
			if(!isCharacterOfIdentifier(name[i],false)) return false;
		}
		return true;
	}
	bool isCharacterOfIdentifier(uint8_t c,bool start)
	{
		if(start) return c>='a' && c<='z' || c>='A' && c<='Z' || c=='_';
		else return c>='a' && c<='z' || c>='A' && c<='Z' || c>='0' && c<='9' || c=='_';
	}
	
	size_t findMatchingBracket(size_t tokenIndex)
	{
		string bracketOpen=get(tokenIndex);
		string bracketClose;
		if(bracketOpen=="(") bracketClose=")";
		else if(bracketOpen=="{") bracketClose="}";
		else if(bracketOpen=="[") bracketClose="]";
		else
		{
			error(tokenIndex,__LINE__);
		}
		
		int bracketLevel=0;
		while(true)
		{
			string str=get(tokenIndex);
			if(str=="(" || str=="{" || str=="[") bracketLevel++;
			else if(str==")" || str=="}" || str=="]") bracketLevel--;
			
			if(bracketLevel==0) break;
			else tokenIndex++;
		}
		if(get(tokenIndex)==bracketClose)
		{
			return tokenIndex;
		}
		else
		{
			error(tokenIndex,__LINE__);
			return 0;
		}
	}
};



class DataType
{
	public:
	
	string name;
	
	bool isIntegerPrimitive=false;
	bool isPassedByReference=false;
	
	bool sizeAndAlignmentDefined=false;
	size_t size=0;
	size_t alignment=1;
	
	DataType(){}
	DataType(const string& _name,bool _isIntegerPrimitive,bool _isPassedByReference,bool _sizeAndAlignmentDefined,size_t _size,size_t _alignment)
	{
		name=_name;
		isIntegerPrimitive=_isIntegerPrimitive;
		isPassedByReference=_isPassedByReference;
		sizeAndAlignmentDefined=_sizeAndAlignmentDefined;
		size=_size;
		alignment=_alignment;
	}
};



class Type
{
	public:
	
	string name="u8";
	int pointerLevels=0;
	bool isReference=false;
	
	bool isNullptr=false;
	bool isIntegerLiteral=false;
	
	bool operator ==(const Type& other) const
	{
		if(name!=other.name) return false;
		if(pointerLevels!=other.pointerLevels) return false;
		if(isReference!=other.isReference) return false;
		if(isNullptr!=other.isNullptr) return false;
		if(isIntegerLiteral!=other.isIntegerLiteral) return false;
		return true;
	}
	bool operator !=(const Type& other) const
	{
		return !(*this==other);
	}
	
	Type(){}
	explicit Type(const string& _name,int _pointerLevels=0,bool _isReference=false)
	{
		name=_name;
		pointerLevels=_pointerLevels;
		isReference=_isReference;
		if(pointerLevels>maxPointerLevels()) throw Error(string("Type with pointer levels over maximum limit: ")+name);
	}
	Type(TokenizedCode& code,size_t& t,bool addErrors=true)
	{
		if(code.get(t)=="ref")
		{
			isReference=true;
			t++;
			code.assert("(",t,__LINE__,addErrors);
			t++;
		}
		while(true)
		{
			if(code.get(t)=="ptr")
			{
				pointerLevels++;
				if(pointerLevels>maxPointerLevels()) code.error(t,__LINE__,addErrors);
				t++;
				code.assert("(",t,__LINE__,addErrors);
				t++;
			}
			else
			{
				name=code.getIdentifier(t,__LINE__,addErrors);
				t++;
				break;
			}
		}
		for(int i=0;i<pointerLevels+(isReference ? 1 : 0);i++)
		{
			code.assert(")",t,__LINE__,addErrors);
			t++;
		}
	}
	
	bool isPointer() const
	{
		return pointerLevels>0;
	}
	Type getReference() const
	{
		Type type=*this;
		type.isReference=true;
		return type;
	}
	Type _getPointer() const
	{
		Type type=*this;
		type.pointerLevels++;
		if(type.pointerLevels>maxPointerLevels()) throw Error(string("Type with pointer levels over maximum limit: ")+name);
		type.isReference=false;
		return type;
	}
	Type getPointedType() const
	{
		Type type=*this;
		type.pointerLevels--;
		if(type.pointerLevels<0) throw Error(string("Type with pointer levels below minimum limit: ")+name);
		type.isReference=false;
		return type;
	}
	Type raw() const
	{
		Type type=*this;
		type.isReference=false;
		return type;
	}
	
	int maxPointerLevels() const
	{
		return 127;
	}
	
	string toString() const
	{
		string str;
		
		for(int i=0;i<pointerLevels;i++)
		{
			str+="ptr(";
		}
		
		str+=name;
		
		for(int i=0;i<pointerLevels;i++)
		{
			str+=")";
		}
		
		if(isReference) str=string("ref(")+str+")";
		
		return str;
	}
};

class FunctionInfo
{
	public:
	
	int classIndex=-1;
	string name;
	vector<Type> parameterTypes;
	
	bool returnTypeKnown=false;
	bool returnsSomething=false;
	Type returnType;
	
	FunctionInfo(){}
	FunctionInfo(int _classIndex,const string& _name,const vector<Type>& _parameterTypes)
	{
		classIndex=_classIndex;
		name=_name;
		parameterTypes=_parameterTypes;
	}
	FunctionInfo(int _classIndex,const string& _name,const vector<Type>& _parameterTypes,bool _returnsSomething,const Type& _returnType=Type())
	{
		classIndex=_classIndex;
		name=_name;
		parameterTypes=_parameterTypes;
		returnTypeKnown=true;
		returnsSomething=_returnsSomething;
		returnType=_returnType;
	}
};

class FunctionParameter
{
	public:
	
	size_t tokenIndex=0;
	
	Type type;
	string name;
	
	FunctionParameter(){}
	FunctionParameter(const Type& _type,const string& _name)
	{
		type=_type;
		name=_name;
	}
	
	string toString()
	{
		return type.toString()+" "+name;
	}
};

class Function
{
	public:
	
	size_t tokenIndex=0;
	
	bool isMethod=false;
	bool isOperator=false;
	string name;
	bool returnsSomething=false;
	Type returnType;
	vector<FunctionParameter> parameters;
	
	bool compilerGenerated=false;
	
	size_t codeStart=0;
	size_t codeEnd=0;
	
	Function(){}
	Function(bool _isMethod,bool _isOperator,const string& _name,bool _returnsSomething,const Type& _returnType,const vector<FunctionParameter>& _parameters,bool _compilerGenerated)
	{
		isMethod=_isMethod;
		isOperator=_isOperator;
		name=_name;
		returnsSomething=_returnsSomething;
		returnType=_returnType;
		parameters=_parameters;
		
		compilerGenerated=_compilerGenerated;
	}
	Function(TokenizedCode& code,size_t& t,bool _isMethod)
	{
		isMethod=_isMethod;
		
		tokenIndex=t;
		
		code.assert("fn",t,__LINE__);
		t++;
		
		if(code.get(t)=="operator")
		{
			isOperator=true;
			t++;
			
			code.assert("(",t,__LINE__);
			t++;
			
			while(true)
			{
				if(code.get(t)==")")
				{
					t++;
					break;
				}
				else
				{
					name+=code.get(t);
					t++;
				}
			}
		}
		else
		{
			isOperator=false;
			name=code.getIdentifier(t,__LINE__);
			t++;
		}
		
		code.assert("(",t,__LINE__);
		t++;
		
		if(code.get(t)==")")
		{
			returnsSomething=false;
			t++;
		}
		else
		{
			returnsSomething=true;
			
			returnType=Type(code,t);
			
			code.assert(")",t,__LINE__);
			t++;
		}
		
		code.assert("(",t,__LINE__);
		t++;
		
		if(code.get(t)==")") t++;
		else
		{
			while(true)
			{
				parameters.emplace_back();
				
				parameters.back().tokenIndex=t;
				parameters.back().type=Type(code,t);
				parameters.back().name=code.getIdentifier(t,__LINE__);
				for(int p=0;p<parameters.size()-1;p++)
				{
					if(parameters[p].name==parameters.back().name)
					{
						code.error(t,__LINE__);
					}
				}
				t++;
				
				if(code.get(t)==")")
				{
					t++;
					break;
				}
				else
				{
					code.assert(",",t,__LINE__);
					t++;
				}
			}
		}
		
		if(isOperator)
		{
			if(!isValidOperator()) code.error(t,__LINE__);
		}
		
		code.assert("{",t,__LINE__);
		codeStart=t;
		t=code.findMatchingBracket(t);
		codeEnd=t;
		t++;
	}
	
	bool isValidOperator()
	{
		if(name=="[]")
		{
			return parameters.size()==1;
		}
		
		vector<string> binaryOperatorNames=vector<string>{
			"*","/","%","+","-","<<",">>","<","<=",">",">=","==","!=","&","^","|","&&","||"
			};
		vector<string> binaryAssignmentOperatorNames=vector<string>{
			"=","*=","/=","%=","+=","-=","<<=",">>=","&=","^=","|="
			};
		vector<string> unaryOperatorNames=vector<string>{
			"-","!","~"
			};
		vector<string> unaryAssignmentOperatorNames=vector<string>{
			"++","--"
			};
		
		vector<string>*operatorNames=nullptr;
		
		if(isMethod)
		{
			if(parameters.size()==1 && !returnsSomething)
			{
				operatorNames=&binaryAssignmentOperatorNames;
			}
			else if(parameters.size()==0 && !returnsSomething)
			{
				operatorNames=&unaryAssignmentOperatorNames;
			}
		}
		else
		{
			if(parameters.size()==2 && returnsSomething)
			{
				operatorNames=&binaryOperatorNames;
			}
			else if(parameters.size()==1 && returnsSomething)
			{
				operatorNames=&unaryOperatorNames;
			}
		}
		
		if(operatorNames==nullptr) return false;
		
		for(int i=0;i<operatorNames->size();i++)
		{
			if((*operatorNames)[i]==name) return true;
		}
		return false;
	}
	
	vector<Type> getParameterTypes()
	{
		vector<Type> parameterTypes;
		for(int i=0;i<parameters.size();i++)
		{
			parameterTypes.push_back(parameters[i].type);
		}
		return parameterTypes;
	}
	FunctionInfo getInfoWithoutReturnType(int classIndex)
	{
		return FunctionInfo(classIndex,name,getParameterTypes());
	}
	
	string toString()
	{
		string str="fn ";
		
		if(isOperator) str+=string("operator(")+name+")";
		else str+=name;
		
		if(returnsSomething)
		{
			str+=string("(")+returnType.toString()+")";
		}
		else str+="()";
		
		str+="(";
		for(int i=0;i<parameters.size();i++)
		{
			str+=parameters[i].toString();
			if(i+1<parameters.size()) str+=",";
		}
		str+="){}";
		
		return str;
	}
};

class ClassAttribute
{
	public:
	
	size_t tokenIndex=0;
	
	Type type;
	string name;
	
	size_t objectOffset=0;
	
	ClassAttribute(){}
	ClassAttribute(TokenizedCode& code,size_t& t)
	{
		tokenIndex=t;
		
		type=Type(code,t);
		name=code.getIdentifier(t,__LINE__);
		t++;
		code.assert(";",t,__LINE__);
		t++;
	}
	
	string toString()
	{
		return type.toString()+" "+name+";"+" //offset="+to_string(objectOffset);
	}
};

class Class
{
	public:
	
	size_t tokenIndex=0;
	
	string name;
	vector<ClassAttribute> attributes;
	vector<Function> methods;
	
	size_t objectSize=0;
	size_t objectAlignment=1;
	
	Class(){}
	Class(TokenizedCode& code,size_t& t)
	{
		tokenIndex=t;
		
		code.assert("class",t,__LINE__);
		t++;
		
		name=code.getIdentifier(t,__LINE__);
		t++;
		
		code.assert("{",t,__LINE__);
		t++;
		
		while(code.get(t)!="}")
		{
			string str=code.get(t);
			
			if(str=="fn")
			{
				methods.emplace_back(code,t,true);
			}
			else
			{
				attributes.emplace_back(code,t);
				for(int a=0;a<attributes.size()-1;a++)
				{
					if(attributes[a].name==attributes.back().name)
					{
						code.error(attributes.back().tokenIndex,__LINE__);
					}
				}
			}
		}
		
		code.assert("}",t,__LINE__);
		t++;
	}
	
	string toString()
	{
		string str=string("class ")+name+" //size="+to_string(objectSize)+" alignment="+to_string(objectAlignment)+"\n{\n";
		
		for(int i=0;i<attributes.size();i++)
		{
			str+=string("    ")+attributes[i].toString()+"\n";
		}
		
		for(int i=0;i<methods.size();i++)
		{
			str+=string("    ")+methods[i].toString()+"\n";
		}
		
		str+="}";
		
		return str;
	}
};

class GlobalVariable
{
	public:
	
	size_t tokenIndex=0;
	
	Type type;
	string name;
	
	GlobalVariable(){}
	GlobalVariable(TokenizedCode& code,size_t& t)
	{
		tokenIndex=t;
		
		type=Type(code,t);
		name=code.getIdentifier(t,__LINE__);
		t++;
		code.assert(";",t,__LINE__);
		t++;
	}
	
	string toString()
	{
		return type.toString()+" "+name+";";
	}
};

class CodeAssembly
{
	public:
	
	string startCode;
	
	string functionCode;
	
	string globalVariableCode;
	int bssSize=0;
	
	string stringCode;
	int numberOfStrings=0;
	
	string identifierDataCode;
};

size_t toAlign(size_t offset,size_t alignment)
{
	size_t m=offset%alignment;
	if(m==0) return 0;
	else return alignment-m;
}

class Variable
{
	public:
	
	Type type;
	bool hasName=false;
	string name;
	int iasmIndex=0;
	
	Variable(){}
	explicit Variable(const Type& _type)
	{
		type=_type;
		hasName=false;
	}
	Variable(const Type& _type,const string& _name)
	{
		type=_type;
		hasName=true;
		name=_name;
	}
};

class MachineRegisters
{
	public:
	
	vector<string> regs1;
	vector<string> regs2;
	vector<string> regs4;
	vector<string> regs8;
	
	void initialize()
	{
		regs1=vector<string>{"al","bl","cl","dl","sil","dil","spl","bpl","r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"};
		regs2=vector<string>{"ax","bx","cx","dx","si","di","sp","bp","r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"};
		regs4=vector<string>{"eax","ebx","ecx","edx","esi","edi","esp","ebp","r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"};
		regs8=vector<string>{"rax","rbx","rcx","rdx","rsi","rdi","rsp","rbp","r8","r9","r10","r11","r12","r13","r14","r15"};
	}
};

class LoopContext
{
	public:
	
	bool inLoop=false;
	
	int variableIndexStart=0;
	
	int continueLabel=0;
	int breakLabel=0;
	
	LoopContext(){}
	LoopContext(int _variableIndexStart,int _continueLabel,int _breakLabel)
	{
		inLoop=true;
		variableIndexStart=_variableIndexStart;
		continueLabel=_continueLabel;
		breakLabel=_breakLabel;
	}
};

class TryContext
{
	public:
	
	bool inTry=false;
	
	int variableIndexStart=0;
	
	int catchLabel=0;
	
	TryContext(){}
	TryContext(int _variableIndexStart,int _catchLabel)
	{
		inTry=true;
		variableIndexStart=_variableIndexStart;
		catchLabel=_catchLabel;
	}
};

class Scope
{
	public:
	
	NamedVector<Variable> variables;
	
	int scopeVariableIndexStart=0;
	
	Scope(){}
	Scope(const NamedVector<Variable>& _variables,int _scopeVariableIndexStart)
	{
		variables=_variables;
		scopeVariableIndexStart=_scopeVariableIndexStart;
	}
};

class FunctionContext
{
	public:
	
	int classIndex=-1;
	int functionIndex=-1;
	Function*functionPtr=nullptr;
	bool isMethod=false;
	
	NamedVector<Variable> variables;
	
	int variableToReturn=-1;
	int variableToThrow=-1;
	
	int returnLabel=-1;
	int returnThrowLabel=-1;
	
	int functionVariableIndexStart=0;
	int scopeVariableIndexStart=0;
	
	int labelsCreated=0;
	
	int iasmVarIndexes=0;
	
	LoopContext loopContext;
	
	TryContext tryContext;
	
	FunctionContext(){}
	FunctionContext(int _classIndex,int _functionIndex,Function*_functionPtr)
	{
		classIndex=_classIndex;
		functionIndex=_functionIndex;
		functionPtr=_functionPtr;
		isMethod=(classIndex!=-1);
	}
	Scope enterScope()
	{
		Scope outScope(variables,scopeVariableIndexStart);
		
		scopeVariableIndexStart=variables.size();
		
		return outScope;
	}
	void exitScope(const Scope& outScope)
	{
		variables=outScope.variables;
		scopeVariableIndexStart=outScope.scopeVariableIndexStart;
	}
	LoopContext startLoop(int continueLabel,int breakLabel)
	{
		LoopContext oldLoopContext=loopContext;
		loopContext=LoopContext(variables.size(),continueLabel,breakLabel);
		return oldLoopContext;
	}
	void endLoop(const LoopContext& oldLoopContext)
	{
		loopContext=oldLoopContext;
	}
	TryContext startTry(int catchLabel)
	{
		TryContext oldTryContext=tryContext;
		tryContext=TryContext(variables.size(),catchLabel);
		return oldTryContext;
	}
	void endTry(const TryContext& oldTryContext)
	{
		tryContext=oldTryContext;
	}
};

class CodeIdentifier
{
	public:
	
	string name;
	
	CodeIdentifier(){}
	CodeIdentifier(const string& _name)
	{
		name=_name;
	}
};

class Code
{
	public:
	
	TokenizedCode code;
	
	vector<Function> functions;
	vector<Class> classes;
	vector<GlobalVariable> globalVariables;
	
	NamedVector<DataType> dataTypes;
	int dataTypesPassByReferenceStartIndex=0;
	int dataTypesClassesStartIndex=0;
	size_t pointerSize=8;
	size_t pointerAlignment=8;
	
	NamedVector<CodeIdentifier> codeIdentifiers;
	
	IasmCode iasmCode;
	
	FunctionContext functionContext;
	
	size_t t=0;
	
	int numberOfDataLabels=0;
	
	Code(){}
	void parse(const TokenizedCode& _code)
	{
		code=_code;
		
		t=0;
		while(code.contains(t))
		{
			string str=code.get(t);
			
			if(str=="fn")
			{
				functions.emplace_back(code,t,false);
			}
			else if(str=="class")
			{
				classes.emplace_back(code,t);
			}
			else
			{
				globalVariables.emplace_back(code,t);
			}
		}
	}
	
	string toString()
	{
		string str;
		
		for(int i=0;i<classes.size();i++)
		{
			str+=classes[i].toString()+"\n";
		}
		
		for(int i=0;i<globalVariables.size();i++)
		{
			str+=globalVariables[i].toString()+"\n";
		}
		
		for(int i=0;i<functions.size();i++)
		{
			str+=functions[i].toString()+"\n";
		}
		
		return str;
	}
	
	private:
		void processParsedDataCheckFunction(Function& function)
		{
			if(function.returnsSomething)
			{
				if(!typeExists(function.returnType))
				{
					code.error(function.tokenIndex,__LINE__);
				}
			}
			for(int i=0;i<function.parameters.size();i++)
			{
				if(!typeExists(function.parameters[i].type))
				{
					code.error(function.parameters[i].tokenIndex,__LINE__);
				}
			}
		}
	public:
	void processParsedData()
	{
		//Keep integer types in this same position and order
		dataTypes.add(DataType("u8",true,false,true,1,1));//Position 0
		dataTypes.add(DataType("u16",true,false,true,2,2));//Position 1
		dataTypes.add(DataType("u32",true,false,true,4,4));//Position 2
		dataTypes.add(DataType("u64",true,false,true,8,8));//Position 3
		dataTypes.add(DataType("i8",true,false,true,1,1));//Position 4
		dataTypes.add(DataType("i16",true,false,true,2,2));//Position 5
		dataTypes.add(DataType("i32",true,false,true,4,4));//Position 6
		dataTypes.add(DataType("i64",true,false,true,8,8));//Position 7
		
		dataTypesPassByReferenceStartIndex=dataTypes.size();
		
		dataTypes.add(DataType("dynamic",false,true,true,16,8));
		
		dataTypesClassesStartIndex=dataTypes.size();
		
		for(int i=0;i<classes.size();i++)
		{
			if(!dataTypes.add(DataType(classes[i].name,false,true,false,0,1)))
			{
				code.error(classes[i].tokenIndex,__LINE__);
			}
		}
		
		{
			for(int i=0;i<classes.size();i++)
			{
				for(int j=0;j<classes[i].attributes.size();j++)
				{
					if(!typeExists(classes[i].attributes[j].type))
					{
						code.error(classes[i].attributes[j].tokenIndex,__LINE__);
					}
				}
				for(int j=0;j<classes[i].methods.size();j++)
				{
					processParsedDataCheckFunction(classes[i].methods[j]);
				}
			}
			
			for(int i=0;i<functions.size();i++)
			{
				processParsedDataCheckFunction(functions[i]);
			}
			
			for(int i=0;i<globalVariables.size();i++)
			{
				if(!typeExists(globalVariables[i].type))
				{
					code.error(globalVariables[i].tokenIndex,__LINE__);
				}
			}
		}
		
		vector<int> classDefined(classes.size(),false);
		int classesDefined=0;
		
		while(true)
		{
			for(int i=0;i<classes.size();i++)
			{
				if(!classDefined[i])
				{
					if(defineClass(i))
					{
						classDefined[i]=true;
						
						DataType& dataType=dataTypes[classes[i].name];
						dataType.size=classes[i].objectSize;
						dataType.alignment=classes[i].objectAlignment;
						dataType.sizeAndAlignmentDefined=true;
					}
				}
			}
			
			int newClassesDefined=0;
			for(int i=0;i<classes.size();i++)
			{
				if(classDefined[i]) newClassesDefined++;
			}
			
			if(newClassesDefined==classes.size()) break;
			
			if(newClassesDefined==classesDefined)
			{
				code.error(-1,__LINE__);
			}
			classesDefined=newClassesDefined;
		}
	}
	private:
		bool defineClass(int classIndex)
		{
			Class& c=classes[classIndex];
			
			c.objectSize=0;
			c.objectAlignment=1;
			
			size_t offset=0;
			
			for(int i=0;i<c.attributes.size();i++)
			{
				ClassAttribute& a=c.attributes[i];
				
				size_t aSize=0;
				size_t aAlignment=1;
				
				if(a.type.pointerLevels>0 || a.type.isReference)
				{
					aSize=pointerSize;
					aAlignment=pointerAlignment;
				}
				else
				{
					try
					{
						DataType& dataType=dataTypes[a.type.name];
						if(!dataType.sizeAndAlignmentDefined)
						{
							return false;
						}
						
						aSize=dataType.size;
						aAlignment=dataType.alignment;
					}
					catch(...)
					{
						code.error(a.tokenIndex,__LINE__);
					}
				}
				
				if(aAlignment==0)
				{
					code.error(a.tokenIndex,__LINE__);
				}
				a.objectOffset=offset+toAlign(offset,aAlignment);
				
				offset+=aSize;
				
				if(aAlignment>c.objectAlignment) c.objectAlignment=aAlignment;
			}
			
			c.objectSize=offset+toAlign(offset,c.objectAlignment);
			
			return true;
		}
		void getTypeSizeAndAlignment(const Type& type,size_t& size,size_t& alignment)
		{
			if(type.isPointer() || type.isReference)
			{
				size=pointerSize;
				alignment=pointerAlignment;
			}
			else
			{
				DataType& dataType=dataTypes[type.name];
				size=dataType.size;
				alignment=dataType.alignment;
			}
		}
		size_t getTypeSize(const Type& type)
		{
			size_t size;
			size_t alignment;
			getTypeSizeAndAlignment(type,size,alignment);
			return size;
		}
		size_t getTypeAlignment(const Type& type)
		{
			size_t size;
			size_t alignment;
			getTypeSizeAndAlignment(type,size,alignment);
			return alignment;
		}
		bool functionMatches(Function& function,const FunctionInfo& info,bool acceptTypeVariations=false)
		{
			if(function.name!=info.name) return false;
			if(info.returnTypeKnown)
			{
				if(function.returnsSomething!=info.returnsSomething) return false;
				if(info.returnsSomething)
				{
					if(function.returnType!=info.returnType) return false;
				}
			}
			if(function.parameters.size()!=info.parameterTypes.size()) return false;
			for(int p=0;p<info.parameterTypes.size();p++)
			{
				if(function.parameters[p].type!=info.parameterTypes[p])
				{
					if(acceptTypeVariations)
					{
						if(!isAssignable(function.parameters[p].type,info.parameterTypes[p])) return false;
					}
					else return false;
				}
			}
			return true;
		}
		int findFunctionIndex(const FunctionInfo& info,bool acceptTypeVariations=false)
		{
			if(info.classIndex!=-1) error(__LINE__);
			for(int i=0;i<functions.size();i++)
			{
				if(functionMatches(functions[i],info,acceptTypeVariations)) return i;
			}
			return -1;
		}
		int findFunctionIndexFromName(const string& name)
		{
			for(int i=0;i<functions.size();i++)
			{
				if(functions[i].name==name) return i;
			}
			return -1;
		}
		int findMethodIndexFromName(int classIndex,const string& name)
		{
			Class& c=classes[classIndex];
			for(int i=0;i<c.methods.size();i++)
			{
				if(c.methods[i].name==name) return i;
			}
			return -1;
		}
		int findClassIndex(const string& name)
		{
			for(int i=0;i<classes.size();i++)
			{
				if(classes[i].name==name)
				{
					return i;
				}
			}
			return -1;
		}
		int findMethodIndex(const FunctionInfo& info,bool acceptTypeVariations=false)
		{
			if(info.classIndex==-1) error(__LINE__);
			Class& c=classes[info.classIndex];
			for(int i=0;i<c.methods.size();i++)
			{
				if(functionMatches(c.methods[i],info,acceptTypeVariations)) return i;
			}
			return -1;
		}
		int findEmptyConstructorIndex(int classIndex)
		{
			return findMethodIndex(FunctionInfo(classIndex,"constructor",vector<Type>{},false));
		}
		int findGlobalVariableIndex(const string& name)
		{
			for(int i=0;i<globalVariables.size();i++)
			{
				if(globalVariables[i].name==name)
				{
					return i;
				}
			}
			return -1;
		}
		bool typeIsClass(const Type& type)
		{
			if(type.pointerLevels>0 || type.isReference) return false;
			if(dataTypes.getIndexOf(type.name)<dataTypesClassesStartIndex) return false;
			return true;
		}
		ClassAttribute*findClassAttribute(const Type& type,const string& name)
		{
			if(!typeIsClass(type.raw())) return nullptr;
			
			for(int i=0;i<classes.size();i++)
			{
				if(classes[i].name==type.name)
				{
					for(int j=0;j<classes[i].attributes.size();j++)
					{
						if(classes[i].attributes[j].name==name)
						{
							return &classes[i].attributes[j];
						}
					}
					return nullptr;
				}
			}
			return nullptr;
		}
		int getIdentifierIndex(const string& name)
		{
			try
			{
				return codeIdentifiers.getIndexOf(name);
			}
			catch(...)
			{
				return -1;
			}
		}
		int createIdentifier(const string& name)
		{
			int index=getIdentifierIndex(name);
			if(index==-1)
			{
				codeIdentifiers.add(CodeIdentifier(name));
				return codeIdentifiers.size()-1;
			}
			return index;
		}
		int createDataLabel()
		{
			int label=numberOfDataLabels;
			numberOfDataLabels++;
			return label;
		}
		void compileIdentifierData()
		{
			identifierDataStart();
			
			int emptyFunctionIndex=findFunctionIndexFromName("__empty");
			if(emptyFunctionIndex==-1) error(__LINE__);
			vector<int> integerCopyFunctionIndexes(4,-1);
			integerCopyFunctionIndexes[0]=findFunctionIndexFromName("__integer_copy8");
			integerCopyFunctionIndexes[1]=findFunctionIndexFromName("__integer_copy16");
			integerCopyFunctionIndexes[2]=findFunctionIndexFromName("__integer_copy32");
			integerCopyFunctionIndexes[3]=findFunctionIndexFromName("__integer_copy64");
			for(int i=0;i<integerCopyFunctionIndexes.size();i++)
			{
				if(integerCopyFunctionIndexes[i]==-1) error(__LINE__);
			}
			
			vector<int> typeLabels;
			for(int i=0;i<dataTypes.size();i++)
			{
				typeLabels.push_back(createDataLabel());
				putDataLabelValue(typeLabels.back());
			}
			
			for(int i=0;i<dataTypes.size();i++)
			{
				compileIdentifierDataOfType(i,typeLabels[i],emptyFunctionIndex,integerCopyFunctionIndexes);
			}
			
			identifierDataEnd();
		}
		void compileIdentifierDataOfType(int typeIndex,int typeLabel,int emptyFunctionIndex,const vector<int>& integerCopyFunctionIndexes)
		{
			putDataLabel(typeLabel);
			
			DataType& dataType=dataTypes[typeIndex];
			
			putData(dataType.size);
			putData(dataType.alignment);
			
			if(dataType.isIntegerPrimitive)
			{
				int size=dataType.size;
				int sizeIndex=0;
				if(size==1) sizeIndex=0;
				else if(size==2) sizeIndex=1;
				else if(size==4) sizeIndex=2;
				else if(size==8) sizeIndex=3;
				else error(__LINE__);
				
				putFunctionAddress(-1,emptyFunctionIndex);
				putFunctionAddress(-1,emptyFunctionIndex);
				putFunctionAddress(-1,integerCopyFunctionIndexes[sizeIndex]);
				
				putData(0);
				putData(0);
				
				putData(0);
				putData(0);
			}
			else if(dataType.name=="dynamic")
			{
				int constructorIndex=findFunctionIndexFromName("__dynamic_constructor");
				int destructorIndex=findFunctionIndexFromName("__dynamic_destructor");
				int assignmentIndex=findFunctionIndexFromName("__dynamic_assignment_operator");
				
				if(constructorIndex==-1) error(__LINE__);
				if(destructorIndex==-1) error(__LINE__);
				if(assignmentIndex==-1) error(__LINE__);
				
				putFunctionAddress(-1,constructorIndex);
				putFunctionAddress(-1,destructorIndex);
				putFunctionAddress(-1,assignmentIndex);
				
				putData(0);
				putData(0);
				
				putData(0);
				putData(0);
			}
			else
			{
				compileIdentifierDataOfClass(findClassIndex(dataType.name));
			}
		}
		void compileIdentifierDataOfClass(int classIndex)
		{
			Class& c=classes[classIndex];
			
			int constructorIndex=findEmptyConstructorIndex(classIndex);
			int destructorIndex=findMethodIndexFromName(classIndex,"destructor");
			int assignmentIndex=findMethodIndexFromName(classIndex,"=");
			
			if(constructorIndex==-1) error(__LINE__);
			if(destructorIndex==-1) error(__LINE__);
			if(assignmentIndex==-1) error(__LINE__);
			
			putFunctionAddress(-1,constructorIndex);
			putFunctionAddress(-1,destructorIndex);
			putFunctionAddress(-1,assignmentIndex);
			
			
			putData(c.attributes.size());
			int attributeListLabel=createDataLabel();
			putDataLabelValue(attributeListLabel);
			
			putData(c.methods.size());
			int methodListLabel=createDataLabel();
			putDataLabelValue(methodListLabel);
			
			
			putDataLabel(attributeListLabel);
			for(int i=0;i<c.attributes.size();i++)
			{
				int identifierIndex=createIdentifier(c.attributes[i].name);
				int typeInteger=getTypeInteger(c.attributes[i].type);
				int offset=c.attributes[i].objectOffset;
				
				putData(identifierIndex);
				putData(typeInteger);
				putData(offset);
				putData(0);
			}
			
			putDataLabel(methodListLabel);
			vector<int> methodParameterListLabels;
			for(int i=0;i<c.methods.size();i++)
			{
				Function& function=c.methods[i];
				
				int identifierIndex=createIdentifier(function.name);
				putData(identifierIndex);
				
				putFunctionAddress(classIndex,i);
				
				if(function.returnsSomething) putData(getTypeInteger(function.returnType));
				else putData(int64_t(int(-1)));
				
				putData(function.parameters.size());
				
				methodParameterListLabels.push_back(createDataLabel());
				putDataLabelValue(methodParameterListLabels.back());
				
				putData(0);
				
				putData(0);
				
				putData(0);
			}
			
			for(int i=0;i<c.methods.size();i++)
			{
				putDataLabel(methodParameterListLabels[i]);
				
				Function& function=c.methods[i];
				
				for(int j=0;j<function.parameters.size();j++)
				{
					putData(getTypeInteger(function.parameters[j].type));
				}
			}
		}
		void compileGlobalVariable(int globalVariableIndex)
		{
			GlobalVariable& v=globalVariables[globalVariableIndex];
			Type type=v.type;
			putGlobalVariable(globalVariableIndex,getTypeSize(type),getTypeAlignment(type));
		}
	public:
	string compile()
	{
		iasmCode.initialize();
		
		checkGlobalVariablesDontCollide();
		checkFunctionsDontCollide();
		checkMethodsDontCollide();
		
		for(int i=0;i<globalVariables.size();i++)
		{
			compileGlobalVariable(i);
		}
		
		addDefaultFunctions();
		
		compileIdentifierData();
		
		for(int i=0;i<functions.size();i++)
		{
			compileFunction(-1,i);
		}
		
		for(int i=0;i<classes.size();i++)
		{
			for(int j=0;j<classes[i].methods.size();j++)
			{
				compileFunction(i,j);
			}
		}
		
		int mainFunctionIndex=findFunctionIndexFromName("__main");
		if(mainFunctionIndex==-1) error(__LINE__);
		
		putMainFunction(-1,mainFunctionIndex);
		
		return iasmCode.toString();//TODO----
	}
	private:
		void checkGlobalVariablesDontCollide()
		{
			for(int i=0;i<globalVariables.size();i++)
			{
				for(int j=0;j<i;j++)
				{
					if(globalVariables[i].name==globalVariables[j].name)
					{
						code.error(globalVariables[i].tokenIndex,__LINE__);
					}
				}
			}
		}
		void checkFunctionsDontCollide()
		{
			for(int i=0;i<functions.size();i++)
			{
				FunctionInfo info=functions[i].getInfoWithoutReturnType(-1);
				for(int j=0;j<i;j++)
				{
					if(functionMatches(functions[j],info))
					{
						code.error(functions[i].tokenIndex,__LINE__);
					}
				}
			}
		}
		void checkMethodsDontCollide()
		{
			for(int i=0;i<classes.size();i++)
			{
				checkMethodsDontCollide(i);
			}
		}
		void checkMethodsDontCollide(int classIndex)
		{
			Class& c=classes[classIndex];
			for(int i=0;i<c.methods.size();i++)
			{
				FunctionInfo info=c.methods[i].getInfoWithoutReturnType(classIndex);
				for(int j=0;j<i;j++)
				{
					if(functionMatches(c.methods[j],info))
					{
						code.error(c.methods[i].tokenIndex,__LINE__);
					}
				}
			}
		}
		void addDefaultFunctions()
		{
			for(int i=0;i<classes.size();i++)
			{
				addDefaultMethodsOfClass(i);
			}
		}
		void addDefaultMethodsOfClass(int classIndex)
		{
			addDefaultConstructor(classIndex);
			addDefaultDestructor(classIndex);
			addDefaultAssignmentOperator(classIndex);
		}
		void addDefaultConstructor(int classIndex)
		{
			classes[classIndex].methods.emplace_back(true,false,"__dc",false,Type(),vector<FunctionParameter>{},true);
			if(findMethodIndex(FunctionInfo(classIndex,"constructor",vector<Type>{},false))==-1)
			{
				classes[classIndex].methods.emplace_back(true,false,"constructor",false,Type(),vector<FunctionParameter>{},true);
			}
		}
		void addDefaultDestructor(int classIndex)
		{
			classes[classIndex].methods.emplace_back(true,false,"__dd",false,Type(),vector<FunctionParameter>{},true);
			if(findMethodIndex(FunctionInfo(classIndex,"destructor",vector<Type>{},false))==-1)
			{
				classes[classIndex].methods.emplace_back(true,false,"destructor",false,Type(),vector<FunctionParameter>{},true);
			}
		}
		void addDefaultAssignmentOperator(int classIndex)
		{
			Type type=Type(classes[classIndex].name);
			if(findMethodIndex(FunctionInfo(classIndex,"=",vector<Type>{type},false))==-1 &&
				findMethodIndex(FunctionInfo(classIndex,"=",vector<Type>{type.getReference()},false))==-1)
			{
				classes[classIndex].methods.emplace_back(true,true,"=",false,Type(),
					vector<FunctionParameter>{FunctionParameter(type,"other")},true);
			}
		}
		void error(int errorCode)
		{
			code.error(t,errorCode);
		}
		void assert(const string& str,int errorCode)
		{
			code.assert(str,t,errorCode);
		}
		uint64_t getTypeInteger(const Type& type)
		{
			uint64_t value=dataTypes.getIndexOf(type.name);
			if(value>=uint64_t(0xffffffffffffff)) error(__LINE__);
			if(type.pointerLevels<0 || type.pointerLevels>127) error(__LINE__);
			value|=(uint64_t(type.pointerLevels)<<56);
			if(type.isReference) value|=(uint64_t(1)<<63);
			return value;
		}
		Type getTypeOf(int var)
		{
			return functionContext.variables[var].type;
		}
		Type getPointer(const Type& type)
		{
			try
			{
				return type._getPointer();
			}
			catch(...)
			{
				error(__LINE__);
				return Type();
			}
		}
		bool isTypePassedByReference(const Type& type)
		{
			if(type.pointerLevels>0 || type.isReference)
			{
				return false;
			}
			else
			{
				DataType& dataType=dataTypes[type.name];
				return dataType.isPassedByReference;
			}
		}
		bool isSignedIntegerPrimitiveTypeName(const string& type)
		{
			return type=="i8" || type=="i16" || type=="i32" || type=="i64";
		}
		bool isSignedInteger(Type type)
		{
			if(type.isReference || type.isPointer()) return false;
			return isSignedIntegerPrimitiveTypeName(type.name);
		}
		void iasmAdd(const string& str,int errorCode)
		{
			try
			{
				iasmCode.addLine(str);
			}
			catch(...)
			{
				error(errorCode);
			}
		}
		void iasmAdd(const string& instruction,const vector<string>& outputArguments,const vector<string>& inputArguments,int errorCode)
		{
			string str=instruction+" ";
			for(int i=0;i<outputArguments.size();i++)
			{
				if(i>0) str+=",";
				str+=outputArguments[i];
			}
			str+=";";
			for(int i=0;i<inputArguments.size();i++)
			{
				if(i>0) str+=",";
				str+=inputArguments[i];
			}
			
			iasmAdd(str,errorCode);
		}
		string iasmVar(int var)
		{
			return string("var(")+to_string(functionContext.variables[var].iasmIndex)+")";
		}
		string iasmVarValue(int var)
		{
			return string("[")+iasmVar(var)+"]";
		}
		string iasmVarValueDeref(int var)
		{
			return string("[[")+iasmVar(var)+"]]";
		}
		string iasmGlobalVar(int index)
		{
			return string("globalvar(")+to_string(index)+")";
		}
		string iasmImm(int64_t imm)
		{
			return string("imm(")+to_string(imm)+")";
		}
		string iasmLabel(int label)
		{
			return string("label(")+to_string(label)+")";
		}
		string iasmEmpty()
		{
			return "empty";
		}
		string iasmReg(const string& reg)
		{
			return string("reg(")+reg+")";
		}
		string iasmStr(const string& str)
		{
			return string("\"")+str+"\"";
		}
		int createVariable(const Variable& varToCreate)
		{
			int var=createVariableWithoutCallingConstructor(varToCreate);
			callConstructor(var);
			return var;
		}
		int createVariableWithoutCallingConstructor(Variable varToCreate)
		{
			int var=functionContext.variables.size();
			
			varToCreate.iasmIndex=functionContext.iasmVarIndexes;
			functionContext.iasmVarIndexes++;
			
			if(!functionContext.variables.add(varToCreate,varToCreate.hasName))
			{
				error(__LINE__);
			}
			
			Type type=getTypeOf(var);
			
			//Allocate memory from stack
			iasmAdd("var",vector<string>{iasmVar(var)},vector<string>{iasmImm(getTypeSize(type)),iasmImm(getTypeAlignment(type))},__LINE__);
			
			return var;
		}
		int createVariable(Type type)
		{
			return createVariable(Variable(type));
		}
		int createVariableWithoutCallingConstructor(Type type)
		{
			return createVariableWithoutCallingConstructor(Variable(type));
		}
		void freeVariable(int var)
		{
			//Free stack memory
			iasmAdd("varfree",vector<string>{},vector<string>{iasmVar(var)},__LINE__);
		}
		int createLabel()
		{
			functionContext.labelsCreated++;
			return functionContext.labelsCreated-1;
		}
		void movRaw(int va,int vb)
		{
			//copy vb to va
			Type ta=getTypeOf(va);
			Type tb=getTypeOf(vb);
			int sa=getTypeSize(ta);
			int sb=getTypeSize(tb);
			string instruction="mov";
			if(isSignedInteger(ta) && isSignedInteger(tb) && sa!=sb)
			{
				instruction="movs";
			}
			iasmAdd(instruction,vector<string>{iasmVarValue(va)},vector<string>{iasmVarValue(vb),iasmImm(sa),iasmImm(sb)},__LINE__);
		}
		void movRawGetAddress(int va,int vb)
		{
			//copy vb's address to va
			iasmAdd("mov",vector<string>{iasmVarValue(va)},vector<string>{iasmVar(vb),iasmImm(pointerSize),iasmImm(pointerSize)},__LINE__);
		}
		void movRawGetDeref(int va,int vb)
		{
			//copy vb's pointed value to va
			Type ta=getTypeOf(va);
			int sa=getTypeSize(ta);
			iasmAdd("mov",vector<string>{iasmVarValue(va)},vector<string>{iasmVarValueDeref(vb),iasmImm(sa),iasmImm(sa)},__LINE__);
		}
		void movRawSetDeref(int va,int vb)
		{
			//copy vb to va's pointedValue
			Type tb=getTypeOf(vb);
			int sb=getTypeSize(tb);
			iasmAdd("mov",vector<string>{iasmVarValueDeref(va)},vector<string>{iasmVarValue(vb),iasmImm(sb),iasmImm(sb)},__LINE__);
		}
		void movRawGetGlobalAddress(int va,int globalIndex)
		{
			//copy global's address to va
			iasmAdd("mov",vector<string>{iasmVarValue(va)},vector<string>{iasmGlobalVar(globalIndex),iasmImm(pointerSize),iasmImm(pointerSize)},__LINE__);
		}
		void movRawGetParameter(int va,int parameterIndex)
		{
			//TODO copy parameter(register or stack) to va
		}
		void movRawGetRegister(int v,const string& reg)
		{
			//copy reg to v
			Type type=getTypeOf(v);
			int size=getTypeSize(type);
			iasmAdd("mov",vector<string>{iasmVarValue(v)},vector<string>{iasmReg(reg),iasmImm(size),iasmImm(size)},__LINE__);
		}
		void movRawSetRegister(const string& reg,int v)
		{
			//copy v to reg
			Type type=getTypeOf(v);
			int size=getTypeSize(type);
			iasmAdd("mov",vector<string>{iasmReg(reg)},vector<string>{iasmVarValue(v),iasmImm(size),iasmImm(size)},__LINE__);
		}
		void movRawImmediate(int va,uint64_t imm)
		{
			//copy imm to va
			Type ta=getTypeOf(va);
			int sa=getTypeSize(ta);
			iasmAdd("mov",vector<string>{iasmVarValue(va)},vector<string>{iasmImm(imm),iasmImm(sa),iasmImm(sa)},__LINE__);
		}
		void movRawImmediateString(int va,const vector<uint8_t>& bytes)
		{
			//TODO create raw string from bytes and copy address to va
		}
		void movRawImmediate_datatypeStart(int va)
		{
			//TODO copy __datatype_start to va
		}
		void addRaw(int va,int vb)
		{
			//add vb to va
			Type ta=getTypeOf(va);
			int sa=getTypeSize(ta);
			iasmAdd("add",vector<string>{iasmVarValue(va)},vector<string>{iasmVarValue(vb),iasmImm(sa)},__LINE__);
		}
		void addRawImmediate(int va,uint64_t imm)
		{
			//add imm to va
			Type ta=getTypeOf(va);
			int sa=getTypeSize(ta);
			iasmAdd("add",vector<string>{iasmVarValue(va)},vector<string>{iasmImm(imm),iasmImm(sa)},__LINE__);
		}
		void movHalfRawAddress(int va,int vb)
		{
			//copy addressof(vb) to va, checking if vb is a reference, but not cheking va
			Type typeb=getTypeOf(vb);
			if(typeb.isReference) movRaw(va,vb);
			else movRawGetAddress(va,vb);
		}
		void mov(int va,int vb)
		{
			//copy vb to va
			Type typea=getTypeOf(va);
			if(typea.isReference) movRawSetDeref(va,getValue(vb));
			else movRaw(va,getValue(vb));
		}
		void functionStart(int classIndex,int functionIndex)
		{
			//start function
			iasmAdd("function",vector<string>{},vector<string>{iasmImm(classIndex),iasmImm(functionIndex)},__LINE__);
		}
		void functionEnd()
		{
			//end function
			iasmAdd("endfunction",vector<string>{},vector<string>{},__LINE__);
		}
		void jmpLabel(int label)
		{
			//jump to label
			iasmAdd("jmp",vector<string>{},vector<string>{iasmLabel(label)},__LINE__);
		}
		void jmpRawIfZero(int var,int label)
		{
			//jump to label if var is zero
			iasmAdd("jz",vector<string>{},vector<string>{iasmVarValue(var),iasmImm(getTypeSize(getTypeOf(var))),iasmLabel(label)},__LINE__);
		}
		void putLabel(int label)
		{
			//put label definition
			iasmAdd(string("L(")+to_string(label)+"):",__LINE__);
		}
		void putAssemblyLine(const string& line)
		{
			//put inline assembly line
			iasmAdd("asm",vector<string>{},vector<string>{iasmStr(line)},__LINE__);
		}
		void retRaw(int var=-1)
		{
			//function return
			iasmAdd("ret",vector<string>{},vector<string>{var!=-1 ? iasmVarValue(var) : iasmEmpty()},__LINE__);
		}
		void retRawThrow(int var)
		{
			//function return with throw
			iasmAdd("retthrow",vector<string>{},vector<string>{iasmVarValue(var)},__LINE__);
		}
		void callRaw(int classIndex,int functionIndex,const vector<int>& arguments,int returnVar,int throwVar,int skipThrowLabel)
		{
			//call function raw
			vector<string> inputs=vector<string>{iasmImm(classIndex),iasmImm(functionIndex)};
			inputs.push_back(throwVar!=-1 ? iasmLabel(skipThrowLabel) : iasmEmpty());
			for(int i=0;i<arguments.size();i++)
			{
				inputs.push_back(iasmVarValue(arguments[i]));
			}
			
			vector<string> outputs;
			outputs.push_back(returnVar!=-1 ? iasmVarValue(returnVar) : iasmEmpty());
			outputs.push_back(throwVar!=-1 ? iasmVarValue(throwVar) : iasmEmpty());
			
			iasmAdd("call",outputs,inputs,__LINE__);
		}
		void rawBlockConstructorStart(int var)
		{
			//constructor start block
			iasmAdd(string("constructor ")+iasmVar(var)+" {",__LINE__);
		}
		void rawBlockDestructorStart(int var)
		{
			//destructor start block
			iasmAdd(string("destructor ")+iasmVar(var)+" {",__LINE__);
		}
		void rawBlockInlineAssemblyStart()
		{
			//inline assembly start block
			iasmAdd("inlineasm {",__LINE__);
		}
		void rawBlockEnd()
		{
			//block end
			iasmAdd("}",__LINE__);
		}
		void putInlineAssemblyModifiedContent(const string& str)
		{
			//put str as modified content
			iasmAdd("asmmodifies",vector<string>{},vector<string>{iasmStr(str)},__LINE__);
		}
		void identifierDataStart()
		{
			//TODO identifier data start
		}
		void identifierDataEnd()
		{
			//TODO identifier data end
		}
		void putData(uint64_t value)
		{
			//TODO
		}
		void putFunctionAddress(int classIndex,int functionIndex)
		{
			//TODO
		}
		void putDataLabel(int label)
		{
			//TODO
		}
		void putDataLabelValue(int label)
		{
			//TODO
		}
		void putGlobalVariable(int index,int size,int alignment)
		{
			//put global variable
			iasmAdd("globalvar",vector<string>{iasmGlobalVar(index)},vector<string>{iasmImm(size),iasmImm(alignment)},__LINE__);
		}
		void putMainFunction(int classIndex,int functionIndex)
		{
			iasmAdd("mainfunction",vector<string>{},vector<string>{iasmImm(classIndex),iasmImm(functionIndex)},__LINE__);
		}
		int callMethod(const FunctionInfo& functionInfo,int objectVar,const vector<int>& arguments,bool acceptTypeVariations=false)
		{
			if(functionInfo.classIndex==-1) error(__LINE__);
			vector<int> args=arguments;
			args.insert(args.begin(),objectVar);
			return callFunctionRaw(functionInfo,args,acceptTypeVariations);
		}
		int callFunction(const FunctionInfo& functionInfo,const vector<int>& arguments,bool acceptTypeVariations=false)
		{
			if(functionInfo.classIndex!=-1) code.error(-1,__LINE__);
			return callFunctionRaw(functionInfo,arguments,acceptTypeVariations);
		}
		int getValue(int varin)
		{
			int varout=varin;
			Type type=getTypeOf(varin);
			if(type.isReference)
			{
				varout=createVariable(type.raw());
				movRawGetDeref(varout,varin);
			}
			return varout;
		}
		int getReference(int varin)
		{
			int varout=varin;
			Type type=getTypeOf(varin);
			if(!type.isReference)
			{
				varout=createVariable(type.getReference());
				movRawGetAddress(varout,varin);
			}
			return varout;
		}
		int getReferenceAsPtri64(int varin)
		{
			int varout=createVariable(Type("i64",1));
			movRaw(varout,getReference(varin));
			return varout;
		}
		int getTypeVariable(Type type)
		{
			int var=createVariable(Type("i64"));
			movRawImmediate(var,getTypeInteger(type));
			return var;
		}
		int getAttributeReference(int objectVar,ClassAttribute& attribute)
		{
			int var=createVariable(attribute.type.getReference());
			movHalfRawAddress(var,objectVar);
			addRawImmediate(var,attribute.objectOffset);
			return var;
		}
		void compileFunction(int classIndex,int functionIndex)
		{
			functionStart(classIndex,functionIndex);
			
			Function*functionPtr=nullptr;
			if(classIndex==-1) functionPtr=&functions[functionIndex];
			else functionPtr=&classes[classIndex].methods[functionIndex];
			
			functionContext=FunctionContext(classIndex,functionIndex,functionPtr);
			
			functionContext.returnLabel=createLabel();
			functionContext.returnThrowLabel=createLabel();
			
			Function& function=*functionContext.functionPtr;
			
			{
				int inputParameterIndex=0;
				
				if(function.returnsSomething)
				{
					if(isTypePassedByReference(function.returnType) && !function.returnType.isReference)
					{
						functionContext.variableToReturn=createVariable(Variable(function.returnType.getReference()));
						
						movRawGetParameter(functionContext.variableToReturn,inputParameterIndex);
						inputParameterIndex++;
					}
					else
					{
						functionContext.variableToReturn=createVariable(Variable(function.returnType));
					}
				}
				
				if(functionContext.isMethod)
				{
					Class& c=classes[classIndex];
					
					int thisVar=createVariable(Variable(Type(c.name).getReference(),"this"));
					
					movRawGetParameter(thisVar,inputParameterIndex);
					inputParameterIndex++;
				}
				
				for(int i=0;i<function.parameters.size();i++)
				{
					Type type=function.parameters[i].type;
					if(isTypePassedByReference(type)) type=type.getReference();
					int var=createVariable(Variable(type,function.parameters[i].name));
					
					movRawGetParameter(var,inputParameterIndex);
					inputParameterIndex++;
				}
				
				functionContext.variableToThrow=createVariable(Variable(Type("dynamic").getReference()));
			}
			
			functionContext.functionVariableIndexStart=functionContext.variables.size();
			functionContext.enterScope();
			
			
			if(functionContext.isMethod && function.name=="constructor")
			{
				callMethod(FunctionInfo(classIndex,"__dc",vector<Type>{},false),functionContext.variables.getIndexOf("this"),vector<int>{});
			}
			
			if(function.compilerGenerated)
			{
				if(functionContext.isMethod)
				{
					if(function.name=="__dc")
					{
						callConstructorsOfAttributesOfThis();
					}
					else if(function.name=="__dd")
					{
						callDestructorsOfAttributesOfThis();
					}
					else if(function.name=="=")
					{
						callAssignmentOperatorsOfAttributesOfThis();
					}
				}
			}
			else
			{
				t=function.codeStart;
				compileScope();
			}
			
			jmpLabel(functionContext.returnLabel);
			{
				putLabel(functionContext.returnThrowLabel);
				retRawThrow(functionContext.variableToThrow);
			}
			putLabel(functionContext.returnLabel);
			
			if(functionContext.isMethod && function.name=="destructor")
			{
				callMethod(FunctionInfo(classIndex,"__dd",vector<Type>{},false),functionContext.variables.getIndexOf("this"),vector<int>{});
			}
			
			if(function.returnsSomething)
			{
				if(!isTypePassedByReference(function.returnType) || function.returnType.isReference)
				{
					retRaw(functionContext.variableToReturn);
				}
				else retRaw();
			}
			else retRaw();
			
			functionEnd();
		}
		void compileScope()
		{
			Scope outScope=functionContext.enterScope();
			
			assert("{",__LINE__);
			t++;
			
			while(true)
			{
				if(code.get(t)=="}")
				{
					break;
				}
				compileStatement();
			}
			
			assert("}",__LINE__);
			t++;
			
			compileExitScope();
			functionContext.exitScope(outScope);
		}
		void compileExitScope()
		{
			callDestructorsUntilVariableIndex(functionContext.scopeVariableIndexStart,true);
		}
		void callDestructorsUntilVariableIndex(int variableIndexStart,bool freeStackSpace=false)
		{
			for(int i=int(functionContext.variables.size())-1;i>=variableIndexStart;i--)
			{
				callDestructor(i);
				if(freeStackSpace) freeVariable(i);
			}
		}
		bool isIntegerPrimitive(const Type& type)
		{
			if(type.pointerLevels>0 || type.isReference) return false;
			return dataTypes[type.name].isIntegerPrimitive;
		}
		bool isTypeWithConstructorAndDestructor(const Type& type)
		{
			if(type.pointerLevels>0 || type.isReference) return false;
			if(dataTypes[type.name].isIntegerPrimitive) return false;
			return true;
		}
		bool isTypeWithAssignmentOperator(const Type& type)
		{
			if(type.pointerLevels>0 || type.isReference) return false;
			if(dataTypes[type.name].isIntegerPrimitive) return false;
			return true;
		}
		void callConstructor(int var)
		{
			callConstructorOrDestructor(var,"constructor");
		}
		void callDestructor(int var)
		{
			callConstructorOrDestructor(var,"destructor");
		}
		void callConstructorOrDestructor(int var,const string& methodName)
		{
			Type type=getTypeOf(var);
			if(!type.isReference)
			{
				if(isTypeWithConstructorAndDestructor(type))
				{
					if(type==Type("dynamic"))
					{
						if(methodName=="constructor")
						{
							rawBlockConstructorStart(var);
							movRawImmediate(var,0);
							rawBlockEnd();
						}
						else
						{
							rawBlockDestructorStart(var);
							callFunction(FunctionInfo(-1,"__dynamic_destructor",vector<Type>{Type("i64",1)},false),vector<int>{getReferenceAsPtri64(var)});
							rawBlockEnd();
						}
					}
					else
					{
						int classIndex=findClassIndex(type.name);
						if(classIndex==-1) error(__LINE__);
						if(methodName=="constructor") rawBlockConstructorStart(var);
						else rawBlockDestructorStart(var);
						callMethod(FunctionInfo(classIndex,methodName,vector<Type>{},false),var,vector<int>{});
						rawBlockEnd();
					}
				}
			}
		}
		void callAssignmentOperatorOfDynamic(int varl,int varr)
		{
			if(getTypeOf(varl).raw()!=Type("dynamic")) error(__LINE__);
			Type type=getTypeOf(varr).raw();
			
			callFunction(FunctionInfo(-1,"__dynamic_assign",vector<Type>{Type("i64",1),Type("i64",1),Type("i64")},false),
				vector<int>{getReferenceAsPtri64(varl),getReferenceAsPtri64(varr),getTypeVariable(type)});
		}
		void callAssignmentOperator(int varl,int varr)
		{
			Type typel=getTypeOf(varl);
			Type typer=getTypeOf(varr);
			if(typel.raw()==Type("dynamic"))
			{
				callAssignmentOperatorOfDynamic(varl,varr);
			}
			else
			{
				if(typer.raw()==Type("dynamic"))
				{
					varr=castFromDynamic(varr,typel);
					typer=getTypeOf(varr);
				}
				
				if(typel.isPointer()) error(__LINE__);
				
				int classIndex=findClassIndex(typel.name);
				if(classIndex==-1) error(__LINE__);
				
				callMethod(FunctionInfo(classIndex,"=",vector<Type>{typer}),varl,vector<int>{varr},true);
			}
		}
		void callConstructorsOfAttributesOfThis()
		{
			Class& c=classes[functionContext.classIndex];
			for(int i=0;i<c.attributes.size();i++)
			{
				callConstructorOrDestructorOfAttributeOfThis(i,"constructor");
			}
		}
		void callDestructorsOfAttributesOfThis()
		{
			Class& c=classes[functionContext.classIndex];
			for(int i=int(c.attributes.size())-1;i>=0;i--)
			{
				callConstructorOrDestructorOfAttributeOfThis(i,"destructor");
			}
		}
		void callAssignmentOperatorsOfAttributesOfThis()
		{
			Class& c=classes[functionContext.classIndex];
			for(int i=0;i<c.attributes.size();i++)
			{
				callAssignmentOperatorOfAttributeOfThis(i);
			}
		}
		void callAssignmentOperatorOfAttributeOfThis(int attributeIndex)
		{
			int thisVar=functionContext.variables.getIndexOf("this");
			int otherVar=functionContext.variables.getIndexOf("other");
			
			ClassAttribute& attribute=classes[functionContext.classIndex].attributes[attributeIndex];
			
			Type type;
			if(attribute.type.isReference) type=getPointer(attribute.type).getReference();
			else type=attribute.type.getReference();
			
			int varl=createVariable(type);
			movHalfRawAddress(varl,thisVar);
			addRawImmediate(varl,attribute.objectOffset);
			
			int varr=createVariable(type);
			movHalfRawAddress(varr,otherVar);
			addRawImmediate(varr,attribute.objectOffset);
			
			if(isTypeWithAssignmentOperator(attribute.type))
			{
				if(attribute.type==Type("dynamic"))
				{
					callAssignmentOperatorOfDynamic(varl,varr);
				}
				else
				{
					int classIndex=findClassIndex(attribute.type.name);
					if(classIndex==-1) error(__LINE__);
					
					callMethod(FunctionInfo(classIndex,"=",vector<Type>{attribute.type}),varl,vector<int>{varr},true);
				}
			}
			else
			{
				int vartmp=createVariable(type.raw());
				movRawGetDeref(vartmp,varr);
				movRawSetDeref(varl,vartmp);
			}
		}
		void callConstructorOrDestructorOfAttributeOfThis(int attributeIndex,const string& methodName)
		{
			int thisVar=functionContext.variables.getIndexOf("this");
			
			ClassAttribute& attribute=classes[functionContext.classIndex].attributes[attributeIndex];
			
			Type type=attribute.type;
			
			if(isTypeWithConstructorAndDestructor(type))
			{
				int var=getAttributeReference(thisVar,attribute);
				
				if(type==Type("dynamic"))
				{
					if(methodName=="constructor")
					{
						int tmp=createVariable(Type("i64"));
						movRawImmediate(tmp,0);
						movRawSetDeref(var,tmp);
					}
					else
					{
						callFunction(FunctionInfo(-1,"__dynamic_destructor",vector<Type>{Type("i64",1)},false),vector<int>{getReferenceAsPtri64(var)});
					}
				}
				else
				{
					int classIndex=findClassIndex(type.name);
					if(classIndex==-1) error(__LINE__);
					callMethod(FunctionInfo(classIndex,methodName,vector<Type>{},false),var,vector<int>{});
				}
			}
		}
		int callFunctionIndirect(int addressVar,const vector<int>& arguments,int returnTypeVar,bool isDestructor)
		{
			return -1;//TODO--------
		}
		bool isLiteral(int var)
		{
			Type type=getTypeOf(var);
			return type.isNullptr || type.isIntegerLiteral;
		}
		int convertLiteral(int var,const Type& outputType)
		{
			Type type=getTypeOf(var);
			int varout=var;
			if(isLiteral(var))
			{
				if(type.isNullptr)
				{
					if(!outputType.isPointer()) error(__LINE__);
				}
				if(type.isIntegerLiteral)
				{
					if(!isIntegerPrimitive(outputType.raw())) error(__LINE__);
				}
				varout=createVariable(outputType);
				mov(varout,var);
			}
			else if(type.raw()!=outputType.raw()) error(__LINE__);
			return varout;
		}
		vector<int> callFunctionTransformArguments(const vector<Type>& parameterTypes,const vector<int>& inputArgs)
		{
			vector<int> outputArgs;
			for(int i=0;i<inputArgs.size();i++)
			{
				int varin=inputArgs[i];
				Type type=getTypeOf(varin);
				Type ptype=parameterTypes[i];
				if(isTypePassedByReference(ptype)) ptype=ptype.getReference();
				
				int var=varin;
				
				if(type.raw()!=ptype.raw())
				{
					if(ptype.raw()==Type("dynamic") || type.raw()==Type("dynamic"))
					{
						//TODO----
					}
					else
					{
						var=convertLiteral(varin,ptype);
					}
					
					if(type.isReference) type=ptype.getReference();
					else type=ptype.raw();
				}
				
				int varout=var;
				
				if(ptype.isReference && !type.isReference)
				{
					varout=createVariable(type.getReference());
					movRawGetAddress(varout,var);
				}
				else if(!ptype.isReference && type.isReference)
				{
					varout=createVariable(type.raw());
					movRawGetDeref(varout,var);
				}
				
				outputArgs.push_back(varout);
			}
			return outputArgs;
		}
		void manageException()
		{
			if(functionContext.tryContext.inTry)
			{
				callDestructorsUntilVariableIndex(functionContext.tryContext.variableIndexStart);
				jmpLabel(functionContext.tryContext.catchLabel);
			}
			else
			{
				callDestructorsUntilVariableIndex(functionContext.functionVariableIndexStart);
				jmpLabel(functionContext.returnThrowLabel);
			}
		}
		int callFunctionRaw(const FunctionInfo& functionInfo,const vector<int>& arguments,bool acceptTypeVariations=false)
		{
			int returnVar=-1;
			
			int classIndex=functionInfo.classIndex;
			int functionIndex=-1;
			if(classIndex==-1) functionIndex=findFunctionIndex(functionInfo,acceptTypeVariations);
			else functionIndex=findMethodIndex(functionInfo,acceptTypeVariations);
			if(functionIndex==-1) error(__LINE__);
			
			Function*functionPtr=nullptr;
			if(classIndex==-1) functionPtr=&functions[functionIndex];
			else functionPtr=&classes[classIndex].methods[functionIndex];
			Function& function=*functionPtr;
			
			bool isDestructor=false;
			if(function.isMethod)
			{
				if(function.name=="destructor") isDestructor=true;
			}
			else if(function.name=="__dynamic_destructor") isDestructor=true;
			
			Type thisType;
			if(function.isMethod) thisType=Type(classes[classIndex].name).getReference();
			
			if(function.returnsSomething) returnVar=createVariable(function.returnType);
			
			bool addReturnValueToParameters=false;
			if(function.returnsSomething)
			{
				if(!function.returnType.isReference)
				{
					if(isTypePassedByReference(function.returnType))
					{
						addReturnValueToParameters=true;
					}
				}
			}
			
			vector<Type> parameterTypes=function.getParameterTypes();
			if(function.isMethod) parameterTypes.insert(parameterTypes.begin(),thisType);
			if(addReturnValueToParameters) parameterTypes.insert(parameterTypes.begin(),function.returnType.getReference());
			
			vector<int> args=arguments;
			
			if(addReturnValueToParameters) args.insert(args.begin(),returnVar);
			
			vector<int> finalArgs=callFunctionTransformArguments(parameterTypes,args);
			
			int skipThrowLabel=createLabel();
			callRaw(classIndex,functionIndex,finalArgs,function.returnsSomething ? returnVar : -1,!isDestructor ? functionContext.variableToThrow : -1,skipThrowLabel);
			if(!isDestructor) manageException();
			putLabel(skipThrowLabel);
			
			return returnVar;
		}
		bool typeExists(const Type& type)
		{
			try
			{
				dataTypes[type.name];
				return true;
			}
			catch(...)
			{
				return false;
			}
		}
		bool parseCheckType(Type& type)
		{
			size_t tokenIndex=t;
			
			try
			{
				type=Type(code,tokenIndex,false);
			}
			catch(...)
			{
				return false;
			}
			
			if(!typeExists(type)) return false;
			
			t=tokenIndex;
			
			return true;
		}
		bool parseIntegerBinary(const string& str,uint64_t& integer)
		{
			integer=0;
			
			for(int i=0;i<str.size();i++)
			{
				integer<<=1;
				if(str[i]=='0'){}
				else if(str[i]=='1') integer|=1;
				else return false;
			}
			
			return true;
		}
		bool parseIntegerOctal(const string& str,uint64_t& integer)
		{
			integer=0;
			
			for(int i=0;i<str.size();i++)
			{
				uint8_t c=str[i];
				integer<<=3;
				if(c>='0' && c<='7') integer|=c-'0';
				else return false;
			}
			
			return true;
		}
		bool parseIntegerHexadecimal(const string& str,uint64_t& integer)
		{
			integer=0;
			
			for(int i=0;i<str.size();i++)
			{
				uint8_t c=str[i];
				integer<<=4;
				if(c>='0' && c<='9') integer|=c-'0';
				else if(c>='a' && c<='f') integer|=10+c-'a';
				else if(c>='A' && c<='F') integer|=10+c-'A';
				else return false;
			}
			
			return true;
		}
		bool parseInteger(const string& token,uint64_t& integer)
		{
			if(token.size()==0) return false;
			
			if(token[0]=='0' && token.size()>=3)
			{
				if(token[1]=='x')
				{
					return parseIntegerHexadecimal(token.substr(2),integer);
				}
				else if(token[1]=='o')
				{
					return parseIntegerOctal(token.substr(2),integer);
				}
				else if(token[1]=='b')
				{
					return parseIntegerBinary(token.substr(2),integer);
				}
			}
			
			try
			{
				integer=stoull(token);
			}
			catch(...)
			{
				return false;
			}
			return true;
		}
		bool parseCheckIntegerToken(uint64_t& integer)
		{
			size_t tokenIndex=t;
			
			string token=code.get(tokenIndex);
			
			if(!parseInteger(token,integer)) return false;
			
			tokenIndex++;
			
			t=tokenIndex;
			
			return true;
		}
		bool parseString(const string& token,string& content)
		{
			content="";
			if(token.size()<2) return false;
			
			uint8_t type=token[0];
			if(type!='\'' && type!='\"') return false;
			if(token.back()!=type) return false;
			
			string input=token.substr(1,token.size()-2);
			
			bool escape=false;
			for(int i=0;i<input.size();i++)
			{
				uint8_t c=input[i];
				if(escape)
				{
					if(c=='n') content.push_back('\n');
					else if(c=='r') content.push_back('\r');
					else if(c=='t') content.push_back('\t');
					else content.push_back(c);
					
					escape=false;
				}
				else
				{
					if(c=='\\')
					{
						escape=true;
					}
					else
					{
						content.push_back(c);
					}
				}
			}
			
			return true;
		}
		bool parseCheckStringLiteralToken(string& literal)
		{
			size_t tokenIndex=t;
			
			string token=code.get(tokenIndex);
			
			if(!parseString(token,literal)) return false;
			
			tokenIndex++;
			
			t=tokenIndex;
			
			return true;
		}
		void jmpIfZero(int var,int label)
		{
			Type type=getTypeOf(var);
			if(!isIntegerPrimitive(type.raw())) error(__LINE__);
			if(type.isReference)
			{
				int varDeref=createVariable(type.raw());
				movRawGetDeref(varDeref,var);
				jmpRawIfZero(varDeref,label);
			}
			else jmpRawIfZero(var,label);
		}
		void compileVariableDeclarationOrExpression()
		{
			bool isVariableDeclaration=false;
			Type type;
			size_t tok=t;
			if(parseCheckType(type))
			{
				if(code.get(t)!="(")
				{
					isVariableDeclaration=true;
					t=tok;
					compileVariableDeclaration();
				}
				else
				{
					t=tok;
				}
			}
			
			if(!isVariableDeclaration)
			{
				compileExpression();
			}
		}
		void compileStatement()
		{
			string start=code.get(t);
			
			if(start=="{")
			{
				compileScope();
			}
			else if(start=="return")
			{
				t++;
				
				Function& function=*functionContext.functionPtr;
				
				if(function.returnsSomething)
				{
					int var=compileExpressionPart();
					if(var==-1) error(__LINE__);
					
					assign(functionContext.variableToReturn,var);
				}
				
				callDestructorsUntilVariableIndex(functionContext.functionVariableIndexStart);
				jmpLabel(functionContext.returnLabel);
				
				assert(";",__LINE__);
				t++;
			}
			else if(start=="throw")
			{
				t++;
				
				int var=compileExpressionPart();
				if(var==-1) error(__LINE__);
				
				int sizeVar=createVariable(Type("i64"));
				movRawImmediate(sizeVar,16);
				
				int allocated=callFunction(FunctionInfo(-1,"__malloc",vector<Type>{Type("i64")}),vector<int>{sizeVar});
				movRaw(functionContext.variableToThrow,allocated);
				
				assign(functionContext.variableToThrow,var);
				
				if(functionContext.tryContext.inTry)
				{
					callDestructorsUntilVariableIndex(functionContext.tryContext.variableIndexStart);
					jmpLabel(functionContext.tryContext.catchLabel);
				}
				else
				{
					callDestructorsUntilVariableIndex(functionContext.functionVariableIndexStart);
					jmpLabel(functionContext.returnThrowLabel);
				}
				
				assert(";",__LINE__);
				t++;
			}
			else if(start=="try")
			{
				t++;
				
				int catchLabel=createLabel();
				TryContext oldTryContext=functionContext.startTry(catchLabel);
				
				int skipCatchLabel=createLabel();
				
				compileScope();
				
				jmpLabel(skipCatchLabel);
				
				functionContext.endTry(oldTryContext);
				
				assert("catch",__LINE__);
				t++;
				
				assert("(",__LINE__);
				t++;
				
				assert("dynamic",__LINE__);
				t++;
				
				string variableName=code.getIdentifier(t,__LINE__);
				t++;
				
				assert(")",__LINE__);
				t++;
				
				putLabel(catchLabel);
				
				Scope outScope=functionContext.enterScope();
				{
					int var=createVariable(Variable(Type("dynamic"),variableName));
					
					assign(var,functionContext.variableToThrow);
					
					callFunction(FunctionInfo(-1,"__free",vector<Type>{Type("u8",1)}),vector<int>{getReference(functionContext.variableToThrow)});
					
					compileScope();
				}
				compileExitScope();
				functionContext.exitScope(outScope);
				
				putLabel(skipCatchLabel);
			}
			else if(start=="if")
			{
				t++;
				
				assert("(",__LINE__);
				t++;
				
				int var=compileExpressionPart();
				if(var==-1) error(__LINE__);
				
				assert(")",__LINE__);
				t++;
				
				if(!isIntegerPrimitive(getTypeOf(var).raw())) error(__LINE__);
				
				int endIfLabel=createLabel();
				int elseLabel=createLabel();
				
				jmpIfZero(var,elseLabel);
				
				if(code.get(t)=="{") compileScope();
				else compileStatement();
				
				jmpLabel(endIfLabel);
				putLabel(elseLabel);
				
				while(code.get(t)=="elif")
				{
					t++;
					
					assert("(",__LINE__);
					t++;
					
					int elifVar=compileExpressionPart();
					if(elifVar==-1) error(__LINE__);
					
					assert(")",__LINE__);
					t++;
					
					if(!isIntegerPrimitive(getTypeOf(elifVar).raw())) error(__LINE__);
					
					elseLabel=createLabel();
					
					jmpIfZero(elifVar,elseLabel);
					
					if(code.get(t)=="{") compileScope();
					else compileStatement();
					
					jmpLabel(endIfLabel);
					putLabel(elseLabel);
				}
				
				if(code.get(t)=="else")
				{
					t++;
					
					if(code.get(t)=="{") compileScope();
					else compileStatement();
				}
				
				putLabel(endIfLabel);
			}
			else if(start=="while")
			{
				t++;
				
				assert("(",__LINE__);
				t++;
				
				int repeatLabel=createLabel();
				int endLabel=createLabel();
				
				putLabel(repeatLabel);
				
				
				int conditionVar=compileExpressionPart();
				if(conditionVar==-1) error(__LINE__);
				
				assert(")",__LINE__);
				t++;
				
				if(!isIntegerPrimitive(getTypeOf(conditionVar).raw())) error(__LINE__);
				
				jmpIfZero(conditionVar,endLabel);
				
				LoopContext outLoopContext=functionContext.startLoop(repeatLabel,endLabel);
				
				compileScope();
				
				functionContext.endLoop(outLoopContext);
				
				jmpLabel(repeatLabel);
				
				putLabel(endLabel);
			}
			else if(start=="for")
			{
				t++;
				
				Scope outScope=functionContext.enterScope();
				
				
				assert("(",__LINE__);
				t++;
				
				if(code.get(t)==";")
				{
					t++;
				}
				else
				{
					compileVariableDeclarationOrExpression();
				}
				
				int repeatLabel=createLabel();
				int continueLabel=createLabel();
				int endLabel=createLabel();
				putLabel(repeatLabel);
				
				if(code.get(t)!=";")
				{
					int conditionVar=compileExpressionPart();
					if(conditionVar==-1) error(__LINE__);
					
					if(!isIntegerPrimitive(getTypeOf(conditionVar).raw())) error(__LINE__);
					
					jmpIfZero(conditionVar,endLabel);
				}
				
				assert(";",__LINE__);
				t++;
				
				
				int skipChangeLabel=createLabel();
				
				jmpLabel(skipChangeLabel);
				
				putLabel(continueLabel);
				
				
				if(code.get(t)!=")")
				{
					compileExpressionPart();
				}
				assert(")",__LINE__);
				t++;
				
				
				jmpLabel(repeatLabel);
				
				putLabel(skipChangeLabel);
				
				
				LoopContext outLoopContext=functionContext.startLoop(continueLabel,endLabel);
				
				compileScope();
				
				functionContext.endLoop(outLoopContext);
				
				
				jmpLabel(continueLabel);
				
				putLabel(endLabel);
				
				
				compileExitScope();
				functionContext.exitScope(outScope);
			}
			else if(start=="break" || start=="continue")
			{
				t++;
				
				assert(";",__LINE__);
				t++;
				
				if(!functionContext.loopContext.inLoop) error(__LINE__);
				
				callDestructorsUntilVariableIndex(functionContext.loopContext.variableIndexStart);
				
				int label=-1;
				if(start=="break") label=functionContext.loopContext.breakLabel;
				else if(start=="continue") label=functionContext.loopContext.continueLabel;
				else error(__LINE__);
				
				jmpLabel(label);
			}
			else if(start=="asm")
			{
				compileInlineAssembly();
			}
			else
			{
				compileVariableDeclarationOrExpression();
			}
		}
		void compileVariableDeclaration()
		{
			Type type=Type(code,t);
			
			string name=code.getIdentifier(t,__LINE__);
			t++;
			
			int var=createVariable(Variable(type,name));
			
			if(code.get(t)=="=")
			{
				t--;
				compileExpression();
			}
			else
			{
				assert(";",__LINE__);
				t++;
			}
		}
		void compileInlineAssembly()
		{
			assert("asm",__LINE__);
			t++;
			
			assert("(",__LINE__);
			t++;
			
			vector<string> asmLiterals;
			
			while(true)
			{
				string token=code.get(t);
				if(token==")" || token==",")
				{
					break;
				}
				
				string literal;
				if(parseCheckStringLiteralToken(literal))
				{
					asmLiterals.push_back(literal);
				}
				else error(__LINE__);
			}
			
			vector<pair<string,int>> inputVars;
			vector<pair<string,int>> outputVars;
			vector<string> modifiedContents;
			
			while(true)
			{
				if(code.get(t)==")")
				{
					break;
				}
				assert(",",__LINE__);
				t++;
				
				string literal;
				if(!parseCheckStringLiteralToken(literal)) error(__LINE__);
				
				if(code.get(t)=="(")
				{
					t++;
					
					string reg=literal;
					bool isOutput=false;
					if(literal.size()>0)
					{
						if(literal[0]=='=')
						{
							reg=literal.substr(1);
							isOutput=true;
						}
					}
					
					string variableName=code.getIdentifier(t,__LINE__);
					
					try
					{
						int var=functionContext.variables.getIndexOf(variableName);
						if(isOutput) outputVars.emplace_back(reg,var);
						else inputVars.emplace_back(reg,var);
					}
					catch(...)
					{
						error(__LINE__);
					}
					t++;
					
					assert(")",__LINE__);
					t++;
				}
				else
				{
					modifiedContents.push_back(literal);
				}
			}
			
			rawBlockInlineAssemblyStart();
			for(int i=0;i<inputVars.size();i++)
			{
				movRawSetRegister(inputVars[i].first,inputVars[i].second);
			}
			for(int i=0;i<modifiedContents.size();i++)
			{
				putInlineAssemblyModifiedContent(modifiedContents[i]);
			}
			for(int i=0;i<asmLiterals.size();i++)
			{
				putAssemblyLine(asmLiterals[i]);
			}
			for(int i=0;i<outputVars.size();i++)
			{
				movRawGetRegister(outputVars[i].second,outputVars[i].first);
			}
			rawBlockEnd();
			
			assert(")",__LINE__);
			t++;
			
			assert(";",__LINE__);
			t++;
		}
		void compileExpression()
		{
			compileExpressionPart();
			assert(";",__LINE__);
			t++;
		}
		int createIntegerLiteral(uint64_t integer)
		{
			int var=createVariable(Type("i64"));
			movRawImmediate(var,integer);
			functionContext.variables[var].type.isIntegerLiteral=true;
			return var;
		}
		int createNullptr()
		{
			int var=createVariable(Type("u8",1));
			movRawImmediate(var,0);
			functionContext.variables[var].type.isNullptr=true;
			return var;
		}
		int createString(const string& str)
		{
			vector<uint8_t> bytes(str.size()+1);
			for(size_t i=0;i<str.size();i++)
			{
				bytes[i]=str[i];
			}
			bytes.back()=0;
			
			int var=createVariable(Type("u8",1));
			movRawImmediateString(var,bytes);
			return var;
		}
		int getOffsetAndTypeOfAttribute(Type baseType,const string& attributeName,Type& attributeType)
		{
			int offset=0;
			
			ClassAttribute*attribute=findClassAttribute(baseType,attributeName);
			if(attribute==nullptr) return -1;
			
			attributeType=attribute->type;
			offset=attribute->objectOffset;
			
			return offset;
		}
		bool isAssignable(Type typel,Type typer)
		{
			if(typel.raw()==Type("dynamic") || typer.raw()==Type("dynamic")) return true;
			else if(typer.isNullptr) return typel.raw().isPointer();
			else if(typer.isIntegerLiteral) return isIntegerPrimitive(typel.raw());
			else return typel.raw()==typer.raw();
		}
		int getMaximumOperatorLevel()
		{
			return 100;
		}
		int getOperatorLevel(const string& s)
		{
			if(s=="*" || s=="/" || s=="%") return 10;
			else if(s=="+" || s=="-") return 9;
			else if(s=="<<" || s==">>") return 8;
			else if(s=="<" || s=="<=" || s==">" || s==">=") return 7;
			else if(s=="==" || s=="!=") return 6;
			else if(s=="&") return 5;
			else if(s=="^") return 4;
			else if(s=="|") return 3;
			else if(s=="&&") return 2;
			else if(s=="||") return 1;
			else return 0;
		}
		bool isComparisonOperator(const string& op)
		{
			return op=="==" || op=="!=" || op=="<" || op==">" || op=="<=" || op==">=";
		}
		int integerOperation(const string& op,Type type,Type outputType,int input_va,int input_vb)
		{
			if(getTypeOf(input_va).isReference) error(__LINE__);
			if(getTypeOf(input_vb).isReference) error(__LINE__);
			if(type.isReference || !isIntegerPrimitive(type)) error(__LINE__);
			if(outputType.isReference || !isIntegerPrimitive(outputType)) error(__LINE__);
			
			int size=getTypeSize(type.raw());
			bool isSigned=isSignedIntegerPrimitiveTypeName(type.name);
			
			int va=input_va;
			int vb=input_vb;
			if(size!=getTypeSize(getTypeOf(input_va)))
			{
				va=createVariable(type);
				movRaw(va,input_va);
			}
			if(size!=getTypeSize(getTypeOf(input_vb)))
			{
				vb=createVariable(type);
				movRaw(vb,input_vb);
			}
			
			string iasmOp;
			
			if(op=="*")
			{
				iasmOp="mul";
			}
			else if(op=="/")
			{
				if(isSigned) iasmOp="divs";
				else iasmOp="div";
			}
			else if(op=="%")
			{
				if(isSigned) iasmOp="mods";
				else iasmOp="mod";
			}
			else if(op=="+")
			{
				iasmOp="add";
			}
			else if(op=="-")
			{
				iasmOp="sub";
			}
			else if(op=="<<")
			{
				iasmOp="shl";
			}
			else if(op==">>")
			{
				if(isSigned) iasmOp="shrs";
				else iasmOp="shr";
			}
			else if(op=="<")
			{
				if(isSigned) iasmOp="cmpl";
				else iasmOp="cmpls";
			}
			else if(op=="<=")
			{
				if(isSigned) iasmOp="cmple";
				else iasmOp="cmples";
			}
			else if(op==">")
			{
				if(isSigned) iasmOp="cmpg";
				else iasmOp="cmpgs";
			}
			else if(op==">=")
			{
				if(isSigned) iasmOp="cmpge";
				else iasmOp="cmpges";
			}
			else if(op=="==")
			{
				iasmOp="cmpe";
			}
			else if(op=="!=")
			{
				iasmOp="cmpne";
			}
			else if(op=="&")
			{
				iasmOp="and";
			}
			else if(op=="^")
			{
				iasmOp="xor";
			}
			else if(op=="|")
			{
				iasmOp="or";
			}
			else if(op=="&&")
			{
				iasmOp="logand";
			}
			else if(op=="||")
			{
				iasmOp="logor";
			}
			else
			{
				error(__LINE__);
			}
			
			int vo=createVariable(outputType);
			
			//TODO put iasmOp instruction
			
			return vo;
		}
		int integerOperationLeftUnary(const string& op,Type type,Type outputType,int input_v)
		{
			if(getTypeOf(input_v).isReference) error(__LINE__);
			if(type.isReference || !isIntegerPrimitive(type)) error(__LINE__);
			if(outputType.isReference || !isIntegerPrimitive(outputType)) error(__LINE__);
			
			int size=getTypeSize(type);
			
			int v=input_v;
			if(size!=getTypeSize(getTypeOf(input_v)))
			{
				v=createVariable(type);
				movRaw(v,input_v);
			}
			
			string iasmOp;
			
			if(op=="-")
			{
				iasmOp="neg";
			}
			else if(op=="~")
			{
				iasmOp="not";
			}
			else if(op=="!")
			{
				iasmOp="lognot";
			}
			else
			{
				error(__LINE__);
			}
			
			int vo=createVariable(outputType);
			
			//TODO put iasmOp instruction
			
			return vo;
		}
		int compileOperator(int varl,int varr,const string& op)
		{
			int returnVar=-1;
			
			Type typel=getTypeOf(varl);
			Type typer=getTypeOf(varr);
			
			if(isIntegerPrimitive(typel.raw()) && isIntegerPrimitive(typer.raw()))
			{
				//two integers
				
				Type type=Type("i64");
				if(typel.isIntegerLiteral && typer.isIntegerLiteral) error(__LINE__);
				else if(typel.isIntegerLiteral) type=typer.raw();
				else if(typer.isIntegerLiteral) type=typel.raw();
				else
				{
					if(typel.raw()!=typer.raw()) error(__LINE__);
					type=typel.raw();
				}
				
				Type resultType=type;
				if(isComparisonOperator(op))
				{
					resultType=Type("u8");
				}
				
				returnVar=integerOperation(op,type,resultType,getValue(varl),getValue(varr));
			}
			else if(typel.isPointer() && typer.isPointer())
			{
				//two pointers
				
				if(typel.isNullptr && typer.isNullptr) error(__LINE__);
				
				if(!typel.isNullptr && !typer.isNullptr)
				{
					if(typel.raw()!=typer.raw()) error(__LINE__);
				}
				
				Type pointerType=(typel.isNullptr ? typer : typel).raw();
				
				Type pointedType=pointerType.getPointedType();
				
				if(op=="-")
				{
					Type type=Type("i64");
					Type resultType=Type("i64");
					
					int result=integerOperation(op,type,resultType,getValue(varl),getValue(varr));
					
					int sizeVar=createVariable(resultType);
					movRawImmediate(sizeVar,getTypeSize(pointedType.raw()));
					
					returnVar=integerOperation("/",resultType,resultType,result,sizeVar);
				}
				else if(op=="==" || op=="!=")
				{
					Type type=Type("i64");
					Type resultType=Type("u8");
					
					returnVar=integerOperation(op,type,resultType,getValue(varl),getValue(varr));
				}
				else error(__LINE__);
			}
			else if(typel.isPointer() && isIntegerPrimitive(typer.raw()))
			{
				//A pointer and an integer
				
				if(typel.isNullptr) error(__LINE__);
				
				if(op=="+" || op=="-")
				{
					Type type=Type("i64");
					Type resultType=Type("i64");
					Type finalType=typel.raw();
					
					Type pointedType=typel.getPointedType();
					
					int sizeVar=createVariable(resultType);
					movRawImmediate(sizeVar,getTypeSize(pointedType.raw()));
					
					int result=integerOperation("*",type,resultType,getValue(varr),sizeVar);
					
					int result2=integerOperation(op,resultType,resultType,getValue(varl),result);
					
					returnVar=createVariable(finalType);
					movRaw(returnVar,result2);
				}
				else error(__LINE__);
			}
			else
			{
				//Any other combination of types (overloaded operator)
				if(typel.raw()==Type("dynamic") || typer.raw()==Type("dynamic"))
				{
					//TODO----
					error(__LINE__);//----
				}
				else
				{
					returnVar=callFunction(FunctionInfo(-1,op,vector<Type>{typel,typer}),vector<int>{varl,varr},true);
				}
			}
			
			return returnVar;
		}
		int compileLeftUnaryOperator(int varin,const string& op)
		{
			int returnVar=-1;
			
			Type typein=getTypeOf(varin);
			
			if(isIntegerPrimitive(typein.raw()))
			{
				//An integer
				
				Type type=Type("i64");
				if(typein.isIntegerLiteral) error(__LINE__);
				else type=typein.raw();
				
				Type resultType=type;
				if(op=="!")
				{
					resultType=Type("u8");
				}
				
				returnVar=integerOperationLeftUnary(op,type,resultType,getValue(varin));
			}
			else
			{
				//Any other type (overloaded operator)
				//TODO----
				error(__LINE__);//----
			}
			
			return returnVar;
		}
		int castPrimitive(int varin,Type resultType)
		{
			if(resultType.isReference) error(__LINE__);
			
			Type typein=getTypeOf(varin);
			
			if(!(typein.isPointer() || isIntegerPrimitive(typein.raw()))
				|| !(resultType.isPointer() || isIntegerPrimitive(resultType.raw())))
			{
				error(__LINE__);
			}
			
			if(typein.isPointer() && resultType.isPointer()){}
			else if(typein.isPointer())
			{
				if(resultType.raw()!=Type("i64")) error(__LINE__);
			}
			else if(resultType.isPointer())
			{
				if(typein.raw()!=Type("i64")) error(__LINE__);
			}
			else if(!typein.isIntegerLiteral)
			{
				int fromSize=getTypeSize(typein.raw());
				int toSize=getTypeSize(resultType.raw());
				if(fromSize!=toSize)
				{
					bool sameSign=(isSignedIntegerPrimitiveTypeName(typein.name)==isSignedIntegerPrimitiveTypeName(resultType.name));
					if(!sameSign) error(__LINE__);
				}
			}
			
			int varout=createVariable(resultType);
			movRaw(varout,getValue(varin));
			
			return varout;
		}
		int castFromDynamic(int varin,Type type)
		{
			int varout=createVariable(type.getReference());
			
			callFunction(FunctionInfo(-1,"__dynamic_cast_to_type",vector<Type>{Type("i64",1),Type("i64",1),Type("i64")},false),
				vector<int>{getReferenceAsPtri64(varin),getReferenceAsPtri64(varout),getTypeVariable(type.raw())});
			
			return varout;
		}
		void assign(int varl,int varr)
		{
			Type typel=getTypeOf(varl);
			Type typer=getTypeOf(varr);
			
			if(typel.raw()==Type("dynamic")) callAssignmentOperator(varl,varr);
			else
			{
				if(isAssignable(typel,typer))
				{
					if(typel.isPointer() || isIntegerPrimitive(typel.raw()))
					{
						if(typer.raw()==Type("dynamic")) varr=castFromDynamic(varr,typel);
						
						if(typel.isReference) movRawSetDeref(varl,getValue(varr));
						else movRaw(varl,getValue(varr));
					}
					else callAssignmentOperator(varl,varr);
				}
				else error(__LINE__);
			}
		}
		void compileAssignmentOperatorUnary(int var,const string& op)
		{
			Type type=getTypeOf(var);
			
			if(type.isPointer() || isIntegerPrimitive(type.raw()))
			{
				int change=1;
				if(type.isPointer()) change=getTypeSize(type.getPointedType().raw());
				
				string iasmOp;
				if(op=="++") iasmOp="add";
				else if(op=="--") iasmOp="sub";
				else error(__LINE__);
				
				int tmp=getValue(var);
				int result=createVariable(type.raw());
				
				//TODO put iasmOp instruction with result, tmp and change (add/sub result,tmp,change)
				
				if(type.isReference) movRawSetDeref(var,result);
				else movRaw(var,result);
			}
			else
			{
				//TODO----
				error(__LINE__);//----
			}
		}
		void compileAssignmentOperator(int varl,int varr,const string& op)//TODO---- use the specific operators
		{
			string internalOp=op.substr(0,op.size()-1);
			
			int var=compileOperator(varl,varr,internalOp);
			
			assign(varl,var);
		}
		int compileFunctionCall(const string& functionName)
		{
			return compileFunctionCallRaw(functionName,-1,-1);
		}
		int compileMethodCall(const string& functionName,int thisVar)
		{
			int classIndex=findClassIndex(getTypeOf(thisVar).name);
			if(classIndex==-1) error(__LINE__);
			return compileFunctionCallRaw(functionName,classIndex,thisVar);
		}
		int compileFunctionCallRaw(const string& functionName,int classIndex,int thisVar)
		{
			int returnVar=-1;
			bool isMethod=(classIndex!=-1);
			
			assert("(",__LINE__);
			t++;
			
			vector<int> arguments;
			while(true)
			{
				if(code.get(t)==")") break;
				
				int arg=compileExpressionPart();
				if(arg==-1) error(__LINE__);
				arguments.push_back(arg);
				
				if(code.get(t)==",") t++;
				else assert(")",__LINE__);
			}
			vector<Type> argumentTypes;
			for(int i=0;i<arguments.size();i++)
			{
				argumentTypes.push_back(getTypeOf(arguments[i]));
			}
			
			assert(")",__LINE__);
			t++;
			
			if(isMethod) returnVar=callMethod(FunctionInfo(classIndex,functionName,argumentTypes),thisVar,arguments,true);
			else returnVar=callFunction(FunctionInfo(-1,functionName,argumentTypes),arguments,true);
			
			return returnVar;
		}
		int compileExpressionPart(int level=0)
		{
			int returnVar=-1;
			
			string start=code.get(t);
			
			if(start=="(")
			{
				t++;
				
				returnVar=compileExpressionPart();
				
				assert(")",__LINE__);
				t++;
			}
			else if(start=="addressof")
			{
				t++;
				
				assert("(",__LINE__);
				t++;
				
				int var=compileExpressionPart();
				if(var==-1) error(__LINE__);
				
				Type type=getTypeOf(var);
				Type resultType=getPointer(type);
				
				returnVar=createVariable(resultType);
				
				string registerToUse="rax";
				
				if(type.isReference) movRaw(returnVar,var);
				else error(__LINE__);
				
				assert(")",__LINE__);
				t++;
			}
			else if(start=="deref")
			{
				t++;
				
				assert("(",__LINE__);
				t++;
				
				int var=compileExpressionPart();
				if(var==-1) error(__LINE__);
				
				Type type=getTypeOf(var);
				
				if(!type.isPointer()) error(__LINE__);
				
				Type resultType=type.getPointedType();
				
				returnVar=createVariable(resultType.getReference());
				
				movRawGetDeref(returnVar,getValue(var));
				
				assert(")",__LINE__);
				t++;
			}
			else if(start=="sizeof" || start=="alignof" || start=="typeof")
			{
				t++;
				
				assert("(",__LINE__);
				t++;
				
				Type type;
				bool isType=parseCheckType(type);
				
				int var=-1;
				
				if(!isType)
				{
					var=compileExpressionPart();
					type=getTypeOf(var);
				}
				
				if(type.raw()==Type("dynamic") && !isType)
				{
					string functionName;
					if(start=="sizeof") functionName="__dynamic_sizeof";
					else if(start=="alignof") functionName="__dynamic_alignof";
					else if(start=="typeof") functionName="__dynamic_typeof";
					else error(__LINE__);
					
					returnVar=createVariable(Type("i64"));
					
					callFunction(FunctionInfo(-1,functionName,vector<Type>{Type("i64",1),Type("i64",1)},false),vector<int>{getReferenceAsPtri64(var),getReferenceAsPtri64(returnVar)});
				}
				else
				{
					int number=0;
					if(start=="sizeof")
					{
						if(type.isPointer()) number=pointerSize;
						else number=getTypeSize(type.raw());
					}
					else if(start=="alignof")
					{
						if(type.isPointer()) number=pointerAlignment;
						else number=getTypeAlignment(type.raw());
					}
					else if(start=="typeof")
					{
						number=getTypeInteger(type.raw());
					}
					
					returnVar=createVariable(Type("i64"));
					movRawImmediate(returnVar,number);
				}
				
				assert(")",__LINE__);
				t++;
			}
			else if(start=="__call" || start=="__call_destructor")
			{
				t++;
				
				assert("(",__LINE__);
				t++;
				
				vector<int> arguments;
				
				while(true)
				{
					if(code.get(t)==")") break;
					
					int arg=compileExpressionPart();
					if(arg==-1) error(__LINE__);
					
					arguments.push_back(arg);
					
					if(code.get(t)==",") t++;
					else assert(")",__LINE__);
				}
				if(arguments.size()==0) error(__LINE__);
				
				vector<int> args(arguments.begin()+1,arguments.end());
				
				callFunctionIndirect(arguments[0],args,-1,start=="__call_destructor");
				
				assert(")",__LINE__);
				t++;
			}
			else
			{
				Type startType;
				bool startIsType=parseCheckType(startType);
				
				if(startIsType)
				{
					if(code.get(t)!="(")
					{
						returnVar=getTypeVariable(startType);
					}
					else
					{
						assert("(",__LINE__);
						t++;
						
						if(startType.isReference) error(__LINE__);
						
						if(startType.isPointer() || isIntegerPrimitive(startType.raw()))
						{
							int var=compileExpressionPart();
							if(var==-1) error(__LINE__);
							
							Type type=getTypeOf(var);
							
							Type resultType=startType;
							
							if(type.raw()==Type("dynamic"))
							{
								Type typeToCastTo=resultType;
								if(typeToCastTo.isPointer())
								{
									typeToCastTo=Type("i64");
								}
								
								returnVar=createVariable(resultType);
								
								callFunction(FunctionInfo(-1,"__dynamic_cast_integer_to_integer",vector<Type>{Type("i64",1),Type("i64",1),Type("i64")},false),
									vector<int>{getReferenceAsPtri64(var),getReferenceAsPtri64(returnVar),getTypeVariable(resultType)});
							}
							else
							{
								if(!type.isPointer() && !isIntegerPrimitive(type.raw())) error(__LINE__);
								
								returnVar=castPrimitive(var,resultType);
							}
						}
						else
						{
							Type type=startType;
							
							returnVar=createVariableWithoutCallingConstructor(type);
							
							t--;
							rawBlockConstructorStart(returnVar);
							compileMethodCall("constructor",returnVar);
							rawBlockEnd();
							t--;
						}
						
						assert(")",__LINE__);
						t++;
					}
				}
				else if(start=="nullptr")
				{
					returnVar=createNullptr();
					t++;
				}
				else if(start=="false")
				{
					returnVar=createIntegerLiteral(0);
					t++;
				}
				else if(start=="true")
				{
					returnVar=createIntegerLiteral(1);
					t++;
				}
				else if(start=="__LINE__")
				{
					returnVar=createIntegerLiteral(code.tokens[t].line);
					t++;
				}
				else if(start=="__datatype_count")
				{
					returnVar=createVariable(Type("i64"));
					movRawImmediate(returnVar,dataTypes.size());
					t++;
				}
				else if(start=="__datatype_start")
				{
					returnVar=createVariable(Type("i64",1));
					movRawImmediate_datatypeStart(returnVar);
					t++;
				}
				else
				{
					bool isInteger=false;
					uint64_t integer=0;
					if(parseCheckIntegerToken(integer)) isInteger=true;
					else if(code.get(t)=="-")
					{
						t++;
						if(parseCheckIntegerToken(integer))
						{
							isInteger=true;
							integer=-integer;
						}
						else t--;
					}
					
					if(isInteger) returnVar=createIntegerLiteral(integer);
					else if(start=="-" || start=="!" || start=="~")
					{
						string op=start;
						t++;
						int var=compileExpressionPart(getMaximumOperatorLevel());
						if(var==-1) error(__LINE__);
						returnVar=compileLeftUnaryOperator(var,op);
					}
					else
					{
						string literal;
						bool isStringLiteral=parseCheckStringLiteralToken(literal);
						if(isStringLiteral)
						{
							int rawStringVar=createString(literal);
							
							string stringClassName="string";
							
							int classIndex=findClassIndex(stringClassName);
							if(classIndex==-1) error(__LINE__);
							
							returnVar=createVariableWithoutCallingConstructor(Type(stringClassName));
							
							rawBlockConstructorStart(returnVar);
							callMethod(FunctionInfo(classIndex,"constructor",vector<Type>{Type("u8",1)},false),returnVar,vector<int>{rawStringVar});
							rawBlockEnd();
						}
						else if(code.get(t)=="r")
						{
							t++;
							if(parseCheckStringLiteralToken(literal))
							{
								isStringLiteral=true;
								returnVar=createString(literal);
							}
							else t--;
						}
						else if(code.get(t)=="b")
						{
							t++;
							if(parseCheckStringLiteralToken(literal))
							{
								isStringLiteral=true;
								if(literal.size()==0) error(__LINE__);
								returnVar=createVariable(Type("u8"));
								movRawImmediate(returnVar,uint8_t(literal[0]));
							}
							else t--;
						}
						
						if(!isStringLiteral)
						{
							bool isFunctionCall=false;
							
							if(functionContext.isMethod)
							{
								int functionIndex=findMethodIndexFromName(functionContext.classIndex,start);
								if(functionIndex!=-1)
								{
									isFunctionCall=true;
									t++;
									returnVar=compileMethodCall(start,functionContext.variables.getIndexOf("this"));
								}
							}
							
							if(!isFunctionCall)
							{
								int functionIndex=findFunctionIndexFromName(start);
								if(functionIndex!=-1)
								{
									isFunctionCall=true;
									t++;
									returnVar=compileFunctionCall(start);
								}
							}
							
							
							if(!isFunctionCall)
							{
								string variableName=start;
								
								try
								{
									returnVar=getReference(functionContext.variables.getIndexOf(variableName));
								}
								catch(...)
								{
									int globalVariableIndex=findGlobalVariableIndex(variableName);
									if(globalVariableIndex!=-1)
									{
										returnVar=createVariable(globalVariables[globalVariableIndex].type.getReference());
										movRawGetGlobalAddress(returnVar,globalVariableIndex);
									}
								}
								
								if(returnVar==-1)
								{
									if(functionContext.isMethod)
									{
										ClassAttribute*thisClassAttribute=findClassAttribute(Type(classes[functionContext.classIndex].name,0),variableName);
										if(thisClassAttribute!=nullptr)
										{
											returnVar=getAttributeReference(functionContext.variables.getIndexOf("this"),*thisClassAttribute);
										}
									}
								}
								
								if(returnVar==-1) error(__LINE__);
								t++;
							}
						}
					}
				}
			}
			
			while(true)
			{
				string token=code.get(t);
				if(token==";" || token=="," || token==")" || token=="]" || token=="}") break;
				
				if(returnVar==-1) error(__LINE__);
				
				if(token==".")
				{
					t++;
					string attribute=code.getIdentifier(t,__LINE__);
					t++;
					
					Type type=getTypeOf(returnVar);
					
					if(type.raw()==Type("dynamic"))
					{
						int identifierIndex=getIdentifierIndex(attribute);
						if(identifierIndex==-1) error(__LINE__);
						
						if(code.get(t)=="(")
						{
							vector<int> args;
							error(__LINE__);//----
							//TODO--------
						}
						else
						{
							int outputVar=createVariable(Type("dynamic"));
							
							int identifierVar=createVariable(Type("i64"));
							movRawImmediate(identifierVar,identifierIndex);
							
							callFunction(FunctionInfo(-1,"__dynamic_get_attribute",vector<Type>{Type("i64",1),Type("i64",1),Type("i64")},false),
								vector<int>{getReferenceAsPtri64(returnVar),getReferenceAsPtri64(outputVar),identifierVar});
							
							returnVar=outputVar;
						}
					}
					else
					{
						ClassAttribute*attrPtr=findClassAttribute(type,attribute);
						if(attrPtr!=nullptr) returnVar=getAttributeReference(returnVar,*attrPtr);
						else returnVar=compileMethodCall(attribute,returnVar);
					}
				}
				else if(token=="[")
				{
					t++;
					
					Type type=getTypeOf(returnVar);
					
					if(type.isPointer())
					{
						int var=compileExpressionPart();
						if(var==-1) error(__LINE__);
						Type varType=getTypeOf(var);
						if(!isIntegerPrimitive(varType.raw())) error(__LINE__);
						
						int address=compileOperator(returnVar,var,"+");
						
						Type resultType=type.getPointedType();
						
						int result=createVariable(resultType.getReference());
						movRaw(result,address);
						
						returnVar=result;
					}
					else if(!isIntegerPrimitive(type.raw()))
					{
						if(type.raw()==Type("dynamic"))
						{
							//TODO----
							error(__LINE__);//----
						}
						else
						{
							int arg=compileExpressionPart();
							if(arg==-1) error(__LINE__);
							
							returnVar=callMethod(FunctionInfo(findClassIndex(type.name),"[]",vector<Type>{getTypeOf(arg)}),returnVar,vector<int>{arg},true);
						}
					}
					else error(__LINE__);
					
					assert("]",__LINE__);
					t++;
				}
				else if(token=="=")
				{
					t++;
					if(code.get(t)=="=")
					{
						int operatorLevel=getOperatorLevel("==");
						if(operatorLevel>level)
						{
							t++;
							int var=compileExpressionPart(operatorLevel);
							if(var==-1) error(__LINE__);
							returnVar=compileOperator(returnVar,var,"==");
						}
						else
						{
							t--;
							break;
						}
					}
					else
					{
						int var=compileExpressionPart();
						if(var==-1) error(__LINE__);
						
						assign(returnVar,var);
						
						returnVar=-1;
					}
				}
				else
				{
					bool isOperator=false;
					int operatorTokens=0;
					string op;
					bool isAssignment=false;
					bool isUnaryAssignment=false;
					
					if(token=="*" || token=="/" || token=="%" || token=="+" || token=="-" || token=="^")
					{
						if((token=="+" || token=="-") && code.get(t+1)==token)
						{
							isOperator=true;
							operatorTokens=2;
							op=token+token;
							isAssignment=true;
							isUnaryAssignment=true;
						}
						else if(code.get(t+1)=="=")
						{
							isOperator=true;
							operatorTokens=2;
							op=token+"=";
							isAssignment=true;
						}
						else
						{
							isOperator=true;
							operatorTokens=1;
							op=token;
						}
					}
					else if(token=="<")
					{
						if(code.get(t+1)=="<")
						{
							if(code.get(t+2)=="=")
							{
								isOperator=true;
								operatorTokens=3;
								op="<<=";
								isAssignment=true;
							}
							else
							{
								isOperator=true;
								operatorTokens=2;
								op="<<";
							}
						}
						else if(code.get(t+1)=="=")
						{
							isOperator=true;
							operatorTokens=2;
							op="<=";
						}
						else
						{
							isOperator=true;
							operatorTokens=1;
							op=token;
						}
					}
					else if(token==">")
					{
						if(code.get(t+1)==">")
						{
							if(code.get(t+2)=="=")
							{
								isOperator=true;
								operatorTokens=3;
								op=">>=";
								isAssignment=true;
							}
							else
							{
								isOperator=true;
								operatorTokens=2;
								op=">>";
							}
						}
						else if(code.get(t+1)=="=")
						{
							isOperator=true;
							operatorTokens=2;
							op=">=";
						}
						else
						{
							isOperator=true;
							operatorTokens=1;
							op=token;
						}
					}
					else if(token=="!")
					{
						if(code.get(t+1)=="=")
						{
							isOperator=true;
							operatorTokens=2;
							op="!=";
						}
					}
					else if(token=="&")
					{
						if(code.get(t+1)=="&")
						{
							isOperator=true;
							operatorTokens=2;
							op="&&";
						}
						else if(code.get(t+1)=="=")
						{
							isOperator=true;
							operatorTokens=2;
							op="&=";
							isAssignment=true;
						}
						else
						{
							isOperator=true;
							operatorTokens=1;
							op=token;
						}
					}
					else if(token=="|")
					{
						if(code.get(t+1)=="|")
						{
							isOperator=true;
							operatorTokens=2;
							op="||";
						}
						else if(code.get(t+1)=="=")
						{
							isOperator=true;
							operatorTokens=2;
							op="|=";
							isAssignment=true;
						}
						else
						{
							isOperator=true;
							operatorTokens=1;
							op=token;
						}
					}
					
					if(isOperator)
					{
						if(isAssignment)
						{
							t+=operatorTokens;
							
							if(isUnaryAssignment)
							{
								compileAssignmentOperatorUnary(returnVar,op);
							}
							else
							{
								int var=compileExpressionPart();
								if(var==-1) error(__LINE__);
								compileAssignmentOperator(returnVar,var,op);
							}
							
							returnVar=-1;
						}
						else
						{
							int operatorLevel=getOperatorLevel(op);
							if(operatorLevel>level)
							{
								t+=operatorTokens;
								int var=compileExpressionPart(operatorLevel);
								returnVar=compileOperator(returnVar,var,op);
							}
							else break;
						}
					}
					else error(__LINE__);
				}
			}
			
			return returnVar;
		}
	public:
};



class Compiler
{
	public:
	
	string compile(const string& inputCode)
	{
		TokenizedCode tokCode(inputCode);
		
		Code code;
		try
		{
			code.parse(tokCode);
			if(code.code.hasErrors())
			{
				code.code.printErrors();
				return "";
			}
			
			code.processParsedData();
			if(code.code.hasErrors())
			{
				code.code.printErrors();
				return "";
			}
			
			string outputCode=code.compile();
			if(code.code.hasErrors())
			{
				code.code.printErrors();
				return "";
			}
			
			return outputCode;
		}
		catch(Error e)
		{
			if(code.code.hasErrors()) code.code.printErrors();
			else cout<<"FATAL ERROR: "<<e.message<<endl;
			return "";
		}
	}
};

int main(int argc,char*argv[])
{
	try
	{
	
	if(argc!=3)
	{
		throw string("Expected 2 arguments. Example: './bolgegc code.bc code.asm'");
	}
	
	Compiler compiler;
	
	string code=fileToString(argv[1]);
	
	string assembly=compiler.compile(code);
	
	stringToFile(assembly,argv[2]);
	
	}catch(const string& str)
	{
		cout<<str<<endl;
	}
	
	return 0;
}

