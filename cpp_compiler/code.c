class RuntimeException
{
	i64 internal_message;
	
	fn constructor()()
	{
		internal_message=i64(r"Not defined exception");
	}
	fn constructor()(ptr(u8) rawMessage)
	{
		internal_message=i64(rawMessage);
	}
	fn getRawMessage(ptr(u8))()
	{
		return ptr(u8)(internal_message);
	}
}

fn __dynamic_constructor()(ptr(i64) dynPtr)
{
	deref(dynPtr)=0;
}

fn __dynamic_destructor()(ptr(i64) dynPtr)
{
	if(deref(dynPtr))
	{
		i64 type=ptr(i64)(deref(dynPtr))[0];
		if(u64(type)<u64(deref(__datatype_count)))
		{
			i64 isReference=ptr(i64)(deref(dynPtr))[1];
			if(isReference==0)
			{
				i64 dataAddress=ptr(i64)(deref(dynPtr))[2];
				if(dataAddress)
				{
					i64 destructorAddress=ptr(i64)(__datatype_start[type])[3];
					__call_destructor(destructorAddress,dataAddress);
					__free(ptr(u8)(dataAddress));
				}
			}
		}
		
		__free(ptr(u8)(deref(dynPtr)));
		deref(dynPtr)=0;
	}
}

fn __dynamic_assignment_operator()(ptr(i64) dynPtr,ptr(i64) dynBPtr)
{
	__dynamic_assign(dynPtr,i64(dynBPtr),dynamic);
}

fn __dynamic_assign()(ptr(i64) dynPtr,i64 rvalue,i64 rvalueType)
{
	__dynamic_assign_internal(dynPtr,rvalue,rvalueType,false);
}

fn __dynamic_assign_internal()(ptr(i64) dynPtr,i64 rvalue,i64 rvalueType,i64 assignDataReference)
{
	if(rvalueType==dynamic)
	{
		if(deref(ptr(i64)(rvalue)))
		{
			__dynamic_typeof(ptr(i64)(rvalue),addressof(rvalueType));
			
			i64 dataAddress=ptr(i64)(deref(ptr(i64)(rvalue)))[2];
			if(dataAddress)
			{
				rvalue=dataAddress;
				if(rvalueType<8 && !assignDataReference)
				{
					i64 size=(i64(1)<<(rvalueType&3));
					if(size==1) rvalue=i64(deref(ptr(i8)(rvalue)));
					if(size==2) rvalue=i64(deref(ptr(i16)(rvalue)));
					if(size==4) rvalue=i64(deref(ptr(i32)(rvalue)));
					if(size==8) rvalue=deref(ptr(i64)(rvalue));
				}
			}
			else
			{
				rvalue=0;
			}
		}
		else
		{
			__dynamic_destructor(dynPtr);
			return;
		}
	}
	
	if(deref(dynPtr))
	{
		i64 type=ptr(i64)(deref(dynPtr))[0];
		if(type==rvalueType)
		{
			if(assignDataReference)
			{
				if(ptr(i64)(deref(dynPtr))[1]==0)
				{
					i64 dataAddress=ptr(i64)(deref(dynPtr))[2];
					i64 destructorAddress=ptr(i64)(__datatype_start[type])[3];
					__call(destructorAddress,dataAddress);
				}
				
				ptr(i64)(deref(dynPtr))[1]=1;
				ptr(i64)(deref(dynPtr))[2]=rvalue;
				return;
			}
			elif(u64(type)<u64(deref(__datatype_count)))
			{
				i64 dataAddress=ptr(i64)(deref(dynPtr))[2];
				if(dataAddress)
				{
					i64 assignmentAddress=ptr(i64)(__datatype_start[type])[4];
					i64 dataB=rvalue;
					if(rvalueType>=8) rvalue=deref(ptr(i64)(dataB));
					__call(assignmentAddress,dataAddress,dataB);
					return;
				}
			}
		}
	}
	
	__dynamic_destructor(dynPtr);
	
	if(assignDataReference)
	{
		i64 dynSize=ptr(i64)(__datatype_start[rvalueType])[0];
		
		deref(dynPtr)=i64(__malloc(24));
		if(deref(dynPtr)==0)
		{
			throw RuntimeException(r"Not enough memory for dynamic");
		}
		ptr(i64)(deref(dynPtr))[0]=rvalueType;
		ptr(i64)(deref(dynPtr))[1]=1;
		ptr(i64)(deref(dynPtr))[2]=rvalue;
	}
	elif(rvalueType<8)
	{
		i64 dynSize=(i64(1)<<(rvalueType&3));
		
		deref(dynPtr)=i64(__malloc(24));
		if(deref(dynPtr)==0)
		{
			throw RuntimeException(r"Not enough memory for dynamic");
		}
		ptr(i64)(deref(dynPtr))[0]=rvalueType;
		ptr(i64)(deref(dynPtr))[1]=0;
		ptr(i64)(deref(dynPtr))[2]=i64(__malloc(dynSize));
		if(ptr(i64)(deref(dynPtr))[2]==0)
		{
			throw RuntimeException(r"Not enough memory for dynamic");
		}
		
		i64 dynValueAddress=ptr(i64)(deref(dynPtr))[2];
		for(i64 i=0;i<dynSize;i++)
		{
			ptr(u8)(dynValueAddress)[i]=ptr(u8)(addressof(rvalue))[i];
		}
	}
	else
	{
		i64 dynSize=ptr(i64)(__datatype_start[rvalueType])[0];
		
		deref(dynPtr)=i64(__malloc(24));
		if(deref(dynPtr)==0)
		{
			throw RuntimeException(r"Not enough memory for dynamic");
		}
		ptr(i64)(deref(dynPtr))[0]=rvalueType;
		ptr(i64)(deref(dynPtr))[1]=0;
		ptr(i64)(deref(dynPtr))[2]=i64(__malloc(dynSize));
		if(ptr(i64)(deref(dynPtr))[2]==0)
		{
			throw RuntimeException(r"Not enough memory for dynamic");
		}
		
		i64 dataAddress=ptr(i64)(deref(dynPtr))[2];
		
		i64 constructorAddress=ptr(i64)(__datatype_start[rvalueType])[2];
		__call(constructorAddress,dataAddress);
		
		i64 assignmentAddress=ptr(i64)(__datatype_start[rvalueType])[4];
		__call(assignmentAddress,dataAddress,rvalue);
	}
}

