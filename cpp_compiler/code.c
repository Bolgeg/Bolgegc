fn __syscall(i64)(i64 syscallNumber,i64 arg0,i64 arg1,i64 arg2,i64 arg3,i64 arg4,i64 arg5)
{
	i64 returnValue;
	asm("syscall",
		"rax"(syscallNumber),
		"rdi"(arg0),"rsi"(arg1),"rdx"(arg2),"r10"(arg3),"r8"(arg4),"r9"(arg5),
		"=rax"(returnValue));
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



fn strlen(i64)(ptr(u8) str)
{
	i64 i=0;
	while(str[i])
	{
		i++;
	}
	return i;
}

fn print()(ptr(u8) str)
{
	__write(1,str,strlen(str));
}

fn main(i64)(i64 argc,ptr(ptr(u8)) argv)
{
	MemoryAllocationSystem memoryAllocationSystem;
	memoryAllocationSystem.initialize();
	PROCESS_MEMORY_ALLOCATION_SYSTEM=addressof(memoryAllocationSystem);
	
	print(r"Hello world\n");
	
	ptr(u8) array=__malloc(1024);
	
	array[0]=b'A';
	array[1]=b'\n';
	array[2]=0;
	
	print(array);
	
	__free(array);
	
	return 0;
}
