#include <stdio.h>

void abcd()
{
	printf("abcd");
}
void abc()
{
	printf("abc");
}
void abd()
{
	printf("abd");
}
void acd()
{
	printf("acd");
}
void bcd()
{
	printf("bcd");
}
void ab()
{
	printf("ab");
}
void ac()
{
	printf("ac"); //29
}
void ad()
{
	printf("ad");
}
void bc()
{
	printf("bc");
}
void bd()
{
	printf("bd");
}
void cd()
{
	printf("cd");
}
void a()
{
	printf("a");
}
void b()
{
	printf("b");
}
void c()
{
	printf("c");
}
void d()
{
	printf("d");
}
void null()
{
	printf("null");
}
int main()
{
	const int A=1;
	const int B=2;
	const int C=3;
	const int D=4;

	klee_make_symbolic(&A, sizeof(A), "A");
	klee_make_symbolic(&B, sizeof(B), "B");
	klee_make_symbolic(&C, sizeof(C), "C");
	klee_make_symbolic(&D, sizeof(D), "D");

	if (A>0) 
	{
		if (B>0) 
		{
			if (D>0) 
			{
				if (D>0) abcd();
				else abc();
			}
			else 
			{
				if (D>0) abd();
				else ab();
			}
		}
		else 
		{
			if (C>0) 
			{
				if (D>0) acd();
				else ac();
			}
			else 
			{
				if (D>0) ad();
				else a();
			}
		}
	}
	else	
	{	if (B>0) 
		{
			if (C>0) 
			{
				if (D>0) bcd();
				else bc();
			}
			else 
			{
				if (D>0) bd();
				else b();
			}
		}
		else 
		{
			if (C>0) 
			{
				if (D>0) cd();
				else c();
			}
			else 
			{
				if (D>0) d();
				else null();
			}
		}
	}
return 0;
}