fn __dynamic_get_attribute()(ptr(i64) dynPtr,ptr(i64) outputDynPtr,i64 identifierIndex)
{
	if(deref(dynPtr))
	{
		i64 type=ptr(i64)(deref(dynPtr))[0];
		
		if(u64(type)<u64(deref(__datatype_count)))
		{
			i64 dataAddress=ptr(i64)(deref(dynPtr))[2];
			i64 attributeCount=ptr(i64)(__datatype_start[type])[5];
			ptr(i64) attributeList=ptr(i64)(ptr(i64)(__datatype_start[type])[6]);
			for(i64 i=0;i<attributeCount;i++)
			{
				i64 index=i*4;
				if(identifierIndex==attributeList[index])
				{
					i64 type=attributeList[index+1];
					i64 rvalue=dataAddress+attributeList[index+2];
					__dynamic_assign_internal(outputDynPtr,rvalue,type,true);
					return;
				}
			}
		}
		
		throw RuntimeException(r"Getting non-existent attribute from dynamic");
	}
	else
	{
		throw RuntimeException(r"Getting non-existent attribute from dynamic");
	}
}

fn __dynamic_get_method()(ptr(i64) dynPtr,ptr(i64) methodPtr,i64 identifierIndex)
{
	if(deref(dynPtr))
	{
		i64 type=ptr(i64)(deref(dynPtr))[0];
		
		if(u64(type)<u64(deref(__datatype_count)))
		{
			i64 methodCount=ptr(i64)(__datatype_start[type])[7];
			ptr(i64) methodList=ptr(i64)(ptr(i64)(__datatype_start[type])[8]);
			for(i64 i=0;i<methodCount;i++)
			{
				i64 index=i*8;
				if(identifierIndex==methodList[index])
				{
					deref(methodPtr)=i64(addressof(methodList[index]));
					return;
				}
			}
		}
		
		throw RuntimeException(r"Getting non-existent method from dynamic");
	}
	else
	{
		throw RuntimeException(r"Getting non-existent method from dynamic");
	}
}

fn __dynamic_function_check_number_of_arguments()(i64 functionInfoAddress,i64 numberOfArguments)
{
	i64 numberOfParameters=ptr(i64)(functionInfoAddress)[3];
	if(numberOfArguments!=numberOfParameters)
	{
		throw RuntimeException(r"Function arguments don't match (number of arguments)");
	}
}

fn __dynamic_function_check_parameter_type()(i64 functionInfoAddress,i64 parameterIndex,i64 argumentType)
{
	ptr(i64) params=ptr(i64)(ptr(i64)(functionInfoAddress)[4]);
	i64 parameterType=params[parameterIndex];
	if(argumentType!=parameterType)
	{
		throw RuntimeException(r"Function arguments don't match (argument type)");
	}
}

fn __dynamic_function_get_address_and_return_type()(i64 functionInfoAddress,ptr(i64) outputFunctionAddress,ptr(i64) outputReturnType)
{
	deref(outputFunctionAddress)=ptr(i64)(functionInfoAddress)[1];
	deref(outputReturnType)=ptr(i64)(functionInfoAddress)[2];
}

fn __dynamic_cast_to_type()(ptr(i64) dynPtr,ptr(i64) outputPtr,i64 type)
{
	if(deref(dynPtr))
	{
		i64 dynType=ptr(i64)(deref(dynPtr))[0];
		if(dynType!=type)
		{
			throw RuntimeException(r"Failed dynamic cast");
		}
		deref(outputPtr)=ptr(i64)(deref(dynPtr))[2];
	}
	else
	{
		throw RuntimeException(r"Failed dynamic cast");
	}
}

fn __dynamic_sizeof()(ptr(i64) dynPtr,ptr(i64) outputPtr)
{
	if(deref(dynPtr))
	{
		i64 type=ptr(i64)(deref(dynPtr))[0];
		if(u64(type)>=u64(deref(__datatype_count)))
		{
			throw RuntimeException(r"Failed sizeof of dynamic (type outside range)");
		}
		deref(outputPtr)=ptr(i64)(__datatype_start[type])[0];
	}
	else deref(outputPtr)=sizeof(dynamic);
}

fn __dynamic_alignof()(ptr(i64) dynPtr,ptr(i64) outputPtr)
{
	if(deref(dynPtr))
	{
		i64 type=ptr(i64)(deref(dynPtr))[0];
		if(u64(type)>=u64(deref(__datatype_count)))
		{
			throw RuntimeException(r"Failed alignof of dynamic (type outside range)");
		}
		deref(outputPtr)=ptr(i64)(__datatype_start[type])[1];
	}
	else deref(outputPtr)=alignof(dynamic);
}

fn __dynamic_typeof()(ptr(i64) dynPtr,ptr(i64) outputPtr)
{
	if(deref(dynPtr))
	{
		deref(outputPtr)=ptr(i64)(deref(dynPtr))[0];
	}
	else deref(outputPtr)=typeof(dynamic);
}

