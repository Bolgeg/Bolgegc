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


class ParseError
{
	public:
};

class NameError
{
	public:
};

class LogicError
{
	public:
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
	
	void addError(size_t tokenIndex,int errorCode)
	{
		if(tokenIndex<tokens.size())
		{
			errors.emplace_back(string("Error:")+to_string(tokens[tokenIndex].line)+":"+to_string(tokens[tokenIndex].column)+": ERROR("+to_string(errorCode)+")");
		}
		else
		{
			errors.emplace_back(string("Error:?:?: ERROR(")+to_string(errorCode)+")");
		}
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
			addError(tokenIndex,__LINE__);
			throw ParseError();
		}
		else
		{
			return tokens[tokenIndex].content;
		}
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
			addError(tokenIndex,__LINE__);
			throw ParseError();
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
			addError(tokenIndex,__LINE__);
			throw ParseError();
		}
	}
};



template <class T>
class NamedVector
{
	private:
	
	vector<T> elements;
	map<string,size_t> nameToIndex;
	
	public:
	
	bool add(const T& newElement)
	{
		if(contains(newElement.name))
		{
			return false;
		}
		
		elements.push_back(newElement);
		nameToIndex[newElement.name]=elements.size()-1;
		
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
				mapContent+=elements[i].name+"\n";
			}
			throw string("FATAL ERROR - Exception on NamedVector's map::at(\""+to_string(index)+"\"). NamedVector content:\n"+mapContent);
		}
	}
	private:
		string nameMapAtError(const string& name)
		{
			string mapContent;
			for(int i=0;i<elements.size();i++)
			{
				mapContent+=elements[i].name+"\n";
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


class DataType
{
	public:
	
	string name;
	
	bool isPrimitive=false;
	size_t size=0;
	size_t alignment=1;
	
	bool sizeAndAlignmentDefined=false;
	
	bool isPassedByReference=false;
	bool isInteger=false;
	
	DataType(){}
	DataType(const string& _name,bool _isPrimitive=false,bool _isInteger=false,bool _sizeAndAlignmentDefined=false,size_t _size=0,size_t _alignment=1)
	{
		name=_name;
		isPrimitive=_isPrimitive;
		isInteger=_isInteger;
		sizeAndAlignmentDefined=_sizeAndAlignmentDefined;
		size=_size;
		alignment=_alignment;
		
		isPassedByReference=!isPrimitive;
	}
};



class Type
{
	public:
	
	string name="u8";
	
	int pointerLevels=0;
	
	Type(){}
	Type(const string& _name,int _pointerLevels)
	{
		name=_name;
		pointerLevels=_pointerLevels;
	}
	Type(TokenizedCode& code,size_t& t,bool addErrors=true)
	{
		pointerLevels=0;
		while(true)
		{
			string str=code.get(t);
			
			if(str=="ptr")
			{
				pointerLevels++;
				t++;
				if(code.get(t)!="(")
				{
					if(addErrors) code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			else
			{
				name=str;
				t++;
				break;
			}
		}
		for(int i=0;i<pointerLevels;i++)
		{
			if(code.get(t)!=")")
			{
				if(addErrors) code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
		}
	}
	
	string toString()
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
		
		return str;
	}
};

class CodeTopFunctionParameter
{
	public:
	
	Type type;
	string name;
	
	CodeTopFunctionParameter(){}
	CodeTopFunctionParameter(const Type& _type,const string& _name)
	{
		type=_type;
		name=_name;
	}
	
	string toString()
	{
		return type.toString()+" "+name;
	}
};

class CodeTopFunction
{
	public:
	
	size_t tokenIndex=0;
	
	bool isOperator=false;
	string name;
	bool returnsSomething=false;
	Type returnType;
	bool returnsReference=false;
	vector<CodeTopFunctionParameter> parameters;
	
	bool compilerGenerated=false;
	
	size_t codeStart=0;
	size_t codeEnd=0;
	
	CodeTopFunction(){}
	CodeTopFunction(bool _isOperator,const string& _name,bool _returnsSomething,const Type& _returnType,const vector<CodeTopFunctionParameter>& _parameters,bool _compilerGenerated)
	{
		isOperator=_isOperator;
		name=_name;
		returnsSomething=_returnsSomething;
		returnType=_returnType;
		parameters=_parameters;
		
		compilerGenerated=_compilerGenerated;
	}
	CodeTopFunction(TokenizedCode& code,size_t& t)
	{
		tokenIndex=t;
		
		if(code.get(t)!="fn")
		{
			code.addError(t,__LINE__);
			throw ParseError();
		}
		t++;
		
		if(code.get(t)=="operator")
		{
			isOperator=true;
			t++;
			
			if(code.get(t)!="(")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
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
			name=code.get(t);
			t++;
		}
		
		if(code.get(t)!="(")
		{
			code.addError(t,__LINE__);
			throw ParseError();
		}
		t++;
		
		if(code.get(t)==")")
		{
			returnsSomething=false;
			t++;
		}
		else
		{
			returnsSomething=true;
			if(code.get(t)=="ref")
			{
				t++;
				returnsReference=true;
				if(code.get(t)!="(")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				returnType=Type(code,t);
				
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			else
			{
				returnType=Type(code,t);
			}
			
			if(code.get(t)!=")")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
		}
		
		if(code.get(t)!="(")
		{
			code.addError(t,__LINE__);
			throw ParseError();
		}
		t++;
		
		if(code.get(t)==")")
		{
			t++;
		}
		else
		{
			while(true)
			{
				parameters.emplace_back();
				
				parameters.back().type=Type(code,t);
				parameters.back().name=code.get(t);
				t++;
				
				if(code.get(t)==")")
				{
					t++;
					break;
				}
				else
				{
					if(code.get(t)!=",")
					{
						code.addError(t,__LINE__);
						throw ParseError();
					}
					t++;
				}
			}
		}
		
		if(code.get(t)!="{")
		{
			code.addError(t,__LINE__);
			throw ParseError();
		}
		codeStart=t;
		t=code.findMatchingBracket(t);
		codeEnd=t;
		t++;
	}
	
	string toString()
	{
		string str="fn ";
		
		if(isOperator) str+=string("operator(")+name+")";
		else str+=name;
		
		if(returnsSomething)
		{
			if(returnsReference) str+=string("(ref(")+returnType.toString()+"))";
			else str+=string("(")+returnType.toString()+")";
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

class CodeTopClassAttribute
{
	public:
	
	size_t tokenIndex=0;
	
	Type type;
	string name;
	
	size_t objectOffset=0;
	
	CodeTopClassAttribute(){}
	CodeTopClassAttribute(TokenizedCode& code,size_t& t)
	{
		tokenIndex=t;
		
		type=Type(code,t);
		name=code.get(t);
		t++;
		if(code.get(t)!=";")
		{
			code.addError(t,__LINE__);
			throw ParseError();
		}
		t++;
	}
	
	string toString()
	{
		return type.toString()+" "+name+";"+" //offset="+to_string(objectOffset);
	}
};

class CodeTopClass
{
	public:
	
	size_t tokenIndex=0;
	
	string name;
	vector<CodeTopClassAttribute> attributes;
	vector<CodeTopFunction> methods;
	
	size_t objectSize=0;
	size_t objectAlignment=1;
	
	CodeTopClass(){}
	CodeTopClass(TokenizedCode& code,size_t& t)
	{
		tokenIndex=t;
		
		if(code.get(t)!="class")
		{
			code.addError(t,__LINE__);
			throw ParseError();
		}
		t++;
		
		name=code.get(t);
		t++;
		
		if(code.get(t)!="{")
		{
			code.addError(t,__LINE__);
			throw ParseError();
		}
		t++;
		
		while(code.get(t)!="}")
		{
			string str=code.get(t);
			
			if(str=="fn")
			{
				methods.emplace_back(code,t);
			}
			else
			{
				attributes.emplace_back(code,t);
			}
		}
		
		if(code.get(t)!="}")
		{
			code.addError(t,__LINE__);
			throw ParseError();
		}
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

class CodeTopGlobalVariable
{
	public:
	
	size_t tokenIndex=0;
	
	Type type;
	string name;
	
	CodeTopGlobalVariable(){}
	CodeTopGlobalVariable(TokenizedCode& code,size_t& t)
	{
		tokenIndex=t;
		
		type=Type(code,t);
		name=code.get(t);
		t++;
		if(code.get(t)!=";")
		{
			code.addError(t,__LINE__);
			throw ParseError();
		}
		t++;
	}
	
	string toString()
	{
		return type.toString()+" "+name+";";
	}
};

class AssemblyCode
{
	public:
	
	string output;
	
	string getOutput()
	{
		return output;
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

class LocalVariable
{
	public:
	
	Type type;
	bool isReference=false;
	string name;
	
	int stackOffset=0;
	
	LocalVariable(){}
	LocalVariable(const Type& _type,bool _isReference,const string& _name)
	{
		type=_type;
		isReference=_isReference;
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
	
	int localIndexStart=0;
	
	int continueLabel=0;
	int breakLabel=0;
	
	LoopContext(){}
	LoopContext(int _localIndexStart,int _continueLabel,int _breakLabel)
	{
		inLoop=true;
		localIndexStart=_localIndexStart;
		continueLabel=_continueLabel;
		breakLabel=_breakLabel;
	}
};

class TryContext
{
	public:
	
	bool inTry=false;
	
	int localIndexStart=0;
	
	int catchLabel=0;
	
	TryContext(){}
	TryContext(int _localIndexStart,int _catchLabel)
	{
		inTry=true;
		localIndexStart=_localIndexStart;
		catchLabel=_catchLabel;
	}
};

class FunctionContext
{
	public:
	
	int classIndex=-1;
	int functionIndex=0;
	
	bool isMethod=false;
	
	size_t variableStack=0;
	size_t variableStackMax=0;
	size_t parameterStack=0;
	
	NamedVector<LocalVariable> baseLocalVariables;
	int namesCreated=0;
	
	int indexOfVariableToReturn=-1;
	
	int indexOfVariableToThrow=-1;
	
	int functionLocalIndexStart=0;
	int scopeLocalIndexStart=0;
	
	int labelsCreated=0;
	
	LoopContext loopContext;
	
	TryContext tryContext;
	
	int enterScope(int newScopeLocalIndexStart)
	{
		int outScopeLocalIndexStart=scopeLocalIndexStart;
		
		scopeLocalIndexStart=newScopeLocalIndexStart;
		
		return outScopeLocalIndexStart;
	}
	void exitScope(int outScopeLocalIndexStart)
	{
		scopeLocalIndexStart=outScopeLocalIndexStart;
	}
	LoopContext startLoop(const LoopContext& newLoopContext)
	{
		LoopContext oldLoopContext=loopContext;
		loopContext=newLoopContext;
		return oldLoopContext;
	}
	void endLoop(const LoopContext& oldLoopContext)
	{
		loopContext=oldLoopContext;
	}
	TryContext startTry(const TryContext& newTryContext)
	{
		TryContext oldTryContext=tryContext;
		tryContext=newTryContext;
		return oldTryContext;
	}
	void endTry(const TryContext& oldTryContext)
	{
		tryContext=oldTryContext;
	}
};

class ExpressionValue
{
	public:
	
	int localVariableIndex=-1;
	
	bool isNullptr=false;
	
	bool isIntegerLiteral=false;
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

class CodeTop
{
	public:
	
	vector<CodeTopFunction> functions;
	vector<CodeTopClass> classes;
	vector<CodeTopGlobalVariable> globalVariables;
	
	NamedVector<DataType> dataTypes;
	int dataTypesPassByReferenceStartIndex=0;
	size_t pointerSize=8;
	size_t pointerAlignment=8;
	
	NamedVector<CodeIdentifier> codeIdentifiers;
	
	CodeAssembly assembly;
	
	
	MachineRegisters machineRegisters;
	string returnRegister="rax";
	vector<string> parameterRegisters=vector<string>{"rdi","rsi","rdx","rcx","r8","r9"};
	
	void initialize()
	{
		machineRegisters.initialize();
	}
	
	CodeTop(TokenizedCode& code)
	{
		initialize();
		
		size_t t=0;
		while(code.contains(t))
		{
			string str=code.get(t);
			
			if(str=="fn")
			{
				functions.emplace_back(code,t);
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
		void processParsedDataCheckFunction(TokenizedCode& code,CodeTopFunction& function)
		{
			if(function.returnsSomething)
			{
				if(!typeExists(function.returnType))
				{
					code.addError(function.tokenIndex,__LINE__);
					throw NameError();
				}
			}
			for(int i=0;i<function.parameters.size();i++)
			{
				if(!typeExists(function.parameters[i].type))
				{
					code.addError(function.tokenIndex,__LINE__);
					throw NameError();
				}
			}
		}
	public:
	void processParsedData(TokenizedCode& code)
	{
		//Keep integer types in this same position and order
		dataTypes.add(DataType("u8",true,true,true,1,1));//Position 0
		dataTypes.add(DataType("u16",true,true,true,2,2));//Position 1
		dataTypes.add(DataType("u32",true,true,true,4,4));//Position 2
		dataTypes.add(DataType("u64",true,true,true,8,8));//Position 3
		dataTypes.add(DataType("i8",true,true,true,1,1));//Position 4
		dataTypes.add(DataType("i16",true,true,true,2,2));//Position 5
		dataTypes.add(DataType("i32",true,true,true,4,4));//Position 6
		dataTypes.add(DataType("i64",true,true,true,8,8));//Position 7
		
		dataTypesPassByReferenceStartIndex=dataTypes.size();
		
		dataTypes.add(DataType("dynamic",false,false,true,pointerSize,pointerAlignment));
		
		for(int i=0;i<classes.size();i++)
		{
			if(!dataTypes.add(DataType(classes[i].name)))
			{
				code.addError(classes[i].tokenIndex,__LINE__);
				throw NameError();
			}
		}
		
		{
			for(int i=0;i<classes.size();i++)
			{
				for(int j=0;j<classes[i].attributes.size();j++)
				{
					if(!typeExists(classes[i].attributes[j].type))
					{
						code.addError(classes[i].attributes[j].tokenIndex,__LINE__);
						throw NameError();
					}
				}
				for(int j=0;j<classes[i].methods.size();j++)
				{
					processParsedDataCheckFunction(code,classes[i].methods[j]);
				}
			}
			
			for(int i=0;i<functions.size();i++)
			{
				processParsedDataCheckFunction(code,functions[i]);
			}
			
			for(int i=0;i<globalVariables.size();i++)
			{
				if(!typeExists(globalVariables[i].type))
				{
					code.addError(globalVariables[i].tokenIndex,__LINE__);
					throw NameError();
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
					if(defineClass(i,code))
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
				code.addError(-1,__LINE__);
				throw LogicError();
			}
			classesDefined=newClassesDefined;
		}
	}
	private:
		bool defineClass(int classIndex,TokenizedCode& code)
		{
			CodeTopClass& c=classes[classIndex];
			
			c.objectSize=0;
			c.objectAlignment=1;
			
			size_t offset=0;
			
			for(int i=0;i<c.attributes.size();i++)
			{
				CodeTopClassAttribute& a=c.attributes[i];
				
				size_t aSize=0;
				size_t aAlignment=1;
				
				if(a.type.pointerLevels>0)
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
						code.addError(a.tokenIndex,__LINE__);
						throw LogicError();
					}
				}
				
				if(aAlignment==0)
				{
					code.addError(a.tokenIndex,__LINE__);
					throw LogicError();
				}
				size_t m=offset%aAlignment;
				if(m!=0) offset+=aAlignment-m;
				
				a.objectOffset=offset;
				
				offset+=aSize;
				
				if(aAlignment>c.objectAlignment) c.objectAlignment=aAlignment;
			}
			
			size_t m=offset%c.objectAlignment;
			if(m!=0) offset+=c.objectAlignment-m;
			
			c.objectSize=offset;
			
			return true;
		}
		string getAssemblyFunctionName(int functionIndex)
		{
			return string("F")+to_string(functionIndex);
		}
		string getAssemblyEmptyFunctionName()
		{
			return "F_EMPTY";
		}
		string getAssemblyIntegerCopyFunctionName(int size)
		{
			return string("F_INTEGER_COPY_")+to_string(size*8);
		}
		string getAssemblyMethodName(int classIndex,int methodIndex)
		{
			return string("C")+to_string(classIndex)+"F"+to_string(methodIndex);
		}
		string getAssemblyGlobalVariableName(int globalVariableIndex)
		{
			return string("GV")+to_string(globalVariableIndex);
		}
		string getAssemblyStringName(int stringIndex)
		{
			return string("S")+to_string(stringIndex);
		}
		string getAssemblyLabelName(int labelIndex)
		{
			return string(".L")+to_string(labelIndex);
		}
		string getAssemblyTypeLabelName(int labelIndex)
		{
			return string("T")+to_string(labelIndex);
		}
		void getTypeSizeAndAlignment(const Type& type,size_t& size,size_t& alignment,bool isReference)
		{
			if(type.pointerLevels>0 || isReference)
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
		size_t getTypeSize(const Type& type,bool isReference)
		{
			size_t size;
			size_t alignment;
			getTypeSizeAndAlignment(type,size,alignment,isReference);
			return size;
		}
		size_t getTypeAlignment(const Type& type,bool isReference)
		{
			size_t size;
			size_t alignment;
			getTypeSizeAndAlignment(type,size,alignment,isReference);
			return alignment;
		}
		int findFunctionIndex(const string& name)
		{
			for(int i=0;i<functions.size();i++)
			{
				if(functions[i].name==name)
				{
					return i;
				}
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
		int findMethodIndex(int classIndex,const string& methodName)
		{
			if(methodName=="constructor")
			{
				return findEmptyConstructorIndex(classIndex);
			}
			else
			{
				for(int i=0;i<classes[classIndex].methods.size();i++)
				{
					if(classes[classIndex].methods[i].name==methodName)
					{
						return i;
					}
				}
				return -1;
			}
		}
		int findEmptyConstructorIndex(int classIndex)
		{
			string methodName="constructor";
			for(int i=0;i<classes[classIndex].methods.size();i++)
			{
				if(classes[classIndex].methods[i].name==methodName)
				{
					if(classes[classIndex].methods[i].parameters.size()==0)
					{
						return i;
					}
				}
			}
			return -1;
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
		CodeTopClassAttribute*findClassAttribute(const Type& type,const string& name)
		{
			if(type.pointerLevels>0) return nullptr;
			
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
		void compileIdentifierData()
		{
			assembly.functionCode+=getAssemblyEmptyFunctionName()+":\n";
			assembly.functionCode+="clc\n";
			assembly.functionCode+="ret\n\n";
			
			for(int i=0;i<4;i++)
			{
				int size=(1<<i);
				assembly.functionCode+=getAssemblyIntegerCopyFunctionName(size)+":\n";
				assembly.functionCode+=string("mov [")+parameterRegisters[0]+"],"+getSizedRegister(parameterRegisters[1],size)+"\n";
				assembly.functionCode+="clc\n";
				assembly.functionCode+="ret\n\n";
			}
			
			
			assembly.identifierDataCode+="\n\n";
			
			assembly.identifierDataCode+="datatype_count:\n";
			assembly.identifierDataCode+=string("dq ")+to_string(dataTypes.size())+"\n";
			
			assembly.identifierDataCode+="datatype_start:\n";
			for(int i=0;i<dataTypes.size();i++)
			{
				assembly.identifierDataCode+=string("dq ")+getAssemblyTypeLabelName(i)+"\n";
			}
			
			assembly.identifierDataCode+="\n";
			
			for(int i=0;i<dataTypes.size();i++)
			{
				assembly.identifierDataCode+=compileIdentifierDataOfType(i);
			}
		}
		string compileIdentifierDataOfType(int typeIndex)
		{
			string output;
			
			output+=getAssemblyTypeLabelName(typeIndex)+":\n";
			
			DataType& dataType=dataTypes[typeIndex];
			
			output+=string("dq ")+to_string(dataType.size)+"\n";
			output+=string("dq ")+to_string(dataType.alignment)+"\n";
			
			if(dataType.isInteger)
			{
				output+=string("dq ")+getAssemblyEmptyFunctionName()+"\n";
				output+=string("dq ")+getAssemblyEmptyFunctionName()+"\n";
				output+=string("dq ")+getAssemblyIntegerCopyFunctionName(dataType.size)+"\n";
				
				output+="dq 0\n";
				output+="dq 0\n";
				
				output+="dq 0\n";
				output+="dq 0\n";
			}
			else if(dataType.name=="dynamic")
			{
				int constructorIndex=findFunctionIndex("__dynamic_constructor");
				int destructorIndex=findFunctionIndex("__dynamic_destructor");
				int assignmentIndex=findFunctionIndex("__dynamic_assignment_operator");
				
				output+=string("dq ")+getAssemblyFunctionName(constructorIndex)+"\n";
				output+=string("dq ")+getAssemblyFunctionName(destructorIndex)+"\n";
				output+=string("dq ")+getAssemblyFunctionName(assignmentIndex)+"\n";
				
				output+="dq 0\n";
				output+="dq 0\n";
				
				output+="dq 0\n";
				output+="dq 0\n";
			}
			else
			{
				output+=compileIdentifierDataOfClass(findClassIndex(dataType.name));
			}
			
			return output;
		}
		string compileIdentifierDataOfClass(int classIndex)
		{
			string output;
			
			CodeTopClass& c=classes[classIndex];
			
			int constructorIndex=findMethodIndex(classIndex,"constructor");
			int destructorIndex=findMethodIndex(classIndex,"destructor");
			int assignmentIndex=findMethodIndex(classIndex,"=");
			
			output+=string("dq ")+getAssemblyMethodName(classIndex,constructorIndex)+"\n";
			output+=string("dq ")+getAssemblyMethodName(classIndex,destructorIndex)+"\n";
			output+=string("dq ")+getAssemblyMethodName(classIndex,assignmentIndex)+"\n";
			
			string outputAttributes;
			int attributeCount=0;
			for(int i=0;i<c.attributes.size();i++)
			{
				Type type=c.attributes[i].type;
				if(type.pointerLevels==0)
				{
					int identifierIndex=createIdentifier(c.attributes[i].name);
					int typeIndex=dataTypes.getIndexOf(type.name);
					int offset=c.attributes[i].objectOffset;
					
					outputAttributes+=string("dq ")+to_string(identifierIndex)+"\n";
					outputAttributes+=string("dq ")+to_string(typeIndex)+"\n";
					outputAttributes+=string("dq ")+to_string(offset)+"\n";
					outputAttributes+="dq 0\n";
					
					attributeCount++;
				}
			}
			output+=string("dq ")+to_string(attributeCount)+"\n";
			output+="dq .attribute_list\n";
			
			string outputMethods;
			vector<string> outputMethodParameters;
			int methodCount=0;
			for(int i=0;i<c.methods.size();i++)
			{
				CodeTopFunction function=c.methods[i];
				if(function.name=="constructor" && function.parameters.size()>0) continue;
				if(function.isOperator) continue;
				if(function.returnsSomething)
				{
					if(function.returnType.pointerLevels>0) continue;
					if(function.returnsReference) continue;
				}
				
				if(function.returnsSomething) continue;//TODO---- NOT IMPLEMENTED
				
				bool parameterError=false;
				string outputParameters;
				for(int j=0;j<function.parameters.size();j++)
				{
					Type type=function.parameters[j].type;
					if(type.pointerLevels>0)
					{
						parameterError=true;
						break;
					}
					else
					{
						outputParameters+=string("dq ")+to_string(dataTypes.getIndexOf(type.name))+"\n";
					}
				}
				if(parameterError) continue;
				
				{
					int identifierIndex=createIdentifier(function.name);
					int returnTypeIndex=-1;
					if(function.returnsSomething) returnTypeIndex=dataTypes.getIndexOf(function.returnType.name);
					
					outputMethods+=string("dq ")+to_string(identifierIndex)+"\n";
					outputMethods+=string("dq ")+getAssemblyMethodName(classIndex,i)+"\n";
					outputMethods+=string("dq ")+to_string(returnTypeIndex)+"\n";
					outputMethods+=string("dq ")+to_string(function.parameters.size())+"\n";
					outputMethods+=string("dq ")+".f"+to_string(methodCount)+"p"+"\n";
					outputMethods+="dq 0\n";
					outputMethods+="dq 0\n";
					outputMethods+="dq 0\n";
					
					outputMethodParameters.push_back(outputParameters);
					
					methodCount++;
				}
			}
			output+=string("dq ")+to_string(methodCount)+"\n";
			output+="dq .method_list\n";
			
			
			output+=".attribute_list:\n";
			output+=outputAttributes;
			
			output+=".method_list:\n";
			output+=outputMethods;
			for(int i=0;i<outputMethodParameters.size();i++)
			{
				output+=string(".f")+to_string(i)+"p"+":\n";
				output+=outputMethodParameters[i];
			}
			
			
			return output;
		}
	public:
	void compile(TokenizedCode& code,AssemblyCode& assemblyCode)
	{
		for(int i=0;i<globalVariables.size();i++)
		{
			size_t size=0;
			size_t alignment=0;
			getTypeSizeAndAlignment(globalVariables[i].type,size,alignment,false);
			
			int m=assembly.bssSize%alignment;
			if(m!=0)
			{
				m=alignment-m;
				string str=string("resb ")+to_string(m)+"\n";
				assembly.globalVariableCode+=str;
			}
			
			string str=getAssemblyGlobalVariableName(i)+": resb "+to_string(size)+"\n";
			assembly.bssSize+=size;
			assembly.globalVariableCode+=str;
		}
		
		addDefaultFunctions(code);
		
		compileIdentifierData();
		
		for(int i=0;i<functions.size();i++)
		{
			compileFunction(-1,i,functions[i],code);
		}
		
		for(int i=0;i<classes.size();i++)
		{
			for(int j=0;j<classes[i].methods.size();j++)
			{
				compileFunction(i,j,classes[i].methods[j],code);
			}
		}
		
		int mainFunctionIndex=findFunctionIndex("__main");
		if(mainFunctionIndex==-1)
		{
			code.addError(-1,__LINE__);
			throw LogicError();
		}
		
		assembly.startCode=string("_start:\n")
			+"mov "+parameterRegisters[0]+",[rsp]"+"\n"
			+"lea "+parameterRegisters[1]+",[rsp+8]"+"\n"
			+"call "+getAssemblyFunctionName(mainFunctionIndex)+"\n"
			+"jnc .nothrow\n"
			+"mov "+returnRegister+",1\n"
			+".nothrow:\n"
			+"mov rdi,"+returnRegister+"\n"
			+"mov rax,60\n"
			+"syscall\n"
			+"\n";
		
		assemblyCode.output=
			string("global _start\n\n")
			+"section .text\n\n"
			+assembly.startCode
			+assembly.functionCode
			+"\nsection .bss\n\n"
			+assembly.globalVariableCode
			+"\nsection .data\n\n"
			+assembly.stringCode
			+assembly.identifierDataCode;
	}
	private:
		void addDefaultFunctions(TokenizedCode& code)
		{
			for(int i=0;i<classes.size();i++)
			{
				addDefaultMethodsOfClass(code,i);
			}
		}
		void addDefaultMethodsOfClass(TokenizedCode& code,int classIndex)
		{
			addDefaultConstructor(code,classIndex);
			addDefaultDestructor(code,classIndex);
			addDefaultAssignmentOperator(code,classIndex);
		}
		void addDefaultConstructor(TokenizedCode& code,int classIndex)
		{
			classes[classIndex].methods.emplace_back(false,"__dc",false,Type(),vector<CodeTopFunctionParameter>(),true);
			if(findEmptyConstructorIndex(classIndex)==-1)
			{
				classes[classIndex].methods.emplace_back(false,"constructor",false,Type(),vector<CodeTopFunctionParameter>(),true);
			}
		}
		void addDefaultDestructor(TokenizedCode& code,int classIndex)
		{
			classes[classIndex].methods.emplace_back(false,"__dd",false,Type(),vector<CodeTopFunctionParameter>(),true);
			if(findMethodIndex(classIndex,"destructor")==-1)
			{
				classes[classIndex].methods.emplace_back(false,"destructor",false,Type(),vector<CodeTopFunctionParameter>(),true);
			}
		}
		void addDefaultAssignmentOperator(TokenizedCode& code,int classIndex)
		{
			if(findMethodIndex(classIndex,"=")==-1)
			{
				classes[classIndex].methods.emplace_back(true,"=",false,Type(),
					vector<CodeTopFunctionParameter>{CodeTopFunctionParameter(Type(classes[classIndex].name,0),"other")},
					true);
			}
		}
		bool isTypePassedByReference(const Type& type)
		{
			if(type.pointerLevels>0)
			{
				return false;
			}
			else
			{
				DataType& dataType=dataTypes[type.name];
				return dataType.isPassedByReference;
			}
		}
		void allocateLocalVariable(NamedVector<LocalVariable>& localVariables,FunctionContext& functionContext,const LocalVariable& varToAdd)
		{
			LocalVariable var=varToAdd;
			
			size_t size=0;
			size_t alignment=1;
			
			getTypeSizeAndAlignment(var.type,size,alignment,var.isReference);
			
			functionContext.variableStack+=toAlign(functionContext.variableStack,alignment);
			functionContext.variableStack+=size;
			
			var.stackOffset=-functionContext.variableStack;
			
			if(functionContext.variableStack>functionContext.variableStackMax)
			{
				functionContext.variableStackMax=functionContext.variableStack;
			}
			
			localVariables.add(var);
		}
		string getNewName(FunctionContext& functionContext)
		{
			string str=string("____")+to_string(functionContext.namesCreated);
			functionContext.namesCreated++;
			return str;
		}
		int createLabel(FunctionContext& functionContext)
		{
			functionContext.labelsCreated++;
			return functionContext.labelsCreated-1;
		}
		string getSizedRegister(const string& fullRegisterName,int size)
		{
			if(size==8) return fullRegisterName;
			else
			{
				for(int i=0;i<machineRegisters.regs8.size();i++)
				{
					if(machineRegisters.regs8[i]==fullRegisterName)
					{
						if(size==4) return machineRegisters.regs4[i];
						else if(size==2) return machineRegisters.regs2[i];
						else if(size==1) return machineRegisters.regs1[i];
						else break;
					}
				}
				throw LogicError();
			}
		}
		string movLocalToRegister(const LocalVariable& localVariable,int offset,int size,const string& reg)
		{
			int stackOffset=localVariable.stackOffset+offset;
			return string("mov ")+getSizedRegister(reg,size)+",[rbp-"+to_string(-stackOffset)+"]\n";
		}
		string movRegisterToLocal(const LocalVariable& localVariable,int offset,int size,const string& reg)
		{
			int stackOffset=localVariable.stackOffset+offset;
			return string("mov [rbp-")+to_string(-stackOffset)+"],"+getSizedRegister(reg,size)+"\n";
		}
		string movLocalToRegister(const LocalVariable& localVariable,const string& reg)
		{
			size_t size=0;
			size_t alignment=1;
			getTypeSizeAndAlignment(localVariable.type,size,alignment,localVariable.isReference);
			return movLocalToRegister(localVariable,0,size,reg);
		}
		string movRegisterToLocal(const LocalVariable& localVariable,const string& reg)
		{
			size_t size=0;
			size_t alignment=1;
			getTypeSizeAndAlignment(localVariable.type,size,alignment,localVariable.isReference);
			return movRegisterToLocal(localVariable,0,size,reg);
		}
		string movLocalAddressToRegister(const LocalVariable& localVariable,int offset,const string& reg)
		{
			int stackOffset=localVariable.stackOffset+offset;
			return string("lea ")+reg+",[rbp-"+to_string(-stackOffset)+"]\n";
		}
		string movGlobalAddressToRegister(int globalVariableIndex,int offset,const string& reg)
		{
			string str=string("mov ")+reg+","+getAssemblyGlobalVariableName(globalVariableIndex);
			if(offset!=0)
			{
				str+=string("+")+to_string(offset);
			}
			str+="\n";
			return str;
		}
		string movInputParameterToLocal(int inputParameterIndex,const LocalVariable& localVariable)
		{
			size_t size=0;
			size_t alignment=1;
			getTypeSizeAndAlignment(localVariable.type,size,alignment,localVariable.isReference);
			
			if(inputParameterIndex<parameterRegisters.size())
			{
				return movRegisterToLocal(localVariable,0,size,parameterRegisters[inputParameterIndex]);
			}
			else
			{
				string str;
				
				string temporaryRegisterToUse="rax";
				
				int stackOffset=16+(inputParameterIndex-parameterRegisters.size())*8;
				str+=string("mov ")+getSizedRegister(temporaryRegisterToUse,size)+",[rbp+"+to_string(stackOffset)+"]\n";
				
				str+=movRegisterToLocal(localVariable,0,size,temporaryRegisterToUse);
				
				return str;
			}
		}
		string movImmediateToLocal(const LocalVariable& localVariable,int64_t immediate,int stringIndex=-1)
		{
			string str;
			
			size_t size=0;
			size_t alignment=1;
			getTypeSizeAndAlignment(localVariable.type,size,alignment,localVariable.isReference);
			
			string temporaryRegisterToUse="rax";
			
			if(stringIndex==-1)
			{
				str+=string("mov ")+getSizedRegister(temporaryRegisterToUse,size)+","+to_string(immediate)+"\n";
			}
			else
			{
				str+=string("mov ")+getSizedRegister(temporaryRegisterToUse,size)+","+getAssemblyStringName(stringIndex)+"\n";
			}
			
			str+=movRegisterToLocal(localVariable,0,size,temporaryRegisterToUse);
			
			return str;
		}
		int createTmp(NamedVector<LocalVariable>& localVariables,FunctionContext& functionContext,const Type& type,bool isReference)
		{
			allocateLocalVariable(localVariables,functionContext,
				LocalVariable(
					type,
					isReference,
					getNewName(functionContext)
					));
			return localVariables.size()-1;
		}
		void compileFunctionStart(int classIndex,int functionIndex,CodeTopFunction& function,TokenizedCode& code,
			string& functionStartCode,string& functionCode,FunctionContext& functionContext)
		{
			functionStartCode=string();
			
			
			functionContext=FunctionContext();
			
			functionContext.classIndex=classIndex;
			functionContext.functionIndex=functionIndex;
			
			functionContext.isMethod=(classIndex>=0);
			
			if(functionContext.isMethod)
			{
				functionStartCode+=getAssemblyMethodName(classIndex,functionIndex)+":\n";
			}
			else
			{
				functionStartCode+=getAssemblyFunctionName(functionIndex)+":\n";
			}
			
			functionStartCode+="push rbp\n";
			functionStartCode+="mov rbp,rsp\n";
			
			functionCode=string();
			
			NamedVector<LocalVariable>& localVariables=functionContext.baseLocalVariables;
			
			{
				{
					functionContext.indexOfVariableToThrow=localVariables.size();
					allocateLocalVariable(localVariables,functionContext,
						LocalVariable(
							Type("dynamic",0),
							false,
							getNewName(functionContext)
							));
				}
				
				int inputParameterIndex=0;
				
				if(function.returnsSomething)
				{
					functionContext.indexOfVariableToReturn=localVariables.size();
					if(function.returnsReference)
					{
						allocateLocalVariable(localVariables,functionContext,
							LocalVariable(
								function.returnType,
								true,
								getNewName(functionContext)
								));
					}
					else
					{
						allocateLocalVariable(localVariables,functionContext,
							LocalVariable(
								function.returnType,
								isTypePassedByReference(function.returnType),
								getNewName(functionContext)
								));
						
						if(isTypePassedByReference(function.returnType))
						{
							functionCode+=movInputParameterToLocal(inputParameterIndex,localVariables[functionContext.indexOfVariableToReturn]);
							inputParameterIndex++;
						}
					}
				}
				
				if(functionContext.isMethod)
				{
					CodeTopClass& c=classes[classIndex];
					
					allocateLocalVariable(localVariables,functionContext,
						LocalVariable(
							Type(c.name,0),
							true,
							"this"
							));
					
					functionCode+=movInputParameterToLocal(inputParameterIndex,localVariables[localVariables.size()-1]);
					inputParameterIndex++;
				}
				
				for(int i=0;i<function.parameters.size();i++)
				{
					allocateLocalVariable(localVariables,functionContext,
						LocalVariable(
							function.parameters[i].type,
							isTypePassedByReference(function.parameters[i].type),
							function.parameters[i].name
							));
					
					functionCode+=movInputParameterToLocal(inputParameterIndex,localVariables[localVariables.size()-1]);
					inputParameterIndex++;
				}
			}
			
			functionCode+=movImmediateToLocal(localVariables[functionContext.indexOfVariableToThrow],0);
			
			functionContext.functionLocalIndexStart=localVariables.size();
			
			functionContext.enterScope(localVariables.size());
		}
		void compileFunctionEnd(int classIndex,int functionIndex,CodeTopFunction& function,TokenizedCode& code,
			string& functionStartCode,string& functionCode,FunctionContext& functionContext)
		{
			NamedVector<LocalVariable>& localVariables=functionContext.baseLocalVariables;
			
			if(function.returnsSomething)
			{
				if(!isTypePassedByReference(function.returnType) || function.returnsReference)
				{
					size_t size=0;
					size_t alignment=1;
					getTypeSizeAndAlignment(function.returnType,size,alignment,function.returnsReference);
					
					functionCode+=movLocalToRegister(localVariables[functionContext.indexOfVariableToReturn],0,size,returnRegister);
				}
			}
			functionCode+="clc\n";
			functionCode+="leave\n";
			functionCode+="ret\n";
			
			size_t totalStackSize=functionContext.variableStackMax+functionContext.parameterStack;
			totalStackSize=totalStackSize+toAlign(totalStackSize,16);
			
			functionStartCode+=string("sub rsp,")+to_string(totalStackSize)+"\n";
			
			assembly.functionCode+=functionStartCode+functionCode+"\n";
		}
		void compileFunction(int classIndex,int functionIndex,CodeTopFunction& function,TokenizedCode& code)
		{
			string functionStartCode;
			string functionCode;
			FunctionContext functionContext;
			
			compileFunctionStart(classIndex,functionIndex,function,code,functionStartCode,functionCode,functionContext);
			NamedVector<LocalVariable> localVariables=functionContext.baseLocalVariables;
			
			if(functionContext.isMethod && function.name=="constructor")
			{
				ExpressionValue value;
				functionCode+=compileCallFunction(function,code,functionContext,localVariables,
					classIndex,findMethodIndex(classIndex,"__dc"),vector<int>{localVariables.getIndexOf("this")},value,false,Type(),false);
			}
			
			if(function.compilerGenerated)
			{
				if(functionContext.isMethod)
				{
					if(function.name=="__dc")
					{
						functionCode+=compileCallConstructorsOfAttributesOfThis(function,code,functionContext,localVariables);
					}
					else if(function.name=="__dd")
					{
						functionCode+=compileCallDestructorsOfAttributesOfThis(function,code,functionContext,localVariables);
					}
					else if(function.name=="=")
					{
						functionCode+=compileCallAssignmentOperatorsOfAttributesOfThis(function,code,functionContext,localVariables);
					}
				}
			}
			else
			{
				functionCode+=compileScope(function,code,function.codeStart,functionContext,localVariables);
			}
			
			functionCode+="jmp .return\n";
			{
				functionCode+=".return_throw:\n";
				
				functionCode+=movLocalToRegister(localVariables[functionContext.indexOfVariableToThrow],returnRegister);
				functionCode+="stc\n";
				functionCode+="leave\n";
				functionCode+="ret\n";
			}
			
			functionCode+=".return:\n";
			
			if(functionContext.isMethod && function.name=="destructor")
			{
				ExpressionValue value;
				functionCode+=compileCallFunction(function,code,functionContext,localVariables,
					classIndex,findMethodIndex(classIndex,"__dd"),vector<int>{localVariables.getIndexOf("this")},value,false,Type(),false);
			}
			
			compileFunctionEnd(classIndex,functionIndex,function,code,functionStartCode,functionCode,functionContext);
		}
		string compileScope(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& outLocalVariables)
		{
			string scopeCode;
			
			NamedVector<LocalVariable> localVariables=outLocalVariables;
			int previousScopeLocalIndexStart=functionContext.enterScope(localVariables.size());
			
			if(code.get(t)!="{")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
			
			while(true)
			{
				if(code.get(t)=="}")
				{
					break;
				}
				scopeCode+=compileStatement(function,code,t,functionContext,localVariables);
			}
			
			if(code.get(t)!="}")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
			
			scopeCode+=compileExitScope(function,code,functionContext,localVariables);
			functionContext.exitScope(previousScopeLocalIndexStart);
			
			return scopeCode;
		}
		string compileExitScope(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables)
		{
			string output;
			
			int localStart=functionContext.scopeLocalIndexStart;
			
			output+=compileCallDestructorsUntilLocalIndex(function,code,functionContext,localVariables,localStart);
			
			return output;
		}
		string compileCallDestructorsUntilLocalIndex(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			int localIndexStart)
		{
			string output;
			for(int i=int(localVariables.size())-1;i>=localIndexStart;i--)
			{
				output+=compileCallDestructorOfLocal(function,code,functionContext,localVariables,i);
			}
			return output;
		}
		bool isTypeWithConstructorAndDestructor(const Type& type)
		{
			if(type.pointerLevels>0) return false;
			if(dataTypes[type.name].isInteger) return false;
			return true;
		}
		bool isTypeWithAssignmentOperator(const Type& type)
		{
			if(type.pointerLevels>0) return false;
			if(dataTypes[type.name].isInteger) return false;
			return true;
		}
		string compileCallConstructorOfLocal(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,int localVariableIndex)
		{
			return compileCallConstructorOrDestructorOfLocal(function,code,functionContext,localVariables,localVariableIndex,"constructor");
		}
		string compileCallDestructorOfLocal(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,int localVariableIndex)
		{
			return compileCallConstructorOrDestructorOfLocal(function,code,functionContext,localVariables,localVariableIndex,"destructor");
		}
		string compileCallConstructorOrDestructorOfLocal(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,
			NamedVector<LocalVariable>& localVariables,int localVariableIndex,const string& methodName)
		{
			if(!localVariables[localVariableIndex].isReference)
			{
				if(isTypeWithConstructorAndDestructor(localVariables[localVariableIndex].type))
				{
					if(localVariables[localVariableIndex].type.name=="dynamic")
					{
						if(methodName=="constructor")
						{
							return movImmediateToLocal(localVariables[localVariableIndex],0);
						}
						else
						{
							ExpressionValue value;
							return compileCallFunction(function,code,functionContext,localVariables,-1,findFunctionIndex("__dynamic_destructor"),
								vector<int>{localVariableIndex},value,false,Type(),false);
						}
					}
					else
					{
						int classIndex=findClassIndex(localVariables[localVariableIndex].type.name);
						if(classIndex==-1)
						{
							code.addError(-1,__LINE__);
							throw LogicError();
						}
						int functionIndex=-1;
						if(methodName=="constructor")
						{
							functionIndex=findEmptyConstructorIndex(classIndex);
						}
						else
						{
							functionIndex=findMethodIndex(classIndex,methodName);
						}
						if(functionIndex==-1)
						{
							code.addError(-1,__LINE__);
							throw LogicError();
						}
						ExpressionValue value;
						return compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,vector<int>{localVariableIndex},value,false,Type(),false);
					}
				}
			}
			return string();
		}
		string compileCallAssignmentOperatorOfDynamicLocal(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,
			NamedVector<LocalVariable>& localVariables,int localVariableIndex,int localVariableIndex2)
		{
			string output;
			
			int localVariableIndex3=createTmp(localVariables,functionContext,Type("i64",0),false);
			
			output+=movImmediateToLocal(localVariables[localVariableIndex3],dataTypes.getIndexOf(localVariables[localVariableIndex2].type.name));
			
			ExpressionValue value;
			output+=compileCallFunction(function,code,functionContext,localVariables,-1,findFunctionIndex("__dynamic_assign"),
				vector<int>{localVariableIndex,localVariableIndex2,localVariableIndex3},value,false,Type(),false);
			
			return output;
		}
		string compileCallAssignmentOperatorOfLocal(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,
			NamedVector<LocalVariable>& localVariables,int localVariableIndex,int localVariableIndex2)
		{
			if(localVariables[localVariableIndex].type.name=="dynamic" && localVariables[localVariableIndex].type.pointerLevels==0)
			{
				return compileCallAssignmentOperatorOfDynamicLocal(function,code,functionContext,localVariables,localVariableIndex,localVariableIndex2);
			}
			else
			{
				string output;
				
				if(localVariables[localVariableIndex2].type.name=="dynamic" && localVariables[localVariableIndex2].type.pointerLevels==0)
				{
					ExpressionValue rvalue;
					rvalue.localVariableIndex=localVariableIndex2;
					
					ExpressionValue castValue;
					output+=compileCastFromDynamic(function,code,t,functionContext,localVariables,rvalue,castValue,localVariables[localVariableIndex].type);
					
					localVariableIndex2=castValue.localVariableIndex;
				}
				
				int classIndex=findClassIndex(localVariables[localVariableIndex].type.name);
				if(classIndex==-1)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				int functionIndex=findMethodIndex(classIndex,"=");
				if(functionIndex==-1)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				ExpressionValue value;
				output+=compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,vector<int>{localVariableIndex,localVariableIndex2},value,false,Type(),false);
				
				return output;
			}
		}
		string compileCallConstructorsOfAttributesOfThis(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,
			NamedVector<LocalVariable>& localVariables)
		{
			string output;
			
			CodeTopClass& c=classes[functionContext.classIndex];
			for(int i=0;i<c.attributes.size();i++)
			{
				output+=compileCallConstructorOrDestructorOfAttributeOfThis(function,code,functionContext,localVariables,i,"constructor");
			}
			
			return output;
		}
		string compileCallDestructorsOfAttributesOfThis(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,
			NamedVector<LocalVariable>& localVariables)
		{
			string output;
			
			CodeTopClass& c=classes[functionContext.classIndex];
			for(int i=int(c.attributes.size())-1;i>=0;i--)
			{
				output+=compileCallConstructorOrDestructorOfAttributeOfThis(function,code,functionContext,localVariables,i,"destructor");
			}
			
			return output;
		}
		string compileCallAssignmentOperatorsOfAttributesOfThis(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,
			NamedVector<LocalVariable>& localVariables)
		{
			string output;
			
			CodeTopClass& c=classes[functionContext.classIndex];
			for(int i=0;i<c.attributes.size();i++)
			{
				output+=compileCallAssignmentOperatorOfAttributeOfThis(function,code,functionContext,localVariables,i);
			}
			
			return output;
		}
		string compileCallAssignmentOperatorOfAttributeOfThis(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,
			NamedVector<LocalVariable>& localVariables,int attributeIndex)
		{
			string output;
			
			LocalVariable thisv=localVariables["this"];
			
			LocalVariable otherv=localVariables["other"];
			
			CodeTopClassAttribute& attribute=classes[functionContext.classIndex].attributes[attributeIndex];
			
			string registerToUse="rax";
			
			
			int localVariableIndex=createTmp(localVariables,functionContext,attribute.type,true);
			
			LocalVariable v=localVariables[localVariableIndex];
			
			output+=movLocalToRegister(thisv,registerToUse);
			output+=string("add ")+registerToUse+","+to_string(attribute.objectOffset)+"\n";
			output+=movRegisterToLocal(v,registerToUse);
			
			
			int localVariableIndex2=createTmp(localVariables,functionContext,attribute.type,true);
			
			LocalVariable v2=localVariables[localVariableIndex2];
			
			output+=movLocalToRegister(otherv,registerToUse);
			output+=string("add ")+registerToUse+","+to_string(attribute.objectOffset)+"\n";
			output+=movRegisterToLocal(v2,registerToUse);
			
			
			if(isTypeWithAssignmentOperator(attribute.type))
			{
				if(attribute.type.name=="dynamic")
				{
					output+=compileCallAssignmentOperatorOfDynamicLocal(function,code,functionContext,localVariables,localVariableIndex,localVariableIndex2);
				}
				else
				{
					int classIndex=findClassIndex(v.type.name);
					if(classIndex==-1)
					{
						code.addError(-1,__LINE__);
						throw LogicError();
					}
					int functionIndex=findMethodIndex(classIndex,"=");
					if(functionIndex==-1)
					{
						code.addError(-1,__LINE__);
						throw LogicError();
					}
					ExpressionValue value;
					output+=compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,vector<int>{localVariableIndex,localVariableIndex2},value,false,Type(),false);
				}
			}
			else
			{
				string registerToUse2="rcx";
				
				output+=movLocalToRegisterGetValue(v2,registerToUse);
				output+=movLocalToRegister(v,registerToUse2);
				output+=string("mov ")+"["+registerToUse2+"],"+getSizedRegister(registerToUse,getTypeSize(attribute.type,false))+"\n";
			}
			
			return output;
		}
		string compileCallConstructorOrDestructorOfAttributeOfThis(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,
			NamedVector<LocalVariable>& localVariables,int attributeIndex,const string& methodName)
		{
			string output;
			
			LocalVariable thisv=localVariables["this"];
			
			CodeTopClassAttribute& attribute=classes[functionContext.classIndex].attributes[attributeIndex];
			
			if(isTypeWithConstructorAndDestructor(attribute.type))
			{
				int localVariableIndex=createTmp(localVariables,functionContext,attribute.type,true);
				
				LocalVariable v=localVariables[localVariableIndex];
				
				string registerToUse="rax";
				
				output+=movLocalToRegister(thisv,registerToUse);
				output+=string("add ")+registerToUse+","+to_string(attribute.objectOffset)+"\n";
				output+=movRegisterToLocal(v,registerToUse);
				
				if(v.type.name=="dynamic")
				{
					if(methodName=="constructor")
					{
						output+=movImmediateToLocal(localVariables[localVariableIndex],0);
					}
					else
					{
						ExpressionValue value;
						output+=compileCallFunction(function,code,functionContext,localVariables,-1,findFunctionIndex("__dynamic_destructor"),
							vector<int>{localVariableIndex},value,false,Type(),false);
					}
				}
				else
				{
					int classIndex=findClassIndex(v.type.name);
					if(classIndex==-1)
					{
						code.addError(-1,__LINE__);
						throw LogicError();
					}
					int functionIndex=-1;
					if(methodName=="constructor")
					{
						functionIndex=findEmptyConstructorIndex(classIndex);
					}
					else
					{
						functionIndex=findMethodIndex(classIndex,methodName);
					}
					if(functionIndex==-1)
					{
						code.addError(-1,__LINE__);
						throw LogicError();
					}
					ExpressionValue value;
					output+=compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,vector<int>{localVariableIndex},value,false,Type(),false);
				}
			}
			
			return output;
		}
		string compileCallFunctionIndirect(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue functionAddressValue,const vector<int>& argumentIndexes,ExpressionValue& value,ExpressionValue returnTypeIndexValue,bool isDestructor)
		{
			string output;
			
			value=ExpressionValue();
			value.localVariableIndex=createTmp(localVariables,functionContext,Type("dynamic",0),false);
			output+=movImmediateToLocal(localVariables[value.localVariableIndex],0);
			
			int skipReturnLabel=createLabel(functionContext);
			int endLabel=createLabel(functionContext);
			
			{
				string registerToUse="rax";
				
				output+=movLocalToRegisterGetValue(localVariables[returnTypeIndexValue.localVariableIndex],registerToUse);
				
				output+=string("cmp ")+registerToUse+","+to_string(dataTypesPassByReferenceStartIndex)+"\n";
				output+=string("jl ")+getAssemblyLabelName(skipReturnLabel)+"\n";
			}
			
			{
				//Case: The function returns an object (not primitive)
				//TODO---- NOT IMPLEMENTED
				
				vector<int> argumentIndexes2=argumentIndexes;
				argumentIndexes2.insert(argumentIndexes2.begin(),value.localVariableIndex);
				
				output+=compileCallFunctionPutParameters(function,code,functionContext,localVariables,argumentIndexes2);
				
				{
					string registerToUse="rax";
					output+=movLocalToRegisterGetValue(localVariables[functionAddressValue.localVariableIndex],registerToUse);
					output+=string("call ")+registerToUse+"\n";
				}
				
				if(!isDestructor) output+=compileCheckException(function,code,functionContext,localVariables);
			}
			
			output+=string("jmp ")+getAssemblyLabelName(endLabel)+"\n";
			output+=getAssemblyLabelName(skipReturnLabel)+":\n";
			
			{
				//Case: The function returns a primitive (not an object), or returns nothing
				
				output+=compileCallFunctionPutParameters(function,code,functionContext,localVariables,argumentIndexes);
				
				{
					string registerToUse="rax";
					output+=movLocalToRegisterGetValue(localVariables[functionAddressValue.localVariableIndex],registerToUse);
					output+=string("call ")+registerToUse+"\n";
				}
				
				if(!isDestructor) output+=compileCheckException(function,code,functionContext,localVariables);
				
				{
					ExpressionValue tmpValue;
					tmpValue.localVariableIndex=createTmp(localVariables,functionContext,Type("i64",0),false);
					
					output+=movRegisterToLocal(localVariables[tmpValue.localVariableIndex],returnRegister);
					
					int skipReturnValueLabel=createLabel(functionContext);
					{
						string registerToUse="rax";
						
						output+=movLocalToRegisterGetValue(localVariables[returnTypeIndexValue.localVariableIndex],registerToUse);
						
						output+=string("cmp ")+registerToUse+",-1\n";
						output+=string("je ")+getAssemblyLabelName(skipReturnValueLabel)+"\n";
					}
					{
						//Case: The function returns a primitive (not an object)
						
						int functionIndex=findFunctionIndex("__dynamic_assign");
						if(functionIndex==-1)
						{
							code.addError(-1,__LINE__);
							throw LogicError();
						}
						
						ExpressionValue tmp;
						output+=compileCallFunction(function,code,functionContext,localVariables,
							-1,functionIndex,vector<int>{value.localVariableIndex,tmpValue.localVariableIndex,returnTypeIndexValue.localVariableIndex},tmp,false,Type(),false);
					}
					output+=getAssemblyLabelName(skipReturnValueLabel)+":\n";
				}
			}
			
			output+=getAssemblyLabelName(endLabel)+":\n";
			
			return output;
		}
		string compileCallFunctionPutParameters(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			const vector<int>& argumentIndexes)
		{
			string output;
			
			int parameterStackSize=0;
			
			for(int parameterIndex=0;parameterIndex<argumentIndexes.size();parameterIndex++)
			{
				string registerToUse="rax";
				if(parameterIndex<parameterRegisters.size())
				{
					registerToUse=parameterRegisters[parameterIndex];
				}
				
				LocalVariable v=localVariables[argumentIndexes[parameterIndex]];
				
				bool isPassedByReference=isTypePassedByReference(v.type);
				
				int size=pointerSize;
				
				if(isPassedByReference)
				{
					size=pointerSize;
					
					if(v.isReference)
					{
						output+=movLocalToRegister(v,registerToUse);
					}
					else
					{
						output+=movLocalAddressToRegister(v,0,registerToUse);
					}
				}
				else
				{
					size=getTypeSize(v.type,false);
					
					output+=movLocalToRegisterGetValue(v,registerToUse);
				}
				
				if(parameterIndex>=parameterRegisters.size())
				{
					int stackOffset=(parameterIndex-parameterRegisters.size())*8;
					
					output+=string("mov ")+"[rsp+"+to_string(stackOffset)+"],"+getSizedRegister(registerToUse,size)+"\n";
					
					parameterStackSize+=8;
				}
			}
			
			if(parameterStackSize>functionContext.parameterStack)
			{
				functionContext.parameterStack=parameterStackSize;
			}
			
			return output;
		}
		string compileCheckException(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables)
		{
			string output;
			
			int skipThrowLabel=createLabel(functionContext);
			output+=string("jnc ")+getAssemblyLabelName(skipThrowLabel)+"\n";
			output+=movRegisterToLocal(localVariables[functionContext.indexOfVariableToThrow],returnRegister);
			
			if(functionContext.tryContext.inTry)
			{
				output+=compileCallDestructorsUntilLocalIndex(function,code,functionContext,localVariables,functionContext.tryContext.localIndexStart);
				output+=string("jmp ")+getAssemblyLabelName(functionContext.tryContext.catchLabel)+"\n";
			}
			else
			{
				output+=compileCallDestructorsUntilLocalIndex(function,code,functionContext,localVariables,functionContext.functionLocalIndexStart);
				output+="jmp .return_throw\n";
			}
			
			output+=getAssemblyLabelName(skipThrowLabel)+":\n";
			
			return output;
		}
		string compileCallFunction(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			int classIndex,int functionIndex,const vector<int>& argumentIndexes,ExpressionValue& value,bool getReturnValue,Type returnValueType,bool returnsReference)
		{
			string output;
			
			bool isDestructor=false;
			if(classIndex!=-1)
			{
				if(classes[classIndex].methods[functionIndex].name=="destructor")
				{
					isDestructor=true;
				}
			}
			else if(functions[functionIndex].name=="__dynamic_destructor")
			{
				isDestructor=true;
			}
			
			output+=compileCallFunctionPutParameters(function,code,functionContext,localVariables,argumentIndexes);
			
			if(classIndex==-1)
			{
				output+=string("call ")+getAssemblyFunctionName(functionIndex)+"\n";
			}
			else
			{
				output+=string("call ")+getAssemblyMethodName(classIndex,functionIndex)+"\n";
			}
			
			if(!isDestructor) output+=compileCheckException(function,code,functionContext,localVariables);
			
			if(getReturnValue)
			{
				value=ExpressionValue();
				
				value.localVariableIndex=createTmp(localVariables,functionContext,returnValueType,returnsReference);
				
				if(isTypePassedByReference(returnValueType) && !returnsReference)
				{
					code.addError(-1,__LINE__);
					throw LogicError();
				}
				
				output+=movRegisterToLocal(localVariables[value.localVariableIndex],returnRegister);
			}
			
			return output;
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
		bool parseCheckType(TokenizedCode& code,size_t& tokenIndex,Type& type)
		{
			size_t t=tokenIndex;
			
			try
			{
				type=Type(code,t,false);
			}
			catch(...)
			{
				return false;
			}
			
			if(!typeExists(type)) return false;
			
			tokenIndex=t;
			
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
		bool parseCheckIntegerToken(TokenizedCode& code,size_t& tokenIndex,uint64_t& integer)
		{
			size_t t=tokenIndex;
			
			string token=code.get(t);
			
			if(!parseInteger(token,integer)) return false;
			
			t++;
			
			tokenIndex=t;
			
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
		bool parseCheckStringLiteralToken(TokenizedCode& code,size_t& tokenIndex,string& literal)
		{
			size_t t=tokenIndex;
			
			string token=code.get(t);
			
			if(!parseString(token,literal)) return false;
			
			t++;
			
			tokenIndex=t;
			
			return true;
		}
		string compileCmpZeroJump(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue value,int label,const string& jumpInstructionName)
		{
			string output;
			
			string registerToUse="rax";
			
			output+=movLocalToRegisterGetValue(localVariables[value.localVariableIndex],registerToUse);
			
			output+=string("cmp ")+getSizedRegister(registerToUse,getTypeSize(localVariables[value.localVariableIndex].type,false))+",0\n";
			output+=jumpInstructionName+" "+getAssemblyLabelName(label)+"\n";
			
			return output;
		}
		bool localIsInteger(const LocalVariable& localVariable)
		{
			if(localVariable.type.pointerLevels>0)
			{
				return false;
			}
			if(!dataTypes[localVariable.type.name].isInteger)
			{
				return false;
			}
			return true;
		}
		string compileVariableDeclarationOrExpression(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables)
		{
			string statementCode;
			
			bool isVariableDeclaration=false;
			Type type;
			size_t tok=t;
			if(parseCheckType(code,t,type))
			{
				if(code.get(t)!="(")
				{
					isVariableDeclaration=true;
					t=tok;
					statementCode+=compileVariableDeclaration(function,code,t,functionContext,localVariables);
				}
				else
				{
					t=tok;
				}
			}
			
			if(!isVariableDeclaration)
			{
				statementCode+=compileExpression(function,code,t,functionContext,localVariables);
			}
			
			return statementCode;
		}
		string compileStatement(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables)
		{
			string statementCode;
			
			string start=code.get(t);
			
			if(start=="{")
			{
				statementCode+=compileScope(function,code,t,functionContext,localVariables);
			}
			else if(start=="return")
			{
				t++;
				
				if(function.returnsSomething)
				{
					ExpressionValue value;
					
					statementCode+=compileExpressionPart(function,code,t,functionContext,localVariables,value);
					
					ExpressionValue returnValue;
					returnValue.localVariableIndex=functionContext.indexOfVariableToReturn;
					
					if(function.returnsReference)
					{
						LocalVariable v=localVariables[value.localVariableIndex];
						if(!v.isReference || v.type.name!=function.returnType.name || v.type.pointerLevels!=function.returnType.pointerLevels)
						{
							code.addError(t,__LINE__);
							throw LogicError();
						}
						
						string registerToUse="rax";
						statementCode+=movLocalToRegister(v,registerToUse);
						statementCode+=movRegisterToLocal(localVariables[returnValue.localVariableIndex],registerToUse);
					}
					else
					{
						bool success=false;
						statementCode+=compileAssignment(function,code,t,functionContext,localVariables,returnValue,value,success);
						if(!success)
						{
							code.addError(t,__LINE__);
							throw LogicError();
						}
					}
				}
				
				statementCode+=compileCallDestructorsUntilLocalIndex(function,code,functionContext,localVariables,functionContext.functionLocalIndexStart);
				statementCode+="jmp .return\n";
				
				if(code.get(t)!=";")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			else if(start=="throw")
			{
				t++;
				
				{
					ExpressionValue value;
					
					statementCode+=compileExpressionPart(function,code,t,functionContext,localVariables,value);
					
					ExpressionValue throwValue;
					throwValue.localVariableIndex=functionContext.indexOfVariableToThrow;
					
					bool success=false;
					statementCode+=compileAssignment(function,code,t,functionContext,localVariables,throwValue,value,success);
					if(!success)
					{
						code.addError(t,__LINE__);
						throw LogicError();
					}
				}
				
				if(functionContext.tryContext.inTry)
				{
					statementCode+=compileCallDestructorsUntilLocalIndex(function,code,functionContext,localVariables,functionContext.tryContext.localIndexStart);
					statementCode+=string("jmp ")+getAssemblyLabelName(functionContext.tryContext.catchLabel)+"\n";
				}
				else
				{
					statementCode+=compileCallDestructorsUntilLocalIndex(function,code,functionContext,localVariables,functionContext.functionLocalIndexStart);
					statementCode+="jmp .return_throw\n";
				}
				
				if(code.get(t)!=";")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			else if(start=="try")
			{
				t++;
				
				int catchLabel=createLabel(functionContext);
				TryContext oldTryContext=functionContext.startTry(TryContext(localVariables.size(),catchLabel));
				
				int skipCatchLabel=createLabel(functionContext);
				
				statementCode+=compileScope(function,code,t,functionContext,localVariables);
				
				statementCode+=string("jmp ")+getAssemblyLabelName(skipCatchLabel)+"\n";
				
				functionContext.endTry(oldTryContext);
				
				if(code.get(t)!="catch")
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				t++;
				
				if(code.get(t)!="(")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				if(code.get(t)!="dynamic")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				string variableName=code.get(t);
				t++;
				
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				statementCode+=getAssemblyLabelName(catchLabel)+":\n";
				
				NamedVector<LocalVariable> outLocalVariables=localVariables;
				int previousScopeLocalIndexStart=functionContext.enterScope(localVariables.size());
				{
					allocateLocalVariable(localVariables,functionContext,
						LocalVariable(
							Type("dynamic",0),
							true,
							variableName
							));
					
					int localVariableIndex=int(localVariables.size())-1;
					
					string registerToUse="rax";
					statementCode+=movLocalAddressToRegister(localVariables[functionContext.indexOfVariableToThrow],0,registerToUse);
					statementCode+=movRegisterToLocal(localVariables[localVariableIndex],registerToUse);
					
					statementCode+=compileScope(function,code,t,functionContext,localVariables);
				}
				statementCode+=compileExitScope(function,code,functionContext,localVariables);
				functionContext.exitScope(previousScopeLocalIndexStart);
				localVariables=outLocalVariables;
				
				statementCode+=getAssemblyLabelName(skipCatchLabel)+":\n";
			}
			else if(start=="if")
			{
				t++;
				
				if(code.get(t)!="(")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				ExpressionValue ifValue;
				
				statementCode+=compileExpressionPart(function,code,t,functionContext,localVariables,ifValue);
				
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				if(!localIsInteger(localVariables[ifValue.localVariableIndex]))
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				int endIfLabel=createLabel(functionContext);
				int elseLabel=createLabel(functionContext);
				
				statementCode+=compileCmpZeroJump(function,code,t,functionContext,localVariables,ifValue,elseLabel,"je");
				
				if(code.get(t)=="{")
				{
					statementCode+=compileScope(function,code,t,functionContext,localVariables);
				}
				else
				{
					statementCode+=compileStatement(function,code,t,functionContext,localVariables);
				}
				
				statementCode+=string("jmp ")+getAssemblyLabelName(endIfLabel)+"\n";
				statementCode+=getAssemblyLabelName(elseLabel)+":\n";
				
				while(code.get(t)=="elif")
				{
					t++;
					
					if(code.get(t)!="(")
					{
						code.addError(t,__LINE__);
						throw ParseError();
					}
					t++;
					
					ExpressionValue elifValue;
					
					statementCode+=compileExpressionPart(function,code,t,functionContext,localVariables,elifValue);
					
					if(code.get(t)!=")")
					{
						code.addError(t,__LINE__);
						throw ParseError();
					}
					t++;
					
					if(!localIsInteger(localVariables[elifValue.localVariableIndex]))
					{
						code.addError(t,__LINE__);
						throw LogicError();
					}
					
					elseLabel=createLabel(functionContext);
					
					statementCode+=compileCmpZeroJump(function,code,t,functionContext,localVariables,elifValue,elseLabel,"je");
					
					if(code.get(t)=="{")
					{
						statementCode+=compileScope(function,code,t,functionContext,localVariables);
					}
					else
					{
						statementCode+=compileStatement(function,code,t,functionContext,localVariables);
					}
					
					statementCode+=string("jmp ")+getAssemblyLabelName(endIfLabel)+"\n";
					statementCode+=getAssemblyLabelName(elseLabel)+":\n";
				}
				
				if(code.get(t)=="else")
				{
					t++;
					
					if(code.get(t)=="{")
					{
						statementCode+=compileScope(function,code,t,functionContext,localVariables);
					}
					else
					{
						statementCode+=compileStatement(function,code,t,functionContext,localVariables);
					}
				}
				
				statementCode+=getAssemblyLabelName(endIfLabel)+":\n";
			}
			else if(start=="while")
			{
				t++;
				
				if(code.get(t)!="(")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				int repeatLabel=createLabel(functionContext);
				int endLabel=createLabel(functionContext);
				
				statementCode+=getAssemblyLabelName(repeatLabel)+":\n";
				
				
				ExpressionValue conditionValue;
				
				statementCode+=compileExpressionPart(function,code,t,functionContext,localVariables,conditionValue);
				
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				if(!localIsInteger(localVariables[conditionValue.localVariableIndex]))
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				statementCode+=compileCmpZeroJump(function,code,t,functionContext,localVariables,conditionValue,endLabel,"je");
				
				LoopContext outLoopContext=functionContext.startLoop(LoopContext(localVariables.size(),repeatLabel,endLabel));
				
				statementCode+=compileScope(function,code,t,functionContext,localVariables);
				
				functionContext.endLoop(outLoopContext);
				
				statementCode+=string("jmp ")+getAssemblyLabelName(repeatLabel)+"\n";
				
				statementCode+=getAssemblyLabelName(endLabel)+":\n";
			}
			else if(start=="for")
			{
				t++;
				
				NamedVector<LocalVariable> outLocalVariables=localVariables;
				int previousScopeLocalIndexStart=functionContext.enterScope(localVariables.size());
				
				
				if(code.get(t)!="(")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				if(code.get(t)==";")
				{
					t++;
				}
				else
				{
					statementCode+=compileVariableDeclarationOrExpression(function,code,t,functionContext,localVariables);
				}
				
				int repeatLabel=createLabel(functionContext);
				int continueLabel=createLabel(functionContext);
				int endLabel=createLabel(functionContext);
				statementCode+=getAssemblyLabelName(repeatLabel)+":\n";
				
				if(code.get(t)!=";")
				{
					ExpressionValue conditionValue;
					statementCode+=compileExpressionPart(function,code,t,functionContext,localVariables,conditionValue);
					if(!localIsInteger(localVariables[conditionValue.localVariableIndex]))
					{
						code.addError(t,__LINE__);
						throw LogicError();
					}
					
					statementCode+=compileCmpZeroJump(function,code,t,functionContext,localVariables,conditionValue,endLabel,"je");
				}
				
				if(code.get(t)!=";")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				
				int skipChangeLabel=createLabel(functionContext);
				
				statementCode+=string("jmp ")+getAssemblyLabelName(skipChangeLabel)+"\n";
				
				statementCode+=getAssemblyLabelName(continueLabel)+":\n";
				
				
				if(code.get(t)!=")")
				{
					ExpressionValue value;
					statementCode+=compileExpressionPart(function,code,t,functionContext,localVariables,value);
				}
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				
				statementCode+=string("jmp ")+getAssemblyLabelName(repeatLabel)+"\n";
				
				statementCode+=getAssemblyLabelName(skipChangeLabel)+":\n";
				
				
				LoopContext outLoopContext=functionContext.startLoop(LoopContext(localVariables.size(),continueLabel,endLabel));
				
				statementCode+=compileScope(function,code,t,functionContext,localVariables);
				
				functionContext.endLoop(outLoopContext);
				
				
				statementCode+=string("jmp ")+getAssemblyLabelName(continueLabel)+"\n";
				
				statementCode+=getAssemblyLabelName(endLabel)+":\n";
				
				
				statementCode+=compileExitScope(function,code,functionContext,localVariables);
				functionContext.exitScope(previousScopeLocalIndexStart);
				localVariables=outLocalVariables;
			}
			else if(start=="break")
			{
				t++;
				
				if(code.get(t)!=";")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				if(!functionContext.loopContext.inLoop)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				statementCode+=compileCallDestructorsUntilLocalIndex(function,code,functionContext,localVariables,functionContext.loopContext.localIndexStart);
				statementCode+=string("jmp ")+getAssemblyLabelName(functionContext.loopContext.breakLabel)+"\n";
			}
			else if(start=="continue")
			{
				t++;
				
				if(code.get(t)!=";")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				if(!functionContext.loopContext.inLoop)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				statementCode+=compileCallDestructorsUntilLocalIndex(function,code,functionContext,localVariables,functionContext.loopContext.localIndexStart);
				statementCode+=string("jmp ")+getAssemblyLabelName(functionContext.loopContext.continueLabel)+"\n";
			}
			else if(start=="asm")
			{
				statementCode+=compileInlineAssembly(function,code,t,functionContext,localVariables);
			}
			else
			{
				statementCode+=compileVariableDeclarationOrExpression(function,code,t,functionContext,localVariables);
			}
			
			return statementCode;
		}
		string compileVariableDeclaration(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables)
		{
			string outputCode;
			
			Type type=Type(code,t);
			
			string name=code.get(t);
			t++;
			
			allocateLocalVariable(localVariables,functionContext,
				LocalVariable(
					type,
					false,
					name
					));
			
			int localVariableIndex=int(localVariables.size())-1;
			
			outputCode+=compileCallConstructorOfLocal(function,code,functionContext,localVariables,localVariableIndex);
			
			if(code.get(t)=="=")
			{
				t--;
				
				outputCode+=compileExpression(function,code,t,functionContext,localVariables);
			}
			else
			{
				if(code.get(t)!=";")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			
			return outputCode;
		}
		string compileInlineAssembly(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables)
		{
			string outputCode;
			
			if(code.get(t)!="asm")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
			
			if(code.get(t)!="(")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
			
			string content;
			while(true)
			{
				string token=code.get(t);
				if(token==")" || token==",")
				{
					break;
				}
				
				string literal;
				if(parseCheckStringLiteralToken(code,t,literal))
				{
					content+=literal+"\n";
				}
				else
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
			}
			
			string outputCodeStart;
			string outputCodeEnd;
			
			while(true)
			{
				if(code.get(t)==")")
				{
					break;
				}
				else if(code.get(t)!=",")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				
				t++;
				
				string literal;
				if(!parseCheckStringLiteralToken(code,t,literal))
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				
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
					
					string variableName=code.get(t);
					if(isOutput)
					{
						try
						{
							LocalVariable v=localVariables[variableName];
							outputCodeEnd+=movRegisterToLocal(v,reg);
						}
						catch(...)
						{
							code.addError(t,__LINE__);
							throw NameError();
						}
					}
					else
					{
						try
						{
							LocalVariable v=localVariables[variableName];
							outputCodeStart+=movLocalToRegister(v,reg);
						}
						catch(...)
						{
							code.addError(t,__LINE__);
							throw NameError();
						}
					}
					t++;
					
					if(code.get(t)!=")")
					{
						code.addError(t,__LINE__);
						throw ParseError();
					}
					t++;
				}
			}
			
			if(code.get(t)!=")")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
			
			if(code.get(t)!=";")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
			
			outputCode=outputCodeStart+";inline assembly start\n"+content+";inline assembly end\n"+outputCodeEnd;
			
			return outputCode;
		}
		string compileExpression(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables)
		{
			string expressionCode;
			
			ExpressionValue value;
			
			expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,value);
			
			if(code.get(t)!=";")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
			
			return expressionCode;
		}
		string compileExpressionValueImmediate(FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,ExpressionValue& value,Type type,uint64_t integer,
			bool isIntegerLiteral,int stringIndex=-1)
		{
			string expressionCode;
			
			value=ExpressionValue();
			
			value.localVariableIndex=createTmp(localVariables,functionContext,type,false);
			
			expressionCode+=movImmediateToLocal(localVariables[value.localVariableIndex],integer,stringIndex);
			
			value.isIntegerLiteral=isIntegerLiteral;
			
			return expressionCode;
		}
		int createAssemblyString(const string& content)
		{
			int index=assembly.numberOfStrings;
			
			string chars;
			for(int i=0;i<content.size();i++)
			{
				chars+=to_string(int(uint8_t(content[i])))+",";
			}
			chars+="0";
			
			assembly.stringCode+=getAssemblyStringName(index)+": db "+chars+"\n";
			
			assembly.numberOfStrings++;
			return index;
		}
		int getOffsetAndTypeOfAttribute(Type baseType,const string& attributeName,Type& attributeType)
		{
			int offset=0;
			
			CodeTopClassAttribute*attribute=findClassAttribute(baseType,attributeName);
			if(attribute==nullptr) return -1;
			
			attributeType=attribute->type;
			offset=attribute->objectOffset;
			
			return offset;
		}
		bool isAssignable(NamedVector<LocalVariable>& localVariables,Type lvalueType,const ExpressionValue& rvalueExpression)
		{
			LocalVariable rvalue=localVariables[rvalueExpression.localVariableIndex];
			
			if(lvalueType.name=="dynamic" && lvalueType.pointerLevels==0 && rvalue.type.pointerLevels==0)
			{
				return true;
			}
			else if(rvalue.type.name=="dynamic" && rvalue.type.pointerLevels==0 && lvalueType.pointerLevels==0)
			{
				return true;
			}
			else if(rvalueExpression.isNullptr)
			{
				if(lvalueType.pointerLevels>0) return true;
				else return false;
			}
			if(rvalueExpression.isIntegerLiteral)
			{
				if(lvalueType.pointerLevels>0) return false;
				return dataTypes[lvalueType.name].isInteger;
			}
			else
			{
				if(lvalueType.pointerLevels!=rvalue.type.pointerLevels) return false;
				if(lvalueType.name!=rvalue.type.name) return false;
			}
			
			return true;
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
			if(op=="==" || op=="!=" || op=="<" || op==">" || op=="<=" || op==">=") return true;
			else return false;
		}
		string movLocalToRegisterGetValue(const LocalVariable& localVariable,const string& reg)
		{
			string expressionCode;
			
			size_t size=getTypeSize(localVariable.type,false);
			
			if(localVariable.isReference)
			{
				expressionCode+=movLocalToRegister(localVariable,reg);
				expressionCode+=string("mov ")+getSizedRegister(reg,size)+",["+reg+"]\n";
			}
			else
			{
				expressionCode+=movLocalToRegister(localVariable,reg);
			}
			
			return expressionCode;
		}
		string signExtendRegister(const string& reg,int initialSize,int finalSize)
		{
			string output;
			if(finalSize>initialSize)
			{
				if(finalSize==8 && initialSize==4)
				{
					output+=string("movsxd ")+getSizedRegister(reg,finalSize)+","+getSizedRegister(reg,initialSize)+"\n";
				}
				else
				{
					output+=string("movsx ")+getSizedRegister(reg,finalSize)+","+getSizedRegister(reg,initialSize)+"\n";
				}
			}
			return output;
		}
		string zeroExtendRegister(const string& reg,int initialSize,int finalSize)
		{
			string output;
			if(finalSize>initialSize)
			{
				if(finalSize==8 && initialSize==4)
				{
					output+=string("mov ")+getSizedRegister(reg,initialSize)+","+getSizedRegister(reg,initialSize)+"\n";
				}
				else
				{
					output+=string("movzx ")+getSizedRegister(reg,finalSize)+","+getSizedRegister(reg,initialSize)+"\n";
				}
			}
			return output;
		}
		size_t getSizeOfTypeFromName(const string& type,bool isReference)
		{
			return getTypeSize(Type(type,0),isReference);
		}
		bool isSignedPrimitiveTypeName(const string& type)
		{
			return type=="i8" || type=="i16" || type=="i32" || type=="i64";
		}
		string compileIntegerOperation(const string& op,const string& type,const string& regl,const string& regr)
		{
			string output;
			
			int size=getSizeOfTypeFromName(type,false);
			bool isSigned=isSignedPrimitiveTypeName(type);
			
			if(op=="*")
			{
				if(regl!="rax" || regr=="rax")
				{
					throw string("Compiler logic error from line: ")+to_string(__LINE__);
				}
				output+=string("mul ")+getSizedRegister(regr,size)+"\n";
			}
			else if(op=="/")
			{
				if(regl!="rax" || regr=="rax" || regr=="rdx")
				{
					throw string("Compiler logic error from line: ")+to_string(__LINE__);
				}
				output+="xor edx,edx\n";
				if(isSigned)
				{
					output+=string("idiv ")+getSizedRegister(regr,size)+"\n";
				}
				else
				{
					output+=string("div ")+getSizedRegister(regr,size)+"\n";
				}
			}
			else if(op=="%")
			{
				if(regl!="rax" || regr=="rax" || regr=="rdx")
				{
					throw string("Compiler logic error from line: ")+to_string(__LINE__);
				}
				output+="xor edx,edx\n";
				if(isSigned)
				{
					output+=string("idiv ")+getSizedRegister(regr,size)+"\n";
				}
				else
				{
					output+=string("div ")+getSizedRegister(regr,size)+"\n";
				}
				output+=string("mov ")+getSizedRegister(regl,size)+","+getSizedRegister("rdx",size)+"\n";
			}
			else if(op=="+")
			{
				output+=string("add ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
			}
			else if(op=="-")
			{
				output+=string("sub ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
			}
			else if(op=="<<")
			{
				if(regl=="rcx" || regr!="rcx")
				{
					throw string("Compiler logic error from line: ")+to_string(__LINE__);
				}
				output+=string("shl ")+getSizedRegister(regl,size)+",cl\n";
			}
			else if(op==">>")
			{
				if(regl=="rcx" || regr!="rcx")
				{
					throw string("Compiler logic error from line: ")+to_string(__LINE__);
				}
				if(isSigned)
				{
					output+=string("sar ")+getSizedRegister(regl,size)+",cl\n";
				}
				else
				{
					output+=string("shr ")+getSizedRegister(regl,size)+",cl\n";
				}
			}
			else if(op=="<")
			{
				output+=string("cmp ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
				if(isSigned)
				{
					output+=string("setl ")+getSizedRegister(regl,1)+"\n";
				}
				else
				{
					output+=string("setb ")+getSizedRegister(regl,1)+"\n";
				}
			}
			else if(op=="<=")
			{
				output+=string("cmp ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
				if(isSigned)
				{
					output+=string("setle ")+getSizedRegister(regl,1)+"\n";
				}
				else
				{
					output+=string("setbe ")+getSizedRegister(regl,1)+"\n";
				}
			}
			else if(op==">")
			{
				output+=string("cmp ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
				if(isSigned)
				{
					output+=string("setg ")+getSizedRegister(regl,1)+"\n";
				}
				else
				{
					output+=string("seta ")+getSizedRegister(regl,1)+"\n";
				}
			}
			else if(op==">=")
			{
				output+=string("cmp ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
				if(isSigned)
				{
					output+=string("setge ")+getSizedRegister(regl,1)+"\n";
				}
				else
				{
					output+=string("setae ")+getSizedRegister(regl,1)+"\n";
				}
			}
			else if(op=="==")
			{
				output+=string("cmp ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
				output+=string("sete ")+getSizedRegister(regl,1)+"\n";
			}
			else if(op=="!=")
			{
				output+=string("cmp ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
				output+=string("setne ")+getSizedRegister(regl,1)+"\n";
			}
			else if(op=="&")
			{
				output+=string("and ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
			}
			else if(op=="^")
			{
				output+=string("xor ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
			}
			else if(op=="|")
			{
				output+=string("or ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
			}
			else if(op=="&&")
			{
				output+=string("cmp ")+getSizedRegister(regl,size)+",0\n";
				output+=string("setne ")+getSizedRegister(regl,1)+"\n";
				output+=string("cmp ")+getSizedRegister(regr,size)+",0\n";
				output+=string("setne ")+getSizedRegister(regr,1)+"\n";
				output+=string("and ")+getSizedRegister(regl,1)+","+getSizedRegister(regr,1)+"\n";
			}
			else if(op=="||")
			{
				output+=string("or ")+getSizedRegister(regl,size)+","+getSizedRegister(regr,size)+"\n";
				output+=string("cmp ")+getSizedRegister(regl,size)+",0\n";
				output+=string("setne ")+getSizedRegister(regl,1)+"\n";
			}
			
			return output;
		}
		string compileIntegerOperationLeftUnary(const string& op,const string& type,const string& reg)
		{
			string output;
			
			int size=getSizeOfTypeFromName(type,false);
			
			if(op=="-")
			{
				output+=string("neg ")+getSizedRegister(reg,size)+"\n";
			}
			else if(op=="~")
			{
				output+=string("not ")+getSizedRegister(reg,size)+"\n";
			}
			else if(op=="!")
			{
				output+=string("cmp ")+getSizedRegister(reg,size)+",0"+"\n";
				output+=string("sete ")+getSizedRegister(reg,1)+"\n";
			}
			
			return output;
		}
		string compileOperator(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue lvalue,ExpressionValue& value,int level,const string& op)
		{
			string expressionCode;
			
			ExpressionValue rvalue;
			expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,rvalue,level);
			
			LocalVariable vlvalue=localVariables[lvalue.localVariableIndex];
			LocalVariable vrvalue=localVariables[rvalue.localVariableIndex];
			
			if(vlvalue.type.pointerLevels==0 && vrvalue.type.pointerLevels==0 &&
				dataTypes[vlvalue.type.name].isInteger && dataTypes[vrvalue.type.name].isInteger)
			{
				//two integers
				
				Type type=Type("i64",0);
				if(lvalue.isIntegerLiteral && rvalue.isIntegerLiteral)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				else if(lvalue.isIntegerLiteral)
				{
					type=vrvalue.type;
				}
				else if(rvalue.isIntegerLiteral)
				{
					type=vlvalue.type;
				}
				else
				{
					if(vlvalue.type.name!=vrvalue.type.name)
					{
						code.addError(t,__LINE__);
						throw LogicError();
					}
					type=vlvalue.type;
				}
				
				Type resultType=type;
				if(isComparisonOperator(op))
				{
					resultType=Type("u8",0);
				}
				
				value=ExpressionValue();
				value.localVariableIndex=createTmp(localVariables,functionContext,resultType,false);
				
				string registerToUseL="rax";
				string registerToUseR="rcx";
				
				expressionCode+=movLocalToRegisterGetValue(vlvalue,registerToUseL);
				expressionCode+=movLocalToRegisterGetValue(vrvalue,registerToUseR);
				
				expressionCode+=compileIntegerOperation(op,type.name,registerToUseL,registerToUseR);
				
				expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUseL);
			}
			else if(vlvalue.type.pointerLevels>0 && vrvalue.type.pointerLevels>0 &&
				(vlvalue.type.pointerLevels==vrvalue.type.pointerLevels && vlvalue.type.name==vrvalue.type.name
					|| lvalue.isNullptr || rvalue.isNullptr))
			{
				//two pointers
				
				if(lvalue.isNullptr && rvalue.isNullptr)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				if(op=="-")
				{
					Type type=Type("i64",0);
					Type resultType=Type("i64",0);
					
					value=ExpressionValue();
					value.localVariableIndex=createTmp(localVariables,functionContext,resultType,false);
					
					string registerToUseL="rax";
					string registerToUseR="rcx";
					
					expressionCode+=movLocalToRegisterGetValue(vlvalue,registerToUseL);
					expressionCode+=movLocalToRegisterGetValue(vrvalue,registerToUseR);
					
					expressionCode+=compileIntegerOperation(op,type.name,registerToUseL,registerToUseR);
					
					Type pointedType=vlvalue.type;
					if(lvalue.isNullptr)
					{
						pointedType=vrvalue.type;
					}
					pointedType.pointerLevels--;
					
					expressionCode+=string("mov ")+registerToUseR+","+to_string(getTypeSize(pointedType,false))+"\n";
					
					expressionCode+=compileIntegerOperation("/",type.name,registerToUseL,registerToUseR);
					
					expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUseL);
				}
				else if(op=="==" || op=="!=")
				{
					Type type=Type("i64",0);
					Type resultType=Type("u8",0);
					
					value=ExpressionValue();
					value.localVariableIndex=createTmp(localVariables,functionContext,resultType,false);
					
					string registerToUseL="rax";
					string registerToUseR="rcx";
					
					expressionCode+=movLocalToRegisterGetValue(vlvalue,registerToUseL);
					expressionCode+=movLocalToRegisterGetValue(vrvalue,registerToUseR);
					
					expressionCode+=compileIntegerOperation(op,type.name,registerToUseL,registerToUseR);
					
					expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUseL);
				}
				else
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
			}
			else if(vlvalue.type.pointerLevels>0 && dataTypes[vrvalue.type.name].isInteger)
			{
				//A pointer and an integer
				
				if(lvalue.isNullptr)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				if(op=="+" || op=="-")
				{
					Type type=Type("i64",0);
					Type resultType=vlvalue.type;
					
					value=ExpressionValue();
					value.localVariableIndex=createTmp(localVariables,functionContext,resultType,false);
					
					string registerToUseL="rax";
					string registerToUseR="rcx";
					
					expressionCode+=movLocalToRegisterGetValue(vrvalue,registerToUseL);
					size_t rsize=getTypeSize(vrvalue.type,false);
					if(rsize!=pointerSize)
					{
						expressionCode+=signExtendRegister(registerToUseL,rsize,pointerSize);
					}
					
					Type pointedType=vlvalue.type;
					pointedType.pointerLevels--;
					expressionCode+=string("mov ")+registerToUseR+","+to_string(getTypeSize(pointedType,false))+"\n";
					expressionCode+=compileIntegerOperation("*",type.name,registerToUseL,registerToUseR);
					expressionCode+=string("mov ")+registerToUseR+","+registerToUseL+"\n";
					
					expressionCode+=movLocalToRegisterGetValue(vlvalue,registerToUseL);
					
					expressionCode+=compileIntegerOperation(op,type.name,registerToUseL,registerToUseR);
					
					expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUseL);
				}
				else
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
			}
			else
			{
				//Any other combination of types (overloaded operator)
				if(vlvalue.type.name=="dynamic" || vrvalue.type.name=="dynamic")
				{
					//TODO---- NOT IMPLEMENTED
					code.addError(t,__LINE__);//----
					throw LogicError();//----
				}
				else
				{
					vector<ExpressionValue> arguments=vector<ExpressionValue>{lvalue,rvalue};
					
					int functionIndex=findFunctionIndexWithArgumentsReturnMinus2IfCollision(-1,op,localVariables,arguments);
					if(functionIndex==-1)
					{
						code.addError(t,__LINE__);
						throw LogicError();
					}
					
					CodeTopFunction& functionToCall=functions[functionIndex];
					
					bool includeReturnReference= functionToCall.returnsSomething && isTypePassedByReference(functionToCall.returnType) && !functionToCall.returnsReference;
					
					ExpressionValue returnReference;
					if(includeReturnReference)
					{
						expressionCode+=createObjectAndGetReference(function,code,functionContext,localVariables,returnReference,functionToCall.returnType);
					}
					
					vector<int> args;
					if(includeReturnReference) args.push_back(returnReference.localVariableIndex);
					
					args.push_back(arguments[0].localVariableIndex);
					args.push_back(arguments[1].localVariableIndex);
					
					ExpressionValue returnValue;
					expressionCode+=compileCallFunction(function,code,functionContext,localVariables,-1,functionIndex,
						args,returnValue,functionToCall.returnsSomething && !includeReturnReference,functionToCall.returnType,functionToCall.returnsReference);
					
					if(includeReturnReference)
					{
						returnValue=returnReference;
					}
					
					value=returnValue;
				}
			}
			
			return expressionCode;
		}
		string compileLeftUnaryOperator(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue& value,const string& op)
		{
			string expressionCode;
			
			ExpressionValue valueInside;
			expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,valueInside,getMaximumOperatorLevel());
			
			LocalVariable v=localVariables[valueInside.localVariableIndex];
			
			if(v.type.pointerLevels==0 && dataTypes[v.type.name].isInteger)
			{
				//An integer
				
				Type type=Type("i64",0);
				if(valueInside.isIntegerLiteral)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				else
				{
					type=v.type;
				}
				
				Type resultType=type;
				if(op=="!")
				{
					resultType=Type("u8",0);
				}
				
				value=ExpressionValue();
				value.localVariableIndex=createTmp(localVariables,functionContext,resultType,false);
				
				string registerToUse="rax";
				
				expressionCode+=movLocalToRegisterGetValue(v,registerToUse);
				
				expressionCode+=compileIntegerOperationLeftUnary(op,type.name,registerToUse);
				
				expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUse);
			}
			else
			{
				//Any other type (overloaded operator)
				//TODO---- NOT IMPLEMENTED
				code.addError(t,__LINE__);//----
				throw LogicError();//----
			}
			
			return expressionCode;
		}
		string castPrimitive(const string& reg,Type initialType,Type resultType,bool& success)
		{
			string output;
			
			string from="i64";
			if(initialType.pointerLevels==0) from=initialType.name;
			string to="i64";
			if(resultType.pointerLevels==0) to=resultType.name;
			
			int fromSize=getSizeOfTypeFromName(from,false);
			int toSize=getSizeOfTypeFromName(to,false);
			if(fromSize==toSize)
			{
				success=true;
				return output;
			}
			else
			{
				bool isSigned=isSignedPrimitiveTypeName(from);
				if(isSignedPrimitiveTypeName(to)!=isSigned)
				{
					success=false;
					return output;
				}
				
				if(toSize>fromSize)
				{
					output+=signExtendRegister(reg,fromSize,toSize);
				}
				else
				{
					output+=zeroExtendRegister(reg,fromSize,toSize);
				}
			}
			
			success=true;
			return output;
		}
		string compileCastFromDynamic(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue inputValue,ExpressionValue& outputValue,Type type)
		{
			string output;
			
			outputValue=ExpressionValue();
			outputValue.localVariableIndex=createTmp(localVariables,functionContext,type,true);
			
			Type typePointer=type;
			typePointer.pointerLevels++;
			int localVariableIndex2=createTmp(localVariables,functionContext,typePointer,false);
			
			string registerToUse="rax";
			output+=movLocalAddressToRegister(localVariables[outputValue.localVariableIndex],0,registerToUse);
			output+=movRegisterToLocal(localVariables[localVariableIndex2],registerToUse);
			
			
			int localVariableIndex3=createTmp(localVariables,functionContext,Type("i64",0),false);
			output+=movImmediateToLocal(localVariables[localVariableIndex3],dataTypes.getIndexOf(type.name));
			
			ExpressionValue value;
			output+=compileCallFunction(function,code,functionContext,localVariables,
				-1,findFunctionIndex("__dynamic_cast_to_type"),vector<int>{inputValue.localVariableIndex,localVariableIndex2,localVariableIndex3},value,false,Type(),false);
			
			return output;
		}
		string compileAssignment(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue lvalue,ExpressionValue rvalue,bool& success)
		{
			string output;
			
			success=true;
			
			if(localVariables[lvalue.localVariableIndex].type.name=="dynamic" && localVariables[lvalue.localVariableIndex].type.pointerLevels==0)
			{
				output+=compileCallAssignmentOperatorOfLocal(function,code,t,functionContext,
					localVariables,lvalue.localVariableIndex,rvalue.localVariableIndex);
			}
			else
			{
				if(isAssignable(localVariables,localVariables[lvalue.localVariableIndex].type,rvalue))
				{
					LocalVariable lv=localVariables[lvalue.localVariableIndex];
					LocalVariable rv=localVariables[rvalue.localVariableIndex];
					
					int lsize=getTypeSize(lv.type,false);
					
					if(lv.type.pointerLevels>0 || dataTypes[lv.type.name].isInteger)
					{
						if(rv.type.pointerLevels==0 && rv.type.name=="dynamic")
						{
							ExpressionValue castValue;
							output+=compileCastFromDynamic(function,code,t,functionContext,localVariables,rvalue,castValue,lv.type);
							
							rv=localVariables[castValue.localVariableIndex];
						}
						
						if(lv.isReference)
						{
							string registerToUseL="rcx";
							string registerToUseR="rax";
							
							output+=movLocalToRegister(lv,registerToUseL);
							output+=movLocalToRegisterGetValue(rv,registerToUseR);
							
							output+=string("mov [")+registerToUseL+"],"+getSizedRegister(registerToUseR,lsize)+"\n";
						}
						else
						{
							string registerToUse="rax";
							
							output+=movLocalToRegisterGetValue(rv,registerToUse);
							output+=movRegisterToLocal(lv,registerToUse);
						}
					}
					else
					{
						output+=compileCallAssignmentOperatorOfLocal(function,code,t,functionContext,
							localVariables,lvalue.localVariableIndex,rvalue.localVariableIndex);
					}
				}
				else
				{
					success=false;
				}
			}
			
			return output;
		}
		string compileAssignmentOperatorUnary(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue value,const string& op)
		{
			string output;
			
			LocalVariable v=localVariables[value.localVariableIndex];
			
			if(v.type.pointerLevels>0 || dataTypes[v.type.name].isInteger)
			{
				string registerToUseL="rax";
				string registerToUseR="rcx";
				
				string opMnemonic="add";
				if(op=="--") opMnemonic="sub";
				
				int size=getTypeSize(v.type,false);
				
				int change=1;
				if(v.type.pointerLevels>0)
				{
					Type pointedType=v.type;
					pointedType.pointerLevels--;
					change=getTypeSize(pointedType,false);
				}
				
				output+=movLocalToRegisterGetValue(v,registerToUseR);
				
				output+=opMnemonic+" "+getSizedRegister(registerToUseR,size)+","+to_string(change)+"\n";
				
				if(v.isReference)
				{
					output+=movLocalToRegister(v,registerToUseL);
				}
				else
				{
					output+=movLocalAddressToRegister(v,0,registerToUseL);
				}
				
				output+=string("mov [")+registerToUseL+"],"+getSizedRegister(registerToUseR,size)+"\n";
			}
			else
			{
				//TODO---- NOT IMPLEMENTED
				code.addError(t,__LINE__);//----
				throw LogicError();//----
			}
			
			return output;
		}
		string compileAssignmentOperator(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue lvalue,const string& op)
		{
			string output;
			
			string internalOp=op.substr(0,op.size()-1);
			
			ExpressionValue rvalue;
			
			output+=compileOperator(function,code,t,functionContext,localVariables,lvalue,rvalue,0,internalOp);
			
			bool success=false;
			output+=compileAssignment(function,code,t,functionContext,localVariables,lvalue,rvalue,success);
			if(!success)
			{
				code.addError(t,__LINE__);
				throw LogicError();
			}
			
			return output;
		}
		string compileNonEmptyConstructorCallOfLocal(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			int localVariableIndex)
		{
			ExpressionValue thisValue;
			thisValue.localVariableIndex=localVariableIndex;
			
			int classIndex=findClassIndex(localVariables[localVariableIndex].type.name);
			
			ExpressionValue value;
			return compileFunctionCall(function,code,t,functionContext,localVariables,value,classIndex,-1,thisValue,true);
		}
		bool argumentsApplyToFunction(CodeTopFunction& function,NamedVector<LocalVariable>& localVariables,const vector<ExpressionValue>& arguments)
		{
			if(function.parameters.size()!=arguments.size()) return false;
			for(int i=0;i<arguments.size();i++)
			{
				if(!isAssignable(localVariables,function.parameters[i].type,arguments[i]))
				{
					return false;
				}
			}
			return true;
		}
		int findFunctionIndexWithArgumentsReturnMinus2IfCollision(int classIndex,string functionName,NamedVector<LocalVariable>& localVariables,const vector<ExpressionValue>& argRvalues)
		{
			int functionIndex=-1;
			if(classIndex==-1)
			{
				for(int i=0;i<functions.size();i++)
				{
					if(functions[i].name==functionName)
					{
						if(argumentsApplyToFunction(functions[i],localVariables,argRvalues))
						{
							if(functionIndex==-1)
							{
								functionIndex=i;
								break;
							}
							else
							{
								return -2;
							}
						}
					}
				}
			}
			else
			{
				for(int i=0;i<classes[classIndex].methods.size();i++)
				{
					if(classes[classIndex].methods[i].name==functionName)
					{
						if(argumentsApplyToFunction(classes[classIndex].methods[i],localVariables,argRvalues))
						{
							if(functionIndex==-1)
							{
								functionIndex=i;
								break;
							}
							else
							{
								return -2;
							}
						}
					}
				}
			}
			return functionIndex;
		}
		string createObjectAndGetReference(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue& outputReference,Type type)
		{
			string output;
			
			ExpressionValue value;
			value.localVariableIndex=createTmp(localVariables,functionContext,type,false);
			
			output+=compileCallConstructorOfLocal(function,code,functionContext,localVariables,value.localVariableIndex);
			
			outputReference=ExpressionValue();
			outputReference.localVariableIndex=createTmp(localVariables,functionContext,type,true);
			
			string registerToUse="rax";
			
			output+=movLocalAddressToRegister(localVariables[value.localVariableIndex],0,registerToUse);
			
			output+=movRegisterToLocal(localVariables[outputReference.localVariableIndex],registerToUse);
			
			return output;
		}
		string compileFunctionCall(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue& value,int classIndex,int functionIndex,const ExpressionValue thisValue,bool isNonEmptyConstructor=false)
		{
			string output;
			
			CodeTopFunction*functionToCall=nullptr;
			bool includeReturnReference=false;
			bool includeThisValue=false;
			if(isNonEmptyConstructor)
			{
				functionToCall=nullptr;
				includeReturnReference=false;
				includeThisValue=true;
			}
			else
			{
				functionToCall= classIndex==-1 ? &functions[functionIndex] : &classes[classIndex].methods[functionIndex];
				includeReturnReference= functionToCall->returnsSomething && isTypePassedByReference(functionToCall->returnType) && !functionToCall->returnsReference;
				includeThisValue= classIndex!=-1;
			}
			
			vector<ExpressionValue> arguments;
			
			if(includeReturnReference)
			{
				if(isNonEmptyConstructor)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				ExpressionValue returnReference;
				output+=createObjectAndGetReference(function,code,functionContext,localVariables,returnReference,functionToCall->returnType);
				
				arguments.push_back(returnReference);
			}
			
			if(includeThisValue)
			{
				arguments.push_back(thisValue);
			}
			
			if(code.get(t)!="(")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
			
			vector<ExpressionValue> argRvalues;
			
			for(int i=0;;i++)
			{
				if(code.get(t)==")")
				{
					break;
				}
				
				ExpressionValue argRvalue;
				
				output+=compileExpressionPart(function,code,t,functionContext,localVariables,argRvalue);
				
				argRvalues.push_back(argRvalue);
				
				if(code.get(t)==",")
				{
					t++;
				}
				else if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
			}
			
			if(code.get(t)!=")")
			{
				code.addError(t,__LINE__);
				throw ParseError();
			}
			t++;
			
			if(isNonEmptyConstructor)
			{
				functionIndex=findFunctionIndexWithArgumentsReturnMinus2IfCollision(classIndex,"constructor",localVariables,argRvalues);
				
				if(functionIndex==-1)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				else if(functionIndex==-2)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				functionToCall=&classes[classIndex].methods[functionIndex];
			}
			
			if(argRvalues.size()!=functionToCall->parameters.size())
			{
				code.addError(t,__LINE__);
				throw LogicError();
			}
			
			for(int i=0;i<argRvalues.size();i++)
			{
				ExpressionValue argRvalue=argRvalues[i];
				
				Type parameterType=functionToCall->parameters[i].type;
				
				bool passedByReference=isTypePassedByReference(parameterType);
				
				bool nonDynamicToDynamic=(parameterType.name=="dynamic" && parameterType.pointerLevels==0 && localVariables[argRvalue.localVariableIndex].type.name!="dynamic");
				
				if(nonDynamicToDynamic) passedByReference=false;
				
				ExpressionValue arg;
				arg.localVariableIndex=createTmp(localVariables,functionContext,parameterType,passedByReference);
				
				if(nonDynamicToDynamic)
				{
					output+=movImmediateToLocal(localVariables[arg.localVariableIndex],0);
					
					output+=compileCallAssignmentOperatorOfDynamicLocal(function,code,functionContext,localVariables,arg.localVariableIndex,argRvalue.localVariableIndex);
				}
				else
				{
					if(passedByReference)
					{
						LocalVariable argInput=localVariables[argRvalue.localVariableIndex];
						Type argumentType=argInput.type;
						if(argumentType.name!=parameterType.name && argumentType.pointerLevels!=parameterType.pointerLevels)
						{
							code.addError(t,__LINE__);
							throw LogicError();
						}
						
						LocalVariable argOutput=localVariables[arg.localVariableIndex];
						string registerToUse="rax";
						if(argInput.isReference)
						{
							output+=movLocalToRegister(argInput,registerToUse);
							output+=movRegisterToLocal(argOutput,registerToUse);
						}
						else
						{
							output+=movLocalAddressToRegister(argInput,0,registerToUse);
							output+=movRegisterToLocal(argOutput,registerToUse);
						}
					}
					else
					{
						bool success=false;
						output+=compileAssignment(function,code,t,functionContext,localVariables,arg,argRvalue,success);
						if(!success)
						{
							code.addError(t,__LINE__);
							throw LogicError();
						}
					}
				}
				
				arguments.push_back(arg);
			}
			
			vector<int> argumentIndexes;
			for(int i=0;i<arguments.size();i++)
			{
				argumentIndexes.push_back(arguments[i].localVariableIndex);
			}
			
			if(isNonEmptyConstructor)
			{
				output+=compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,
					argumentIndexes,value,false,Type(),false);
			}
			else
			{
				output+=compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,
					argumentIndexes,value,functionToCall->returnsSomething && !includeReturnReference,functionToCall->returnType,
						functionToCall->returnsReference);
			}
			
			if(includeReturnReference)
			{
				value=arguments[0];
			}
			
			return output;
		}
		string compileExpressionPart(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue& value,int level=0)
		{
			string expressionCode;
			
			string start=code.get(t);
			
			if(start=="(")
			{
				t++;
				
				expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,value);
				
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			else if(start=="addressof")
			{
				t++;
				
				if(code.get(t)!="(")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				ExpressionValue valueInside;
				
				expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,valueInside);
				
				LocalVariable v=localVariables[valueInside.localVariableIndex];
				
				Type resultType=v.type;
				resultType.pointerLevels++;
				
				value=ExpressionValue();
				
				value.localVariableIndex=createTmp(localVariables,functionContext,resultType,false);
				
				string registerToUse="rax";
				
				if(v.isReference)
				{
					expressionCode+=movLocalToRegister(v,registerToUse);
				}
				else
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUse);
				
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			else if(start=="deref")
			{
				t++;
				
				if(code.get(t)!="(")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				ExpressionValue valueInside;
				
				expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,valueInside);
				
				LocalVariable v=localVariables[valueInside.localVariableIndex];
				
				if(v.type.pointerLevels==0)
				{
					code.addError(t,__LINE__);
					throw LogicError();
				}
				
				Type resultType=v.type;
				resultType.pointerLevels--;
				
				value=ExpressionValue();
				
				value.localVariableIndex=createTmp(localVariables,functionContext,resultType,true);
				
				string registerToUse="rax";
				
				expressionCode+=movLocalToRegisterGetValue(v,registerToUse);
				
				expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUse);
				
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			else if(start=="sizeof" || start=="alignof" || start=="typeof")
			{
				t++;
				
				if(code.get(t)!="(")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				bool isType=false;
				Type type;
				if(parseCheckType(code,t,type))
				{
					isType=true;
				}
				
				int typeLocalVariableIndex=-1;
				
				if(!isType)
				{
					ExpressionValue valueInside;
					
					expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,valueInside);
					
					typeLocalVariableIndex=valueInside.localVariableIndex;
					
					LocalVariable v=localVariables[valueInside.localVariableIndex];
					
					type=v.type;
				}
				
				if(type.name=="dynamic" && type.pointerLevels==0 && !isType)
				{
					string functionName;
					if(start=="sizeof") functionName="__dynamic_sizeof";
					else if(start=="alignof") functionName="__dynamic_alignof";
					else if(start=="typeof") functionName="__dynamic_typeof";
					else
					{
						code.addError(t,__LINE__);
						throw LogicError();
					}
					
					int functionIndex=findFunctionIndex(functionName);
					if(functionIndex==-1)
					{
						code.addError(t,__LINE__);
						throw LogicError();
					}
					
					value=ExpressionValue();
					value.localVariableIndex=createTmp(localVariables,functionContext,Type("i64",0),false);
					
					int localVariableIndex=createTmp(localVariables,functionContext,Type("i64",1),false);
					
					string registerToUse="rax";
					expressionCode+=movLocalAddressToRegister(localVariables[value.localVariableIndex],0,registerToUse);
					expressionCode+=movRegisterToLocal(localVariables[localVariableIndex],registerToUse);
					
					ExpressionValue returnValue;
					expressionCode+=compileCallFunction(function,code,functionContext,localVariables,-1,functionIndex,
						vector<int>{typeLocalVariableIndex,localVariableIndex},returnValue,false,Type(),false);
				}
				else
				{
					int number=0;
					if(start=="sizeof")
					{
						if(type.pointerLevels>0)
						{
							number=pointerSize;
						}
						else
						{
							number=getTypeSize(type,false);
						}
					}
					else if(start=="alignof")
					{
						if(type.pointerLevels>0)
						{
							number=pointerAlignment;
						}
						else
						{
							number=getTypeAlignment(type,false);
						}
					}
					else if(start=="typeof")
					{
						if(type.pointerLevels>0)
						{
							code.addError(t,__LINE__);
							throw LogicError();
						}
						else
						{
							number=dataTypes.getIndexOf(type.name);
						}
					}
					
					expressionCode+=compileExpressionValueImmediate(functionContext,localVariables,value,Type("i64",0),number,false);
				}
				
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			else if(start=="__call" || start=="__call_destructor")
			{
				t++;
				
				if(code.get(t)!="(")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
				
				vector<ExpressionValue> argRvalues;
				
				for(int i=0;;i++)
				{
					if(code.get(t)==")")
					{
						break;
					}
					
					ExpressionValue argRvalue;
					
					expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,argRvalue);
					
					argRvalues.push_back(argRvalue);
					
					if(code.get(t)==",")
					{
						t++;
					}
					else if(code.get(t)!=")")
					{
						code.addError(t,__LINE__);
						throw ParseError();
					}
				}
				
				if(argRvalues.size()==0)
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				
				ExpressionValue returnTypeIndexValue;
				returnTypeIndexValue.localVariableIndex=createTmp(localVariables,functionContext,Type("i64",0),false);
				expressionCode+=movImmediateToLocal(localVariables[returnTypeIndexValue.localVariableIndex],-1);
				
				vector<int> argumentIndexes;
				for(int i=1;i<argRvalues.size();i++)
				{
					argumentIndexes.push_back(argRvalues[i].localVariableIndex);
				}
				
				expressionCode+=compileCallFunctionIndirect(function,code,functionContext,localVariables,
					argRvalues[0],argumentIndexes,value,returnTypeIndexValue,start=="__call_destructor");
				
				if(code.get(t)!=")")
				{
					code.addError(t,__LINE__);
					throw ParseError();
				}
				t++;
			}
			else
			{
				bool isCastOrConstructor=false;
				Type castOrConstructorType;
				if(parseCheckType(code,t,castOrConstructorType))
				{
					isCastOrConstructor=true;
				}
				
				if(isCastOrConstructor)
				{
					if(code.get(t)!="(")
					{
						Type type=castOrConstructorType;
						if(type.pointerLevels>0)
						{
							code.addError(t,__LINE__);
							throw LogicError();
						}
						else
						{
							int number=dataTypes.getIndexOf(type.name);
							
							expressionCode+=compileExpressionValueImmediate(functionContext,localVariables,value,Type("i64",0),number,false);
						}
					}
					else
					{
						if(code.get(t)!="(")
						{
							code.addError(t,__LINE__);
							throw ParseError();
						}
						t++;
						
						if(castOrConstructorType.pointerLevels>0 || dataTypes[castOrConstructorType.name].isPrimitive)
						{
							ExpressionValue valueInside;
							
							expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,valueInside);
							
							LocalVariable v=localVariables[valueInside.localVariableIndex];
							
							Type resultType=castOrConstructorType;
							
							if(v.type.name=="dynamic" && v.type.pointerLevels==0)
							{
								Type typeToCastTo=resultType;
								if(typeToCastTo.pointerLevels>0)
								{
									typeToCastTo=Type("i64",0);
								}
								
								int functionIndex=findFunctionIndex("__dynamic_cast_integer_to_integer");
								if(functionIndex==-1)
								{
									code.addError(t,__LINE__);
									throw LogicError();
								}
								
								value=ExpressionValue();
								value.localVariableIndex=createTmp(localVariables,functionContext,resultType,false);
								
								int localVariableIndex2=createTmp(localVariables,functionContext,Type("i64",1),false);
								
								string registerToUse="rax";
								expressionCode+=movLocalAddressToRegister(localVariables[value.localVariableIndex],0,registerToUse);
								expressionCode+=movRegisterToLocal(localVariables[localVariableIndex2],registerToUse);
								
								int localVariableIndex3=createTmp(localVariables,functionContext,Type("i64",0),false);
								expressionCode+=movImmediateToLocal(localVariables[localVariableIndex3],dataTypes.getIndexOf(typeToCastTo.name));
								
								ExpressionValue returnValue;
								expressionCode+=compileCallFunction(function,code,functionContext,localVariables,-1,functionIndex,
									vector<int>{valueInside.localVariableIndex,localVariableIndex2,localVariableIndex3},returnValue,false,Type(),false);
							}
							else
							{
								if(v.type.pointerLevels==0 && !dataTypes[v.type.name].isPrimitive)
								{
									code.addError(t,__LINE__);
									throw LogicError();
								}
								
								value=ExpressionValue();
								
								value.localVariableIndex=createTmp(localVariables,functionContext,resultType,false);
								
								string registerToUse="rax";
								
								expressionCode+=movLocalToRegisterGetValue(v,registerToUse);
								
								if(valueInside.isIntegerLiteral)
								{
									bool success=false;
									expressionCode+=castPrimitive(registerToUse,v.type,resultType,success);
									if(!success)
									{
										code.addError(t,__LINE__);
										throw LogicError();
									}
								}
								
								expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUse);
							}
						}
						else
						{
							if(code.get(t)==")")
							{
								Type type=castOrConstructorType;
								
								value=ExpressionValue();
								
								value.localVariableIndex=createTmp(localVariables,functionContext,type,false);
								
								expressionCode+=compileCallConstructorOfLocal(function,code,functionContext,localVariables,value.localVariableIndex);
							}
							else
							{
								Type type=castOrConstructorType;
								
								value=ExpressionValue();
								
								value.localVariableIndex=createTmp(localVariables,functionContext,type,false);
								
								t--;
								expressionCode+=compileNonEmptyConstructorCallOfLocal(function,code,t,functionContext,localVariables,value.localVariableIndex);
								t--;
							}
						}
						
						if(code.get(t)!=")")
						{
							code.addError(t,__LINE__);
							throw ParseError();
						}
						t++;
					}
				}
				else if(start=="nullptr")
				{
					expressionCode+=compileExpressionValueImmediate(functionContext,localVariables,value,Type("u8",1),0,false);
					
					value.isNullptr=true;
					
					t++;
				}
				else if(start=="false")
				{
					expressionCode+=compileExpressionValueImmediate(functionContext,localVariables,value,Type("i64",0),0,true);
					t++;
				}
				else if(start=="true")
				{
					expressionCode+=compileExpressionValueImmediate(functionContext,localVariables,value,Type("i64",0),1,true);
					t++;
				}
				else if(start=="__datatype_count" || start=="__datatype_start")
				{
					value=ExpressionValue();
					value.localVariableIndex=createTmp(localVariables,functionContext,Type("i64",1),false);
					
					string labelName;
					if(start=="__datatype_count") labelName="datatype_count";
					else if(start=="__datatype_start") labelName="datatype_start";
					else
					{
						code.addError(t,__LINE__);
						throw LogicError();
					}
					
					string registerToUse="rax";
					expressionCode+=string("mov ")+registerToUse+","+labelName+"\n";
					expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUse);
					
					t++;
				}
				else
				{
					bool isInteger=false;
					uint64_t integer=0;
					if(start=="__LINE__")
					{
						isInteger=true;
						integer=code.tokens[t].line;
						t++;
					}
					else if(parseCheckIntegerToken(code,t,integer))
					{
						isInteger=true;
					}
					else if(code.get(t)=="-")
					{
						t++;
						if(parseCheckIntegerToken(code,t,integer))
						{
							isInteger=true;
							integer=-integer;
						}
						else
						{
							t--;
						}
					}
					
					if(isInteger)
					{
						expressionCode+=compileExpressionValueImmediate(functionContext,localVariables,value,Type("i64",0),integer,true);
					}
					else if(start=="-" || start=="!" || start=="~")
					{
						string op=start;
						t++;
						expressionCode+=compileLeftUnaryOperator(function,code,t,functionContext,localVariables,value,op);
					}
					else
					{
						bool isStringLiteral=false;
						string literal;
						if(parseCheckStringLiteralToken(code,t,literal))
						{
							isStringLiteral=true;
							
							int stringIndex=createAssemblyString(literal);
							
							expressionCode+=compileExpressionValueImmediate(functionContext,localVariables,value,Type("u8",1),0,false,stringIndex);
							
							string stringClassName="string";
							
							int classIndex=findClassIndex(stringClassName);
							if(classIndex==-1)
							{
								code.addError(t-1,__LINE__);
								throw LogicError();
							}
							int functionIndex=-1;
							for(int i=0;i<classes[classIndex].methods.size();i++)
							{
								CodeTopFunction& method=classes[classIndex].methods[i];
								if(method.name=="constructor")
								{
									if(!method.returnsSomething && method.parameters.size()==1)
									{
										Type type=method.parameters[0].type;
										if(type.name=="u8" && type.pointerLevels==1)
										{
											functionIndex=i;
											break;
										}
									}
								}
							}
							if(functionIndex==-1)
							{
								code.addError(t-1,__LINE__);
								throw LogicError();
							}
							
							ExpressionValue stringValue;
							stringValue.localVariableIndex=createTmp(localVariables,functionContext,Type(stringClassName,0),false);
							
							ExpressionValue returnValue;
							expressionCode+=compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,
								vector<int>{stringValue.localVariableIndex,value.localVariableIndex},returnValue,false,Type(),false);
							
							value=stringValue;
						}
						else if(code.get(t)=="r")
						{
							t++;
							if(parseCheckStringLiteralToken(code,t,literal))
							{
								isStringLiteral=true;
								
								int stringIndex=createAssemblyString(literal);
								
								expressionCode+=compileExpressionValueImmediate(functionContext,localVariables,value,Type("u8",1),0,false,stringIndex);
							}
							else
							{
								t--;
							}
						}
						else if(code.get(t)=="b")
						{
							t++;
							if(parseCheckStringLiteralToken(code,t,literal))
							{
								isStringLiteral=true;
								
								if(literal.size()==0)
								{
									code.addError(t-1,__LINE__);
									throw LogicError();
								}
								
								expressionCode+=compileExpressionValueImmediate(functionContext,localVariables,value,Type("u8",0),uint8_t(literal[0]),false);
							}
							else
							{
								t--;
							}
						}
						
						if(!isStringLiteral)
						{
							bool isFunctionCall=false;
							
							if(functionContext.isMethod)
							{
								int functionIndex=findMethodIndex(functionContext.classIndex,start);
								if(functionIndex!=-1)
								{
									isFunctionCall=true;
									
									t++;
									
									ExpressionValue thisValue;
									thisValue.localVariableIndex=localVariables.getIndexOf("this");
									
									expressionCode+=compileFunctionCall(function,code,t,functionContext,localVariables,value,
										functionContext.classIndex,functionIndex,thisValue);
								}
							}
							
							if(!isFunctionCall)
							{
								int functionIndex=findFunctionIndex(start);
								if(functionIndex!=-1)
								{
									isFunctionCall=true;
									
									t++;
									
									expressionCode+=compileFunctionCall(function,code,t,functionContext,localVariables,value,
										-1,functionIndex,ExpressionValue());
								}
							}
							
							
							if(!isFunctionCall)
							{
								string variableName=start;
								bool isGlobal=false;
								bool isReferenceLocal=false;
								
								string registerToUse="rax";
								
								LocalVariable v;
								int globalVariableIndex=-1;
								Type type;
								
								int offset=0;
								
								bool isClassAttribute=false;
								
								if(functionContext.isMethod)
								{
									CodeTopClassAttribute*thisClassAttribute=findClassAttribute(Type(classes[functionContext.classIndex].name,0),variableName);
									if(thisClassAttribute!=nullptr)
									{
										isClassAttribute=true;
										
										v=localVariables["this"];
										type=thisClassAttribute->type;
										offset=thisClassAttribute->objectOffset;
										isReferenceLocal=v.isReference;
									}
								}
								
								if(!isClassAttribute)
								{
									try
									{
										v=localVariables[variableName];
										type=v.type;
										isReferenceLocal=v.isReference;
									}
									catch(...)
									{
										globalVariableIndex=findGlobalVariableIndex(variableName);
										if(globalVariableIndex==-1)
										{
											code.addError(t,__LINE__);
											throw NameError();
										}
										isGlobal=true;
										type=globalVariables[globalVariableIndex].type;
									}
								}
								
								t++;
								
								bool isMethodCall=false;
								int methodClassIndex=-1;
								int methodFunctionIndex=-1;
								
								if(!(type.name=="dynamic" && type.pointerLevels==0))
								{
									Type baseType=type;
									while(code.get(t)==".")
									{
										t++;
										string attribute=code.get(t);
										
										Type attributeType;
										int offsetToAdd=getOffsetAndTypeOfAttribute(baseType,attribute,attributeType);
										
										if(offsetToAdd==-1)
										{
											if(baseType.pointerLevels>0)
											{
												code.addError(t,__LINE__);
												throw NameError();
											}
											
											methodClassIndex=findClassIndex(baseType.name);
											int functionIndex=findMethodIndex(methodClassIndex,attribute);
											if(functionIndex!=-1)
											{
												isMethodCall=true;
												
												t++;
												
												methodFunctionIndex=functionIndex;
												
												break;
											}
											else
											{
												code.addError(t,__LINE__);
												throw NameError();
											}
										}
										
										offset+=offsetToAdd;
										baseType=attributeType;
										
										t++;
									}
									type=baseType;
								}
								
								if(isGlobal)
								{
									expressionCode+=movGlobalAddressToRegister(globalVariableIndex,offset,registerToUse);
								}
								else
								{
									if(isReferenceLocal)
									{
										expressionCode+=movLocalToRegister(v,registerToUse);
										if(offset!=0)
										{
											expressionCode+=string("add ")+registerToUse+","+to_string(offset)+"\n";
										}
									}
									else
									{
										expressionCode+=movLocalAddressToRegister(v,offset,registerToUse);
									}
								}
								
								value=ExpressionValue();
								
								value.localVariableIndex=createTmp(localVariables,functionContext,type,true);
								
								expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUse);
								
								if(isMethodCall)
								{
									ExpressionValue thisValue=value;
									
									expressionCode+=compileFunctionCall(function,code,t,functionContext,localVariables,value,
										methodClassIndex,methodFunctionIndex,thisValue);
								}
							}
						}
					}
				}
			}
			
			while(true)
			{
				string token=code.get(t);
				if(token==";" || token=="," || token==")" || token=="]" || token=="}")
				{
					break;
				}
				
				if(token==".")
				{
					LocalVariable v=localVariables[value.localVariableIndex];
					t++;
					string attribute=code.get(t);
					
					if(v.type.name=="dynamic" && v.type.pointerLevels==0)
					{
						int identifierIndex=getIdentifierIndex(attribute);
						if(identifierIndex==-1)
						{
							code.addError(t,__LINE__);
							throw NameError();
						}
						
						if(code.get(t+1)=="(")
						{
							t++;
							
							int functionIndex=findFunctionIndex("__dynamic_get_method");
							if(functionIndex==-1)
							{
								code.addError(t,__LINE__);
								throw LogicError();
							}
							
							ExpressionValue methodValue=ExpressionValue();
							methodValue.localVariableIndex=createTmp(localVariables,functionContext,Type("i64",0),false);
							
							int localVariableIndex2=createTmp(localVariables,functionContext,Type("i64",1),false);
							{
								string registerToUse="rax";
								expressionCode+=movLocalAddressToRegister(localVariables[methodValue.localVariableIndex],0,registerToUse);
								expressionCode+=movRegisterToLocal(localVariables[localVariableIndex2],registerToUse);
							}
							
							int localVariableIndex3=createTmp(localVariables,functionContext,Type("i64",0),false);
							expressionCode+=movImmediateToLocal(localVariables[localVariableIndex3],identifierIndex);
							
							{
								ExpressionValue returnValue;
								expressionCode+=compileCallFunction(function,code,functionContext,localVariables,-1,functionIndex,
									vector<int>{value.localVariableIndex,localVariableIndex2,localVariableIndex3},returnValue,false,Type(),false);
							}
							
							t++;
							
							vector<ExpressionValue> argRvalues;
							
							for(int i=0;;i++)
							{
								if(code.get(t)==")")
								{
									break;
								}
								
								ExpressionValue argRvalue;
								
								expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,argRvalue);
								
								argRvalues.push_back(argRvalue);
								
								if(code.get(t)==",")
								{
									t++;
								}
								else if(code.get(t)!=")")
								{
									code.addError(t,__LINE__);
									throw ParseError();
								}
							}
							
							if(code.get(t)!=")")
							{
								code.addError(t,__LINE__);
								throw ParseError();
							}
							t++;
							
							{
								functionIndex=findFunctionIndex("__dynamic_function_check_number_of_arguments");
								int argCountLocalVariableIndex=createTmp(localVariables,functionContext,Type("i64",0),false);
								expressionCode+=movImmediateToLocal(localVariables[argCountLocalVariableIndex],argRvalues.size());
								
								ExpressionValue returnValue;
								expressionCode+=compileCallFunction(function,code,functionContext,localVariables,-1,functionIndex,
									vector<int>{methodValue.localVariableIndex,argCountLocalVariableIndex},returnValue,false,Type(),false);
							}
							
							for(int i=0;i<argRvalues.size();i++)
							{
								functionIndex=findFunctionIndex("__dynamic_function_check_parameter_type");
								
								int iLocalVariableIndex=createTmp(localVariables,functionContext,Type("i64",0),false);
								expressionCode+=movImmediateToLocal(localVariables[iLocalVariableIndex],i);
								
								LocalVariable arg=localVariables[argRvalues[i].localVariableIndex];
								if(arg.type.pointerLevels>0)
								{
									code.addError(t,__LINE__);
									throw LogicError();
								}
								
								int typeLocalVariableIndex=createTmp(localVariables,functionContext,Type("i64",0),false);
								expressionCode+=movImmediateToLocal(localVariables[typeLocalVariableIndex],dataTypes.getIndexOf(arg.type.name));
								
								ExpressionValue returnValue;
								expressionCode+=compileCallFunction(function,code,functionContext,localVariables,-1,functionIndex,
									vector<int>{methodValue.localVariableIndex,iLocalVariableIndex,typeLocalVariableIndex},returnValue,false,Type(),false);
							}
							
							ExpressionValue functionAddressValue;
							functionAddressValue.localVariableIndex=createTmp(localVariables,functionContext,Type("i64",0),false);
							
							ExpressionValue returnTypeIndexValue;
							returnTypeIndexValue.localVariableIndex=createTmp(localVariables,functionContext,Type("i64",0),false);
							
							{
								int functionIndex=findFunctionIndex("__dynamic_function_get_address_and_return_type");
								if(functionIndex==-1)
								{
									code.addError(t,__LINE__);
									throw LogicError();
								}
								
								int aptrLocalVariableIndex=createTmp(localVariables,functionContext,Type("i64",1),false);
								{
									string registerToUse="rax";
									expressionCode+=movLocalAddressToRegister(localVariables[functionAddressValue.localVariableIndex],0,registerToUse);
									expressionCode+=movRegisterToLocal(localVariables[aptrLocalVariableIndex],registerToUse);
								}
								
								int tptrLocalVariableIndex=createTmp(localVariables,functionContext,Type("i64",1),false);
								{
									string registerToUse="rax";
									expressionCode+=movLocalAddressToRegister(localVariables[returnTypeIndexValue.localVariableIndex],0,registerToUse);
									expressionCode+=movRegisterToLocal(localVariables[tptrLocalVariableIndex],registerToUse);
								}
								
								ExpressionValue returnValue;
								expressionCode+=compileCallFunction(function,code,functionContext,localVariables,-1,functionIndex,
									vector<int>{methodValue.localVariableIndex,aptrLocalVariableIndex,tptrLocalVariableIndex},returnValue,false,Type(),false);
							}
							
							vector<int> args;
							args.push_back(value.localVariableIndex);
							for(int i=0;i<argRvalues.size();i++)
							{
								args.push_back(argRvalues[i].localVariableIndex);
							}
							
							ExpressionValue returnValue;
							expressionCode+=compileCallFunctionIndirect(function,code,functionContext,localVariables,functionAddressValue,
								args,returnValue,returnTypeIndexValue,attribute=="destructor");
							
							value=returnValue;
						}
						else
						{
							int functionIndex=findFunctionIndex("__dynamic_get_attribute");
							if(functionIndex==-1)
							{
								code.addError(t,__LINE__);
								throw LogicError();
							}
							
							ExpressionValue resultValue=ExpressionValue();
							resultValue.localVariableIndex=createTmp(localVariables,functionContext,Type("dynamic",0),false);
							expressionCode+=movImmediateToLocal(localVariables[resultValue.localVariableIndex],0);
							
							int localVariableIndex3=createTmp(localVariables,functionContext,Type("i64",0),false);
							expressionCode+=movImmediateToLocal(localVariables[localVariableIndex3],identifierIndex);
							
							ExpressionValue returnValue;
							expressionCode+=compileCallFunction(function,code,functionContext,localVariables,-1,functionIndex,
								vector<int>{value.localVariableIndex,resultValue.localVariableIndex,localVariableIndex3},returnValue,false,Type(),false);
							
							value=resultValue;
							
							t++;
						}
					}
					else
					{
						Type baseType=v.type;
						
						bool isMethodCall=false;
						int methodClassIndex=-1;
						int methodFunctionIndex=-1;
						
						Type attributeType;
						int offset=getOffsetAndTypeOfAttribute(baseType,attribute,attributeType);
						if(offset==-1)
						{
							if(baseType.pointerLevels>0)
							{
								code.addError(t,__LINE__);
								throw NameError();
							}
							
							methodClassIndex=findClassIndex(baseType.name);
							int functionIndex=findMethodIndex(methodClassIndex,attribute);
							if(functionIndex!=-1)
							{
								isMethodCall=true;
								methodFunctionIndex=functionIndex;
							}
							else
							{
								code.addError(t,__LINE__);
								throw NameError();
							}
						}
						
						t++;
						
						if(isMethodCall)
						{
							ExpressionValue thisValue=value;
							
							expressionCode+=compileFunctionCall(function,code,t,functionContext,localVariables,value,
								methodClassIndex,methodFunctionIndex,thisValue);
						}
						else
						{
							string registerToUse="rax";
							
							if(v.isReference)
							{
								expressionCode+=movLocalToRegister(v,registerToUse);
							}
							else
							{
								expressionCode+=movLocalAddressToRegister(v,0,registerToUse);
							}
							
							expressionCode+=string("add ")+registerToUse+","+to_string(offset)+"\n";
							
							value=ExpressionValue();
							
							value.localVariableIndex=createTmp(localVariables,functionContext,attributeType,true);
							
							expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUse);
						}
					}
				}
				else if(token=="[")
				{
					t++;
					
					LocalVariable v=localVariables[value.localVariableIndex];
					
					if(v.type.pointerLevels>0)
					{
						ExpressionValue lvalue=value;
						
						expressionCode+=compileOperator(function,code,t,functionContext,localVariables,lvalue,value,0,"+");
						
						ExpressionValue pvalue=value;
						
						LocalVariable p=localVariables[pvalue.localVariableIndex];
						
						if(p.type.pointerLevels==0)
						{
							code.addError(t,__LINE__);
							throw LogicError();
						}
						Type resultType=p.type;
						resultType.pointerLevels--;
						
						string registerToUse="rax";
						
						expressionCode+=movLocalToRegisterGetValue(p,registerToUse);
						
						value=ExpressionValue();
						value.localVariableIndex=createTmp(localVariables,functionContext,resultType,true);
						
						expressionCode+=movRegisterToLocal(localVariables[value.localVariableIndex],registerToUse);
					}
					else if(!dataTypes[v.type.name].isInteger)
					{
						if(v.type.name=="dynamic")
						{
							//TODO---- NOT IMPLEMENTED
							code.addError(t,__LINE__);//----
							throw LogicError();//----
						}
						else
						{
							ExpressionValue argument;
							expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,argument);
							
							int classIndex=findClassIndex(v.type.name);
							int functionIndex=findFunctionIndexWithArgumentsReturnMinus2IfCollision(classIndex,"[]",localVariables,vector<ExpressionValue>{argument});
							if(functionIndex==-1)
							{
								code.addError(t,__LINE__);
								throw LogicError();
							}
							
							CodeTopFunction& method=classes[classIndex].methods[functionIndex];
							
							bool includeReturnReference= method.returnsSomething && isTypePassedByReference(method.returnType) && !method.returnsReference;
							
							ExpressionValue returnReference;
							if(includeReturnReference)
							{
								expressionCode+=createObjectAndGetReference(function,code,functionContext,localVariables,returnReference,method.returnType);
							}
							
							vector<int> args;
							if(includeReturnReference) args.push_back(returnReference.localVariableIndex);
							
							args.push_back(value.localVariableIndex);
							args.push_back(argument.localVariableIndex);
							
							ExpressionValue returnValue;
							expressionCode+=compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,
								args,returnValue,method.returnsSomething && !includeReturnReference,method.returnType,method.returnsReference);
							
							if(includeReturnReference)
							{
								returnValue=returnReference;
							}
							
							value=returnValue;
						}
					}
					else
					{
						code.addError(t,__LINE__);
						throw LogicError();
					}
					
					if(code.get(t)!="]")
					{
						code.addError(t,__LINE__);
						throw ParseError();
					}
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
							ExpressionValue newValue;
							expressionCode+=compileOperator(function,code,t,functionContext,localVariables,value,newValue,operatorLevel,"==");
							value=newValue;
						}
						else
						{
							t--;
							break;
						}
					}
					else
					{
						ExpressionValue rvalue;
						
						expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,rvalue);
						
						bool success=false;
						expressionCode+=compileAssignment(function,code,t,functionContext,localVariables,value,rvalue,success);
						if(!success)
						{
							code.addError(t,__LINE__);
							throw LogicError();
						}
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
								expressionCode+=compileAssignmentOperatorUnary(function,code,t,functionContext,localVariables,value,op);
							}
							else
							{
								expressionCode+=compileAssignmentOperator(function,code,t,functionContext,localVariables,value,op);
							}
						}
						else
						{
							int operatorLevel=getOperatorLevel(op);
							if(operatorLevel>level)
							{
								t+=operatorTokens;
								ExpressionValue newValue;
								expressionCode+=compileOperator(function,code,t,functionContext,localVariables,value,newValue,operatorLevel,op);
								value=newValue;
							}
							else
							{
								break;
							}
						}
					}
					else
					{
						code.addError(t,__LINE__);
						throw ParseError();
					}
				}
			}
			
			return expressionCode;
		}
	public:
};



class Compiler
{
	public:
	
	
	
	string compile(const string& inputCode)
	{
		TokenizedCode code(inputCode);
		
		try
		{
			CodeTop codeTop(code);
			if(code.hasErrors())
			{
				code.printErrors();
				return "";
			}
			
			codeTop.processParsedData(code);
			if(code.hasErrors())
			{
				code.printErrors();
				return "";
			}
			
			AssemblyCode assemblyCode;
			codeTop.compile(code,assemblyCode);
			if(code.hasErrors())
			{
				code.printErrors();
				return "";
			}
			
			string outputCode=assemblyCode.getOutput();
			
			return outputCode;
		}
		catch(ParseError e)
		{
			if(code.hasErrors()) code.printErrors();
			else cout<<"FATAL ERROR(ParseError)"<<endl;
			return "";
		}
		catch(NameError e)
		{
			if(code.hasErrors()) code.printErrors();
			else cout<<"FATAL ERROR(NameError)"<<endl;
			return "";
		}
		catch(LogicError e)
		{
			if(code.hasErrors()) code.printErrors();
			else cout<<"FATAL ERROR(LogicError)"<<endl;
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
		throw string("Expected 2 arguments. Example: './bolgegc code.c code.asm'");
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

