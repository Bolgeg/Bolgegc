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
		
		while(true)
		{
			if(p>=inputCode.size()) break;
			
			uint8_t c=inputCode[p];
			
			if(c=='\n')
			{
				insideToken=false;
				lineNumber++;
				lineStart=p+1;
			}
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
						tokens.emplace_back(string(1,c),lineNumber,1+p-lineStart);
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
					tokens.emplace_back(string(1,c),lineNumber,1+p-lineStart);
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
		return nameToIndex.at(name);
	}
	
	T& operator [](size_t index)
	{
		return elements.at(index);
	}
	T& operator [](const string& name)
	{
		return elements[nameToIndex.at(name)];
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
	
	string name;
	
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
			returnType=Type(code,t);
			
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
	
	int functionLocalIndexStart=0;
	int scopeLocalIndexStart=0;
	
	int labelsCreated=0;
	
	LoopContext loopContext;
	
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
};

class ExpressionValue
{
	public:
	
	int localVariableIndex=-1;
	
	bool isNullptr=false;
	
	bool isIntegerLiteral=false;
};

class CodeTop
{
	public:
	
	vector<CodeTopFunction> functions;
	vector<CodeTopClass> classes;
	vector<CodeTopGlobalVariable> globalVariables;
	
	NamedVector<DataType> dataTypes;
	size_t pointerSize=8;
	size_t pointerAlignment=8;
	
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
		dataTypes.add(DataType("u8",true,true,true,1,1));
		dataTypes.add(DataType("u16",true,true,true,2,2));
		dataTypes.add(DataType("u32",true,true,true,4,4));
		dataTypes.add(DataType("u64",true,true,true,8,8));
		dataTypes.add(DataType("i8",true,true,true,1,1));
		dataTypes.add(DataType("i16",true,true,true,2,2));
		dataTypes.add(DataType("i32",true,true,true,4,4));
		dataTypes.add(DataType("i64",true,true,true,8,8));
		
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
			for(int i=0;i<classes[classIndex].methods.size();i++)
			{
				if(classes[classIndex].methods[i].name==methodName)
				{
					return i;
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
		
		//TODO----
		
		assembly.startCode=string("_start:\n")
			+"mov "+parameterRegisters[0]+",[rsp]"+"\n"
			+"lea "+parameterRegisters[1]+",[rsp+8]"+"\n"
			+"call "+getAssemblyFunctionName(findFunctionIndex("main"))+"\n"
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
			+assembly.stringCode;
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
			if(findMethodIndex(classIndex,"constructor")==-1)
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
				int inputParameterIndex=0;
				
				if(function.returnsSomething)
				{
					functionContext.indexOfVariableToReturn=localVariables.size();
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
			
			functionContext.functionLocalIndexStart=localVariables.size();
			
			functionContext.enterScope(localVariables.size());
		}
		void compileFunctionEnd(int classIndex,int functionIndex,CodeTopFunction& function,TokenizedCode& code,
			string& functionStartCode,string& functionCode,FunctionContext& functionContext)
		{
			NamedVector<LocalVariable>& localVariables=functionContext.baseLocalVariables;
			
			if(function.returnsSomething)
			{
				if(!isTypePassedByReference(function.returnType))
				{
					size_t size=0;
					size_t alignment=1;
					getTypeSizeAndAlignment(function.returnType,size,alignment,false);
					
					functionCode+=movLocalToRegister(localVariables[functionContext.indexOfVariableToReturn],0,size,returnRegister);
				}
			}
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
					classIndex,findMethodIndex(classIndex,"__dc"),vector<int>{localVariables.getIndexOf("this")},value,false,Type());
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
			
			//TODO----
			
			functionCode+=".return:\n";
			
			if(functionContext.isMethod && function.name=="destructor")
			{
				ExpressionValue value;
				functionCode+=compileCallFunction(function,code,functionContext,localVariables,
					classIndex,findMethodIndex(classIndex,"__dd"),vector<int>{localVariables.getIndexOf("this")},value,false,Type());
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
					int classIndex=findClassIndex(localVariables[localVariableIndex].type.name);
					if(classIndex==-1)
					{
						code.addError(-1,__LINE__);
						throw LogicError();
					}
					int functionIndex=findMethodIndex(classIndex,methodName);
					if(functionIndex==-1)
					{
						code.addError(-1,__LINE__);
						throw LogicError();
					}
					ExpressionValue value;
					return compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,vector<int>{localVariableIndex},value,false,Type());
				}
			}
			return string();
		}
		string compileCallAssignmentOperatorOfLocal(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,
			NamedVector<LocalVariable>& localVariables,int localVariableIndex,int localVariableIndex2)
		{
			int classIndex=findClassIndex(localVariables[localVariableIndex].type.name);
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
			return compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,vector<int>{localVariableIndex,localVariableIndex2},value,false,Type());
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
			
			LocalVariable& thisv=localVariables["this"];
			
			LocalVariable& otherv=localVariables["other"];
			
			CodeTopClassAttribute& attribute=classes[functionContext.classIndex].attributes[attributeIndex];
			
			string registerToUse="rax";
			
			
			int localVariableIndex=createTmp(localVariables,functionContext,attribute.type,true);
			
			LocalVariable& v=localVariables[localVariableIndex];
			
			output+=movLocalToRegister(thisv,registerToUse);
			output+=string("add ")+registerToUse+","+to_string(attribute.objectOffset)+"\n";
			output+=movRegisterToLocal(v,registerToUse);
			
			
			int localVariableIndex2=createTmp(localVariables,functionContext,attribute.type,true);
			
			LocalVariable& v2=localVariables[localVariableIndex2];
			
			output+=movLocalToRegister(otherv,registerToUse);
			output+=string("add ")+registerToUse+","+to_string(attribute.objectOffset)+"\n";
			output+=movRegisterToLocal(v2,registerToUse);
			
			
			if(isTypeWithAssignmentOperator(attribute.type))
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
				output+=compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,vector<int>{localVariableIndex,localVariableIndex2},value,false,Type());
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
			
			LocalVariable& thisv=localVariables["this"];
			
			CodeTopClassAttribute& attribute=classes[functionContext.classIndex].attributes[attributeIndex];
			
			if(isTypeWithConstructorAndDestructor(attribute.type))
			{
				int localVariableIndex=createTmp(localVariables,functionContext,attribute.type,true);
				
				LocalVariable& v=localVariables[localVariableIndex];
				
				string registerToUse="rax";
				
				output+=movLocalToRegister(thisv,registerToUse);
				output+=string("add ")+registerToUse+","+to_string(attribute.objectOffset)+"\n";
				output+=movRegisterToLocal(v,registerToUse);
				
				int classIndex=findClassIndex(v.type.name);
				if(classIndex==-1)
				{
					code.addError(-1,__LINE__);
					throw LogicError();
				}
				int functionIndex=findMethodIndex(classIndex,methodName);
				if(functionIndex==-1)
				{
					code.addError(-1,__LINE__);
					throw LogicError();
				}
				ExpressionValue value;
				output+=compileCallFunction(function,code,functionContext,localVariables,classIndex,functionIndex,vector<int>{localVariableIndex},value,false,Type());
			}
			
			return output;
		}
		string compileCallFunction(CodeTopFunction& function,TokenizedCode& code,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			int classIndex,int functionIndex,const vector<int>& argumentIndexes,ExpressionValue& value,bool getReturnValue,const Type& returnValueType)
		{
			string output;
			
			{
				int parameterStackSize=0;
				
				for(int parameterIndex=0;parameterIndex<argumentIndexes.size();parameterIndex++)
				{
					string registerToUse="rax";
					if(parameterIndex<parameterRegisters.size())
					{
						registerToUse=parameterRegisters[parameterIndex];
					}
					
					LocalVariable& v=localVariables[argumentIndexes[parameterIndex]];
					
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
						
						output+=string("mov ")+"[rsp+"+to_string(stackOffset)+","+getSizedRegister(registerToUse,size)+"\n";
						
						parameterStackSize+=8;
					}
				}
				
				if(parameterStackSize>functionContext.parameterStack)
				{
					functionContext.parameterStack=parameterStackSize;
				}
			}
			
			if(classIndex==-1)
			{
				output+=string("call ")+getAssemblyFunctionName(functionIndex)+"\n";
			}
			else
			{
				output+=string("call ")+getAssemblyMethodName(classIndex,functionIndex)+"\n";
			}
			
			//TODO---- check for exception
			
			if(getReturnValue)
			{
				value=ExpressionValue();
				
				value.localVariableIndex=createTmp(localVariables,functionContext,returnValueType,false);
				
				if(isTypePassedByReference(returnValueType))
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
			ExpressionValue& value,int label,const string& jumpInstructionName)
		{
			string output;
			
			string registerToUse="rax";
			
			output+=movLocalToRegisterGetValue(localVariables[value.localVariableIndex],registerToUse);
			
			output+=string("cmp ")+getSizedRegister(registerToUse,getTypeSize(localVariables[value.localVariableIndex].type,false))+",0\n";
			output+=jumpInstructionName+" "+getAssemblyLabelName(label)+"\n";
			
			return output;
		}
		bool localIsInteger(LocalVariable& localVariable)
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
					
					bool success=false;
					statementCode+=compileAssignment(function,code,t,functionContext,localVariables,returnValue,value,success);
					if(!success)
					{
						code.addError(t,__LINE__);
						throw LogicError();
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
				
				statementCode+=compileScope(function,code,t,functionContext,localVariables);
				
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
					
					statementCode+=compileScope(function,code,t,functionContext,localVariables);
					
					statementCode+=getAssemblyLabelName(elseLabel)+":\n";
				}
				
				if(code.get(t)=="else")
				{
					t++;
					
					statementCode+=compileScope(function,code,t,functionContext,localVariables);
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
			
			//TODO--------
			
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
							LocalVariable& v=localVariables[variableName];
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
							LocalVariable& v=localVariables[variableName];
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
		string compileExpressionValueImmediate(FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,ExpressionValue& value,const Type& type,uint64_t integer,
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
		int getOffsetAndTypeOfAttribute(const Type& baseType,const string& attributeName,Type& attributeType)
		{
			int offset=0;
			
			CodeTopClassAttribute*attribute=findClassAttribute(baseType,attributeName);
			if(attribute==nullptr) return -1;
			
			attributeType=attribute->type;
			offset=attribute->objectOffset;
			
			return offset;
		}
		bool isAssignable(NamedVector<LocalVariable>& localVariables,const ExpressionValue& lvalueExpression,const ExpressionValue& rvalueExpression)
		{
			LocalVariable& lvalue=localVariables[lvalueExpression.localVariableIndex];
			LocalVariable& rvalue=localVariables[rvalueExpression.localVariableIndex];
			
			if(rvalueExpression.isNullptr)
			{
				if(lvalue.type.pointerLevels>0) return true;
				else return false;
			}
			if(rvalueExpression.isIntegerLiteral)
			{
				if(lvalue.type.pointerLevels>0) return false;
				return dataTypes[lvalue.type.name].isInteger;
			}
			else
			{
				if(lvalue.type.pointerLevels!=rvalue.type.pointerLevels) return false;
				if(lvalue.type.name!=rvalue.type.name) return false;
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
		string movLocalToRegisterGetValue(LocalVariable& localVariable,const string& reg)
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
				output+=string("shl ")+getSizedRegister(regl,size)+"\n";
			}
			else if(op==">>")
			{
				if(regl=="rcx" || regr!="rcx")
				{
					throw string("Compiler logic error from line: ")+to_string(__LINE__);
				}
				if(isSigned)
				{
					output+=string("sar ")+getSizedRegister(regl,size)+"\n";
				}
				else
				{
					output+=string("shr ")+getSizedRegister(regl,size)+"\n";
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
			bool isSigned=isSignedPrimitiveTypeName(type);
			
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
			ExpressionValue& lvalue,ExpressionValue& value,int level,const string& op)
		{
			string expressionCode;
			
			ExpressionValue rvalue;
			expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,rvalue,level);
			
			LocalVariable& vlvalue=localVariables[lvalue.localVariableIndex];
			LocalVariable& vrvalue=localVariables[rvalue.localVariableIndex];
			
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
				//TODO----
				code.addError(t,__LINE__);//----
				throw LogicError();//----
			}
			
			return expressionCode;
		}
		string compileLeftUnaryOperator(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue& value,const string& op)
		{
			string expressionCode;
			
			ExpressionValue valueInside;
			expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,valueInside,getMaximumOperatorLevel());
			
			LocalVariable& v=localVariables[valueInside.localVariableIndex];
			
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
				//TODO----
				code.addError(t,__LINE__);//----
				throw LogicError();//----
			}
			
			return expressionCode;
		}
		string castPrimitive(const string& reg,const Type& initialType,const Type& resultType,bool& success)
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
		string compileAssignment(CodeTopFunction& function,TokenizedCode& code,size_t& t,FunctionContext& functionContext,NamedVector<LocalVariable>& localVariables,
			ExpressionValue& lvalue,ExpressionValue& rvalue,bool& success)
		{
			string output;
			
			if(isAssignable(localVariables,lvalue,rvalue))
			{
				LocalVariable& lv=localVariables[lvalue.localVariableIndex];
				LocalVariable& rv=localVariables[rvalue.localVariableIndex];
				
				int lsize=getTypeSize(lv.type,false);
				int rsize=getTypeSize(rv.type,false);
				
				if(lv.type.pointerLevels>0 || dataTypes[lv.type.name].isInteger)
				{
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
					output+=compileCallAssignmentOperatorOfLocal(function,code,functionContext,
						localVariables,lvalue.localVariableIndex,rvalue.localVariableIndex);
				}
				
				success=true;
			}
			else
			{
				success=false;
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
				
				LocalVariable& v=localVariables[valueInside.localVariableIndex];
				
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
				
				LocalVariable& v=localVariables[valueInside.localVariableIndex];
				
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
						code.addError(t,__LINE__);
						throw ParseError();
					}
					t++;
					
					if(castOrConstructorType.pointerLevels>0 || dataTypes[castOrConstructorType.name].isPrimitive)
					{
						ExpressionValue valueInside;
						
						expressionCode+=compileExpressionPart(function,code,t,functionContext,localVariables,valueInside);
						
						LocalVariable& v=localVariables[valueInside.localVariableIndex];
						
						Type resultType=castOrConstructorType;
						
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
							code.addError(t,__LINE__);//TODO----
							throw LogicError();//TODO----
						}
					}
					
					if(code.get(t)!=")")
					{
						code.addError(t,__LINE__);
						throw ParseError();
					}
					t++;
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
							
							code.addError(t-1,__LINE__);
							throw LogicError();
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
							string variableName=start;
							bool isGlobal=false;
							bool isReferenceLocal=false;
							
							string registerToUse="rax";
							
							LocalVariable v;
							int globalVariableIndex=-1;
							Type type;
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
							
							int offset=0;
							t++;
							
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
										code.addError(t,__LINE__);
										throw NameError();
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
				
				if(token=="=")
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
					
					if(token=="*" || token=="/" || token=="%" || token=="+" || token=="-" || token=="^")
					{
						isOperator=true;
						operatorTokens=1;
						op=token;
					}
					else if(token=="<")
					{
						if(code.get(t+1)=="<")
						{
							isOperator=true;
							operatorTokens=2;
							op="<<";
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
							isOperator=true;
							operatorTokens=2;
							op=">>";
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
						else
						{
							isOperator=true;
							operatorTokens=1;
							op=token;
						}
					}
					
					if(isOperator)
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
					else
					{
						code.addError(t,__LINE__);
						throw ParseError();
					}
				}
				
				//TODO--------
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
		catch(...)
		{
			if(code.hasErrors()) code.printErrors();
			else cout<<"FATAL ERROR"<<endl;
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