fn __dynamic_cast_integer_to_integer()(ptr(i64) dynPtr,ptr(i64) outputPtr,i64 outputType)
{
	if(deref(dynPtr))
	{
		i64 dynType=ptr(i64)(deref(dynPtr))[0];
		i64 dynValueAddress=ptr(i64)(deref(dynPtr))[2];
		
		if(dynType>7)
		{
			throw RuntimeException(r"Failed dynamic cast to integer (not an integer)");
		}
		i64 dynSign=i64(dynType>=4);
		i64 dynSize=(i64(1)<<(dynType&3));
		
		i64 outputSign=i64(outputType>=4);
		i64 outputSize=(i64(1)<<(outputType&3));
		
		if(dynSign==outputSign)
		{
			u64 v=0;
			for(i64 i=0;i<dynSize;i++)
			{
				ptr(u8)(addressof(v))[i]=ptr(u8)(dynValueAddress)[i];
			}
			
			if(dynSign)
			{
				if(ptr(u8)(addressof(v))[dynSize-1]&0x80)
				{
					for(i64 i=dynSize;i<outputSize;i++)
					{
						ptr(u8)(addressof(v))[i]=0xff;
					}
				}
			}
			
			for(i64 i=0;i<outputSize;i++)
			{
				ptr(u8)(outputPtr)[i]=ptr(u8)(addressof(v))[i];
			}
		}
		elif(dynSize==outputSize)
		{
			for(i64 i=0;i<dynSize;i++)
			{
				ptr(u8)(outputPtr)[i]=ptr(u8)(dynValueAddress)[i];
			}
		}
		else
		{
			throw RuntimeException(r"Failed dynamic cast to integer");
		}
	}
	else
	{
		throw RuntimeException(r"Failed dynamic cast to integer (not an integer)");
	}
}





class __struct_stat
{
	u64 st_dev;
	u64 st_ino;
	u32 st_mode;
	u32 st_nlink;
	u32 st_uid;
	u32 st_gid;
	u64 st_rdev;
	u64 __pad1;
	i64 st_size;
	i32 st_blksize;
	i32 __pad2;
	i64 st_blocks;
	i64 st_atime;
	u64 st_atime_nsec;
	i64 st_mtime;
	u64 st_mtime_nsec;
	i64 st_ctime;
	u64 st_ctime_nsec;
	u32 __unused4;
	u32 __unused5;
}

i64 __errno;

fn __syscall(i64)(i64 syscallNumber,i64 arg0,i64 arg1,i64 arg2,i64 arg3,i64 arg4,i64 arg5)
{
	i64 returnValue;
	asm("syscall",
		"rax"(syscallNumber),
		"rdi"(arg0),"rsi"(arg1),"rdx"(arg2),"r10"(arg3),"r8"(arg4),"r9"(arg5),
		"=rax"(returnValue));
	if(returnValue<0)
	{
		if(returnValue>-4096)
		{
			__errno=-returnValue;
			returnValue=-1;
		}
	}
	return returnValue;
}

fn __read(i64)(i64 fd,ptr(u8) buf,i64 count)
{
	return __syscall(0,fd,i64(buf),count,0,0,0);
}
fn __write(i64)(i64 fd,ptr(u8) buf,i64 count)
{
	return __syscall(1,fd,i64(buf),count,0,0,0);
}
fn __open(i64)(ptr(u8) filename,i64 flags,i64 mode)
{
	return __syscall(2,i64(filename),flags,mode,0,0,0);
}
fn __close(i64)(i64 fd)
{
	return __syscall(3,fd,0,0,0,0,0);
}
fn __stat(i64)(ptr(u8) filename,ptr(__struct_stat) statbuf)
{
	return __syscall(4,i64(filename),i64(statbuf),0,0,0,0);
}
fn __fstat(i64)(i64 fd,ptr(__struct_stat) statbuf)
{
	return __syscall(5,fd,i64(statbuf),0,0,0,0);
}

fn __mmap(ptr(u8))(ptr(u8) addr,i64 length,i64 prot,i64 flags,i64 fd,i64 offset)
{
	return ptr(u8)(__syscall(9,i64(addr),length,prot,flags,fd,offset));
}
fn __munmap(i64)(ptr(u8) addr,i64 length)
{
	return __syscall(11,i64(addr),length,0,0,0,0);
}

fn __exit(i64)(i64 error_code)
{
	return __syscall(60,error_code,0,0,0,0,0);
}



class MemoryAllocationSystemMacroblockEntry
{
	i64 blockAddress;
	i64 maximumContiguousFreeSpace;
}

class MemoryAllocationSystem
{
	i64 pageSize;
	
	i64 blockSize;
	i64 blockElementSize;
	
	i64 blockElements;
	i64 blockBitSpace;
	i64 blockSpace;
	i64 blockSpaceStart;
	
	i64 allocationInBlockSizeLimit;
	
	i64 macroblockSize;
	
	i64 macroblockEntrySize;
	i64 macroblockEntryStart;
	i64 macroblockEntries;
	
	
	i64 firstMacroblockAddress;
	
	
	fn constructor()()
	{
		pageSize=4096;
		blockSize=(i64(1)<<20);
		blockElementSize=16;
		macroblockSize=(i64(1)<<20);
		
		blockElements=blockSize/(blockElementSize+1);
		{
			i64 m=blockElements%64;
			if(m) blockElements+=64-m;
		}
		blockBitSpace=blockElements/8;
		blockSpace=blockElements*blockElementSize;
		blockSpaceStart=blockSize-blockSpace;
		
		allocationInBlockSizeLimit=blockSpace;
		
		macroblockEntrySize=sizeof(MemoryAllocationSystemMacroblockEntry);
		macroblockEntryStart=8;
		macroblockEntries=(macroblockSize-macroblockEntryStart-8)/macroblockEntrySize;
		
		firstMacroblockAddress=0;
	}
	
