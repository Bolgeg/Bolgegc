fn func(i64)(i64 a,i64 b)
{
	
}

class Fuou
{
	u8 a;
	i16 b;
	i8 c;
}

class Type
{
	i64 a;
	i32 b;
	u8 c;
	ptr(i64) d;
	
	fn constructor()()
	{
		this.a=5;
		this.b=0;
		this.c=b'A';
		this.d=nullptr;
	}
}

class Foo
{
	i32 a;
	Type b;
	ptr(Type) c;
}

class Fua
{
	Foo f;
	Foo fofo;
	ptr(Fua) p;
	
	fn fuf(Foo)()
	{
		
	}
}

ptr(i64) pointer;

Fua f;
u8 m;
Foo foo;

fn main(i64)(i64 argc,ptr(ptr(u8)) argv)
{
	ptr(u8) stringPtr=r"Hello world\n";
	i64 length=12;
	
	if(length>0)
	{
		Fua f;
		Fua f2=f;
		Fua f3=Fua();
	}
	
	for(i64 i=0;i<10;i=i+1)
	{
		asm("mov rax,1"
			"mov rdi,1"
			"syscall"
			,"rsi"(stringPtr),"rdx"(length));
	}
	
	return 0;
}