	fn mmap(ptr(u8))(i64 size)
	{
		return __mmap(nullptr,size,0b11,0x21,-1,0);
	}
	fn munmap(i64)(ptr(u8) address,i64 size)
	{
		return __munmap(address,size);
	}
	
	fn internalMallocBig(i64)(i64 size)
	{
		i64 address=i64(mmap(size));
		if(address==-1) return 0;
		return address;
	}
	fn internalFreeBig(u8)(i64 address,i64 size)
	{
		i64 r=munmap(ptr(u8)(address),size);
		if(r==-1) return false;
		else return true;
	}
	
	fn reserveBlock(i64)()
	{
		i64 address=i64(mmap(blockSize));
		if(address==-1) return 0;
		return address;
	}
	fn freeBlock(u8)(i64 address)
	{
		i64 r=munmap(ptr(u8)(address),blockSize);
		if(r==-1) return false;
		else return true;
	}
	fn initializeBlock()(i64 blockAddress)
	{
		ptr(i64) p=ptr(i64)(blockAddress);
		i64 number=blockElements/64;
		for(i64 i=0;i<number;i++)
		{
			p[i]=0;
		}
	}
	
	fn setContiguousBitsOfI64To()(i64 startAddress,i64 startI64,i64 startI64Offset,i64 endI64Offset,u8 bitValue)
	{
		u64 value=ptr(u64)(startAddress)[startI64];
		
		u64 bitsToChange=(u64(-1)<<u64(startI64Offset));
		if(endI64Offset<64) bitsToChange&=(~(u64(-1)<<u64(endI64Offset)));
		
		if(bitValue)
		{
			value|=bitsToChange;
		}
		else
		{
			value&=(~bitsToChange);
		}
		
		ptr(u64)(startAddress)[startI64]=value;
	}
	fn setContiguousBitsTo()(i64 startAddress,i64 startBit,i64 numberOfBits,u8 bitValue)
	{
		i64 startI64=startBit/64;
		i64 startI64Offset=startBit%64;
		
		i64 endBit=startBit+numberOfBits-1;
		
		i64 endI64=endBit/64;
		i64 endI64Offset=endBit%64+1;
		
		if(endI64==startI64)
		{
			setContiguousBitsOfI64To(startAddress,startI64,startI64Offset,endI64Offset,bitValue);
		}
		else
		{
			setContiguousBitsOfI64To(startAddress,startI64,startI64Offset,64,bitValue);
			setContiguousBitsOfI64To(startAddress,endI64,0,endI64Offset,bitValue);
			for(i64 i=startI64+1;i<endI64;i++)
			{
				if(bitValue) ptr(i64)(startAddress)[i]=-1;
				else ptr(i64)(startAddress)[i]=0;
			}
		}
	}
	fn findContiguousZeroBits(i64)(i64 startAddress,i64 numberOfi64s,i64 numberOfZeroBits)
	{
		if(numberOfZeroBits>=4)
		{
			i64 intSize=8;
			if(numberOfZeroBits>8) intSize=16;
			if(numberOfZeroBits>16) intSize=32;
			if(numberOfZeroBits>32) intSize=64;
			if(numberOfZeroBits>64) intSize=128;
			
			if(intSize==8)
			{
				i64 max_=numberOfi64s*8;
				for(i64 index=0;index<max_;index++)
				{
					if(ptr(i8)(startAddress)[index]==0)
					{
						return index*intSize;
					}
				}
			}
			elif(intSize==16)
			{
				i64 max_=numberOfi64s*4;
				for(i64 index=0;index<max_;index++)
				{
					if(ptr(i16)(startAddress)[index]==0)
					{
						return index*intSize;
					}
				}
			}
			elif(intSize==32)
			{
				i64 max_=numberOfi64s*2;
				for(i64 index=0;index<max_;index++)
				{
					if(ptr(i32)(startAddress)[index]==0)
					{
						return index*intSize;
					}
				}
			}
			elif(intSize==64)
			{
				i64 max_=numberOfi64s;
				for(i64 index=0;index<max_;index++)
				{
					if(ptr(i64)(startAddress)[index]==0)
					{
						return index*intSize;
					}
				}
			}
			else
			{
				i64 numberOfZeroInts=numberOfZeroBits/64;
				if(numberOfZeroInts*64<numberOfZeroBits)
				{
					numberOfZeroInts++;
				}
				
				i64 count=0;
				for(i64 index=0;index<numberOfi64s;index++)
				{
					if(ptr(i64)(startAddress)[index]==0) count++;
					else count=0;
					
					if(count>=numberOfZeroInts)
					{
						return (index+1-count)*64;
					}
				}
			}
		}
		else
		{
			for(i64 i64Index=0;i64Index<numberOfi64s;i64Index++)
			{
				if(ptr(i64)(startAddress)[i64Index]!=-1)
				{
					for(i64 i8Index=0;i8Index<8;i8Index++)
					{
						i8 ibyte=ptr(i8)(startAddress)[i64Index*8+i8Index];
						if(ibyte!=-1)
						{
							u8 byte=u8(ibyte);
							
							i64 count=0;
							for(i64 bitIndex=0;bitIndex<8;bitIndex++)
							{
								if(((byte>>u8(bitIndex))&1)==0) count++;
								else count=0;
								
								if(count>=numberOfZeroBits)
								{
									return (i64Index*8+i8Index)*8+bitIndex+1-count;
								}
							}
						}
					}
				}
			}
		}
		return -1;
	}
	
	fn createBlock(i64)()
	{
		i64 blockAddress=reserveBlock();
		initializeBlock(blockAddress);
		return blockAddress;
	}
	fn destroyBlock(u8)(i64 blockAddress)
	{
		return freeBlock(blockAddress);
	}
	fn blockReserve(i64)(i64 blockAddress,i64 spaceSize)
	{
		i64 bits=spaceSize/blockElementSize;
		i64 bitOffset=findContiguousZeroBits(blockAddress,blockElements/64,bits);
		if(bitOffset==-1) return 0;
		setContiguousBitsTo(blockAddress,bitOffset,bits,1);
		return blockAddress+blockSpaceStart+bitOffset*blockElementSize;
	}
	fn blockFree()(i64 blockAddress,i64 address,i64 spaceSize)
	{
		i64 bits=spaceSize/blockElementSize;
		i64 bitOffset=(address-blockAddress-blockSpaceStart)/blockElementSize;
		setContiguousBitsTo(blockAddress,bitOffset,bits,0);
	}
	fn blockGetMaximumContiguousFreeSpace(i64)(i64 blockAddress)
	{
		i64 maximum=0;
		i64 indexes=blockElements/64;
		i64 count=0;
		for(i64 index=0;index<indexes;index++)
		{
			if(ptr(i64)(blockAddress)[index]==0) count++;
			else count=0;
			
			if(count>maximum)
			{
				maximum=count;
			}
		}
		return maximum*64*blockElementSize;
	}
	
	fn reserveMacroblock(i64)()
	{
		i64 address=i64(mmap(macroblockSize));
		if(address==-1) return 0;
		return address;
	}
	fn freeMacroblock(u8)(i64 address)
	{
		i64 r=munmap(ptr(u8)(address),macroblockSize);
		if(r==-1) return false;
		else return true;
	}
	fn initializeMacroblock()(i64 macroblockAddress)
	{
		deref(ptr(i64)(macroblockAddress))=0;
		ptr(MemoryAllocationSystemMacroblockEntry) p=getMacroblockEntryPointer(macroblockAddress);
		for(i64 i=0;i<macroblockEntries;i++)
		{
			p[i].blockAddress=0;
		}
		deref(ptr(i64)(macroblockAddress+macroblockSize-8))=0;
	}
	fn createMacroblock(i64)()
	{
		i64 address=reserveMacroblock();
		if(address==0) return 0;
		initializeMacroblock(address);
		return address;
	}
	
	fn getMacroblockEntryPointer(ptr(MemoryAllocationSystemMacroblockEntry))(i64 macroblockAddress)
	{
		return ptr(MemoryAllocationSystemMacroblockEntry)(macroblockAddress+macroblockEntryStart);
	}
	fn getNextMacroblockAddress(i64)(i64 address)
	{
		return deref(ptr(i64)(address+macroblockSize-8));
	}
	fn setNextMacroblockAddress()(i64 macroblockAddress,i64 next)
	{
		deref(ptr(i64)(macroblockAddress+macroblockSize-8))=next;
	}
	
	fn internalMalloc(i64)(i64 size,ptr(i64) blockIndex)
	{
		if(size>allocationInBlockSizeLimit)
		{
			i64 m=size%pageSize;
			if(m) size+=pageSize-m;
			return internalMallocBig(size);
		}
		else
		{
			i64 macroblockIndex=0;
			i64 macroblockAddress=firstMacroblockAddress;
			while(true)
			{
				{
					ptr(MemoryAllocationSystemMacroblockEntry) entries=getMacroblockEntryPointer(macroblockAddress);
					for(i64 i=0;i<macroblockEntries;i++)
					{
						i64 blockAddress=entries[i].blockAddress;
						if(blockAddress==0)
						{
							blockAddress=createBlock();
							if(blockAddress==0) return 0;
							entries[i].blockAddress=blockAddress;
							entries[i].maximumContiguousFreeSpace=blockSpace;
						}
						
						if(entries[i].maximumContiguousFreeSpace>=size)
						{
							i64 address=blockReserve(blockAddress,size);
							if(address)
							{
								entries[i].maximumContiguousFreeSpace=blockGetMaximumContiguousFreeSpace(blockAddress);
								
								deref(blockIndex)=macroblockIndex*macroblockEntries+i;
								
								return address;
							}
						}
					}
				}
				macroblockIndex++;
				i64 next=getNextMacroblockAddress(macroblockAddress);
				if(next)
				{
					macroblockAddress=next;
				}
				else
				{
					i64 newMacroblockAddress=createMacroblock();
					if(newMacroblockAddress==0) return 0;
					setNextMacroblockAddress(macroblockAddress,newMacroblockAddress);
					macroblockAddress=newMacroblockAddress;
					
					ptr(MemoryAllocationSystemMacroblockEntry) entries=getMacroblockEntryPointer(macroblockAddress);
					
					i64 blockAddress=createBlock();
					if(blockAddress==0) return 0;
					entries[0].blockAddress=blockAddress;
					
					i64 address=blockReserve(blockAddress,size);
					
					entries[0].maximumContiguousFreeSpace=blockGetMaximumContiguousFreeSpace(blockAddress);
					
					deref(blockIndex)=macroblockIndex*macroblockEntries;
					
					return address;
				}
			}
		}
	}
	fn internalFree(u8)(i64 address,i64 size,i64 blockIndex)
	{
		if(size>allocationInBlockSizeLimit)
		{
			i64 m=size%pageSize;
			if(m) size+=pageSize-m;
			return internalFreeBig(address,size);
		}
		else
		{
			i64 macroblockIndex=blockIndex/macroblockEntries;
			i64 macroblockAddress=firstMacroblockAddress;
			for(i64 i=0;i<macroblockIndex;i++)
			{
				macroblockAddress=getNextMacroblockAddress(macroblockAddress);
			}
			
			i64 entryIndex=blockIndex%macroblockEntries;
			ptr(MemoryAllocationSystemMacroblockEntry) entries=getMacroblockEntryPointer(macroblockAddress);
			
			i64 blockAddress=entries[entryIndex].blockAddress;
			
			blockFree(blockAddress,address,size);
			
			i64 space=blockGetMaximumContiguousFreeSpace(blockAddress);
			
			if(space==0)
			{
				if(!destroyBlock(blockAddress)) return false;
				entries[entryIndex].blockAddress=0;
			}
			else
			{
				entries[entryIndex].maximumContiguousFreeSpace=space;
			}
			
			return true;
		}
	}
	
	fn initialize(u8)()
	{
		firstMacroblockAddress=createMacroblock();
		if(firstMacroblockAddress==0) return false;
		return true;
	}
	
	fn malloc(ptr(u8))(i64 size)
	{
		i64 internalSize=size;
		i64 m=internalSize%16;
		if(m) internalSize+=16-m;
		internalSize+=16;
		i64 blockIndex=-1;
		i64 address=internalMalloc(internalSize,addressof(blockIndex));
		if(address==0) return nullptr;
		deref(ptr(i64)(address))=internalSize;
		ptr(i64)(address)[1]=blockIndex;
		return ptr(u8)(address+16);
	}
	fn free(u8)(ptr(u8) pointer)
	{
		return internalFree(i64(pointer)-16,deref(ptr(i64)(pointer-16)),deref(ptr(i64)(pointer-8)));
	}
}

ptr(MemoryAllocationSystem) PROCESS_MEMORY_ALLOCATION_SYSTEM;

fn __malloc(ptr(u8))(i64 size)
{
	return deref(PROCESS_MEMORY_ALLOCATION_SYSTEM).malloc(size);
}

fn __free(u8)(ptr(u8) pointer)
{
	return deref(PROCESS_MEMORY_ALLOCATION_SYSTEM).free(pointer);
}

fn __main(i64)(i64 argc,ptr(ptr(u8)) argv)
{
	__errno=0;
	MemoryAllocationSystem memoryAllocationSystem;
	if(!memoryAllocationSystem.initialize()) return 1;
	PROCESS_MEMORY_ALLOCATION_SYSTEM=addressof(memoryAllocationSystem);
	return __main2(argc,argv);
}

fn strlen(i64)(ptr(u8) str)
{
	i64 i=0;
	while(str[i])
	{
		i++;
	}
	return i;
}

fn printr()(ptr(u8) str)
{
	__write(1,str,strlen(str));
}

fn __main2(i64)(i64 argc,ptr(ptr(u8)) argv)
{
	try
	{
		return main(argc,argv);
	}
	catch(dynamic e)
	{
		if(typeof(e)==RuntimeException)
		{
			RuntimeException re=e;
			printr(r"exception: RuntimeException: ");
			printr(re.getRawMessage());
			printr(r"\n");
		}
		else
		{
			printr(r"exception: Not known exception\n");
		}
	}
}



fn uToString(string)(u64 integer)
{
	string str;
	while(integer)
	{
		u64 digit=integer%10;
		integer/=10;
		str.put(b'0'+u8(digit));
	}
	if(str.size()==0) str.put(b'0');
	
	string str2;
	for(i64 i=str.size()-1;i>=0;i--)
	{
		str2.put(str[i]);
	}
	return str2;
}

fn iToString(string)(i64 integer)
{
	if(integer<0) return "-"+uToString(u64(-integer));
	else return uToString(u64(integer));
}

class string
{
	ptr(u8) internal_data;
	i64 internal_size;
	i64 internal_reservedSize;
	
	fn constructor()()
	{
		internal_initialize();
	}
	fn constructor()(ptr(u8) raw)
	{
		internal_clear();
		i64 initialSize=strlen(raw)+1;
		internal_data=internal_allocate(initialSize);
		internal_reservedSize=initialSize;
		internal_size=initialSize;
		internal_copy(internal_data,raw,initialSize);
	}
	fn constructor()(i64 integer)
	{
		internal_constructor_integer(integer);
	}
	fn constructor()(u64 integer)
	{
		internal_constructor_u_integer(integer);
	}
	fn destructor()()
	{
		internal_deinitialize();
	}
	fn operator(=)()(string str)
	{
		if(addressof(str)==addressof(this)) return;
		internal_checkInitialized();
		ptr(u8) newData=internal_allocate(str.internal_size);
		internal_copy(newData,str.internal_data,str.internal_size);
		if(internal_data!=nullptr) internal_free(internal_data);
		internal_data=newData;
		internal_reservedSize=str.internal_size;
		internal_size=str.internal_size;
	}
	
	fn size(i64)()
	{
		internal_checkInitialized();
		return internal_size-1;
	}
	fn data(ptr(u8))()
	{
		internal_checkInitialized();
		return internal_data;
	}
	fn clear()()
	{
		internal_checkInitialized();
		internal_resize(1);
	}
	fn resize()(i64 newSize)
	{
		internal_checkInitialized();
		i64 newInternalSize=newSize+1;
		i64 oldSize=internal_size;
		internal_resize(newInternalSize);
		if(newInternalSize>oldSize) internal_setTo(internal_data,oldSize,newInternalSize,0);
	}
	fn capacity(i64)()
	{
		internal_checkInitialized();
		return internal_reservedSize-1;
	}
	fn reserve()(i64 newCapacity)
	{
		internal_checkInitialized();
		i64 newReservedSize=newCapacity+1;
		if(newReservedSize>internal_reservedSize)
		{
			internal_setCapacity(newReservedSize);
		}
	}
	fn shrinkToFit()()
	{
		internal_checkInitialized();
		internal_setCapacity(internal_size);
	}
	fn add()(string str)
	{
		internal_checkInitialized();
		if(str.internal_data!=nullptr)
		{
			i64 oldSize=internal_size;
			i64 strSize=str.internal_size;
			internal_resize(internal_size+str.internal_size-1);
			internal_copy(internal_data+oldSize-1,str.internal_data,strSize-1);
		}
	}
	fn put()(u8 byte)
	{
		internal_checkInitialized();
		internal_resize(internal_size+1);
		internal_data[internal_size-2]=byte;
	}
	fn operator([])(ref(u8))(i64 index)
	{
		return internal_data[index];
	}
	fn at(ref(u8))(i64 index)
	{
		internal_checkInitialized();
		if(u64(index)>=u64(internal_size-1)) throw RuntimeException(r"Out of range of string");
		return internal_data[index];
	}
	fn front(ref(u8))()
	{
		return at(0);
	}
	fn back(ref(u8))()
	{
		return at(size()-1);
	}
	
	fn internal_constructor_integer()(i64 integer)
	{
		internal_clear();
		this=iToString(integer);
	}
	fn internal_constructor_u_integer()(u64 integer)
	{
		internal_clear();
		this=uToString(integer);
	}
	fn internal_resize()(i64 newSize)
	{
		if(newSize==internal_size) return;
		if(newSize<internal_size)
		{
			internal_size=newSize;
			internal_data[newSize-1]=0;
		}
		else
		{
			if(newSize>internal_reservedSize)
			{
				i64 newReservedSize=newSize*2;
				internal_setCapacity(newReservedSize);
			}
			internal_size=newSize;
			internal_data[newSize-1]=0;
		}
	}
	fn internal_setCapacity()(i64 newReservedSize)
	{
		if(newReservedSize==internal_reservedSize) return;
		if(newReservedSize<internal_size) return;
		ptr(u8) newData=internal_allocate(newReservedSize);
		internal_copy(newData,internal_data,internal_size);
		if(internal_data!=nullptr) internal_free(internal_data);
		internal_data=newData;
		internal_reservedSize=newReservedSize;
	}
	fn internal_checkInitialized()()
	{
		if(internal_data==nullptr) internal_initialize();
	}
	fn internal_initialize()()
	{
		internal_clear();
		i64 initialSize=1;
		internal_data=internal_allocate(initialSize);
		internal_reservedSize=initialSize;
		internal_size=initialSize;
		internal_data[0]=0;
	}
	fn internal_deinitialize()()
	{
		if(internal_data!=nullptr) internal_free(internal_data);
		internal_clear();
	}
	fn internal_setTo()(ptr(u8) array,i64 start,i64 end_,u8 value)
	{
		for(i64 i=start;i<end_;i++)
		{
			array[i]=value;
		}
	}
	fn internal_copy()(ptr(u8) a,ptr(u8) b,i64 bytes)
	{
		for(i64 i=0;i<bytes;i++)
		{
			a[i]=b[i];
		}
	}
	fn internal_clear()()
	{
		internal_data=nullptr;
		internal_size=0;
		internal_reservedSize=0;
	}
	fn internal_allocate(ptr(u8))(i64 space)
	{
		ptr(u8) array=__malloc(space);
		if(array==nullptr) throw RuntimeException(r"Not enough memory for string");
		return array;
	}
	fn internal_free()(ptr(u8) array)
	{
		__free(array);
	}
}

fn operator(+)(string)(string a,string b)
{
	string c=a;
	c.add(b);
	return c;
}

fn operator(==)(u8)(string a,string b)
{
	if(a.size()!=b.size()) return false;
	for(i64 i=0;i<a.size();i++)
	{
		if(a[i]!=b[i]) return false;
	}
	return true;
}

fn operator(!=)(u8)(string a,string b)
{
	return !(a==b);
}

fn print()(string str)
{
	printr(str.data());
}

class List
{
	ptr(dynamic) internal_data;
	i64 internal_size;
	i64 internal_reservedSize;
	
	fn constructor()()
	{
		internal_initialize();
	}
	fn destructor()()
	{
		internal_deinitialize();
	}
	fn operator(=)()(List other)
	{
		if(addressof(other)==addressof(this)) return;
		internal_resize(other.internal_size);
		internal_copy(internal_data,other.internal_data,internal_size);
	}
	
	fn size(i64)()
	{
		return internal_size;
	}
	fn data(ptr(dynamic))()
	{
		return internal_data;
	}
	fn clear()()
	{
		internal_resize(0);
	}
	fn resize()(i64 newSize)
	{
		internal_resize(newSize);
	}
	fn capacity(i64)()
	{
		return internal_reservedSize;
	}
	fn reserve()(i64 newCapacity)
	{
		if(newCapacity>internal_reservedSize)
		{
			internal_setCapacity(newCapacity);
		}
	}
	fn shrinkToFit()()
	{
		internal_setCapacity(internal_size);
	}
	fn add()(List other)
	{
		if(other.internal_data!=nullptr)
		{
			i64 oldSize=internal_size;
			i64 otherSize=other.internal_size;
			internal_resize(oldSize+otherSize);
			internal_copy(internal_data+oldSize,other.internal_data,otherSize);
		}
	}
	fn put()(dynamic element)
	{
		internal_resize(internal_size+1);
		internal_data[internal_size-1]=element;
	}
	fn operator([])(ref(dynamic))(i64 index)
	{
		return internal_data[index];
	}
	fn at(ref(dynamic))(i64 index)
	{
		if(u64(index)>=u64(internal_size)) throw RuntimeException(r"Out of range of List");
		return internal_data[index];
	}
	fn front(ref(dynamic))()
	{
		return at(0);
	}
	fn back(ref(dynamic))()
	{
		return at(internal_size-1);
	}
	
	fn internal_resize()(i64 newSize)
	{
		if(newSize==internal_size) return;
		if(newSize<internal_size)
		{
			internal_destruct(internal_data,newSize,internal_size);
			internal_size=newSize;
		}
		else
		{
			if(newSize>internal_reservedSize)
			{
				i64 newReservedSize=newSize*2;
				internal_setCapacity(newReservedSize);
			}
			internal_construct(internal_data,internal_size,newSize);
			internal_size=newSize;
		}
	}
	fn internal_setCapacity()(i64 newReservedSize)
	{
		if(newReservedSize==internal_reservedSize) return;
		if(newReservedSize<internal_size) return;
		if(newReservedSize==0)
		{
			if(internal_data!=nullptr)
			{
				internal_destruct(internal_data,0,internal_size);
				internal_free(internal_data);
			}
			internal_clear();
			return;
		}
		ptr(dynamic) newData=internal_allocate(newReservedSize);
		internal_copy_memory(newData,internal_data,internal_size);
		if(internal_data!=nullptr) internal_free(internal_data);
		internal_data=newData;
		internal_reservedSize=newReservedSize;
	}
	fn internal_initialize()()
	{
		internal_clear();
	}
	fn internal_deinitialize()()
	{
		if(internal_data!=nullptr)
		{
			internal_destruct(internal_data,0,internal_size);
			internal_free(internal_data);
		}
		internal_clear();
	}
	fn internal_construct()(ptr(dynamic) array,i64 start,i64 end_)
	{
		i64 byteStart=start*sizeof(dynamic);
		i64 byteEnd_=end_*sizeof(dynamic);
		ptr(u8) address=ptr(u8)(array);
		for(i64 i=byteStart;i<byteEnd_;i++)
		{
			address[i]=0;
		}
	}
	fn internal_destruct()(ptr(dynamic) array,i64 start,i64 end_)
	{
		for(i64 i=start;i<end_;i++)
		{
			array[i].destructor();
		}
	}
	fn internal_copy_memory()(ptr(dynamic) a,ptr(dynamic) b,i64 count)
	{
		i64 bytes=count*sizeof(dynamic);
		ptr(u8) addressA=ptr(u8)(a);
		ptr(u8) addressB=ptr(u8)(b);
		for(i64 i=0;i<bytes;i++)
		{
			addressA[i]=addressB[i];
		}
	}
	fn internal_copy()(ptr(dynamic) a,ptr(dynamic) b,i64 count)
	{
		for(i64 i=0;i<count;i++)
		{
			a[i]=b[i];
		}
	}
	fn internal_clear()()
	{
		internal_data=nullptr;
		internal_size=0;
		internal_reservedSize=0;
	}
	fn internal_allocate(ptr(dynamic))(i64 number)
	{
		ptr(dynamic) array=ptr(dynamic)(__malloc(number*sizeof(dynamic)));
		if(array==nullptr) throw RuntimeException(r"Not enough memory for List");
		return array;
	}
	fn internal_free()(ptr(dynamic) array)
	{
		__free(ptr(u8)(array));
	}
}




fn fileToString(string)(string filepath)
{
	i64 fd=__open(filepath.data(),0,0);
	if(fd==-1)
	{
		throw "Could not open the file '"+filepath+"'";
	}
	
	i64 fileSize=0;
	{
		__struct_stat stat;
		if(__fstat(fd,addressof(stat))==-1)
		{
			throw "Could not get the size of the file '"+filepath+"'";
		}
		fileSize=stat.st_size;
	}
	
	string str;
	str.resize(fileSize);
	
	i64 ret=__read(fd,str.data(),fileSize);
	if(ret==-1 || ret!=fileSize)
	{
		throw "Could not read from the file '"+filepath+"'";
	}
	
	if(__close(fd)==-1)
	{
		throw "Could not close the file '"+filepath+"'";
	}
	
	return str;
}

fn stringToFile()(string str,string filepath)
{
	i64 fd=__open(filepath.data(),0b1001000001,0b111111111);
	if(fd==-1)
	{
		throw "Could not open the file '"+filepath+"'";
	}
	
	i64 fileSize=str.size();
	
	i64 ret=__write(fd,str.data(),fileSize);
	if(ret==-1 || ret!=fileSize)
	{
		throw "Could not write to the file '"+filepath+"'";
	}
	
	if(__close(fd)==-1)
	{
		throw "Could not close the file '"+filepath+"'";
	}
}




class Compiler
{
	//TODO----
	
	fn compile(string)(string inputCode)
	{
		//TODO----
		
		return "";//TODO----
	}
}

fn main(i64)(i64 argc,ptr(ptr(u8)) argv)
{
	try
	{
		List args;
		for(i64 i=1;i<argc;i++)
		{
			args.put(string(argv[i]));
		}
		
		if(args.size()!=2)
		{
			throw "Expected 2 arguments. Example: './bolgegc code.c code.asm'";
		}
		string inputFilepath=args[0];
		string outputFilepath=args[1];
		
		Compiler compiler;
		
		string code=fileToString(inputFilepath);
		
		string assembly=compiler.compile(code);
		
		stringToFile(assembly,outputFilepath);
	}
	catch(dynamic e)
	{
		if(typeof(e)==string)
		{
			string str=e;
			print(str+"\n");
		}
		else throw e;
	}
	return 0;
}
