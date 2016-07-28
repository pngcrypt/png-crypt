#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <ctype.h>

const double M_PI=3.14159265358979323846 ;

// my custom functions

//===============================================
float floor_f(float f, int signs)
//===============================================
{
	float f1 = floor(f);
	return f1 + (float)((int)((f-f1)*signs) % signs)/signs;
}

//===============================================
double floor_d(double f, int signs)
//===============================================
{
	double f1 = floor(f);
	return f1 + (double)((int)((f-f1)*signs) % signs)/signs;
}

//===============================================
int stricmp(const char* s1, const char* s2)
//===============================================
{
	// strings compare with case insensitivity
	if(!s1 || !s2)
		return (int)(s1==s2);

    while(true)
	{
		if(!*s1 || !*s2)
			return (int)(*s1==*s2);
		if(tolower(*s1) != tolower(*s2))
			return 0;
		s1++;
		s2++;
	}
}

//===============================================
char* strchr_replace(char* str, char chFnd, char chRep)
//===============================================
{
	// replace all chars chFnd with chRep in string str

	if(str)
	{
		char* s=str;

		while(*s)
		{
			if(*s == chFnd)
				*s = chRep;
			s++;
		}
	}
	return str;
}

//===============================================
int file_exist(const char* filename)
//===============================================
{
	if(FILE* fptr = fopen(filename, "rb"))
	{
		fclose(fptr);
		return 1;
	}
	return 0;
}

//===============================================
int file_close(FILE** f)
//===============================================
{
	int err=fclose(*f);
	if(!err)
		*f = NULL; // success closed
	return err;
}

//===============================================
int file_size(FILE* f)
//===============================================
{
	if(!f)
		return -1;
	fseek(f,0,SEEK_END);
	int fSize = ftell(f);
	rewind(f);
	return fSize;
}

//===============================================
int fname_correct(char* fname)
//===============================================
{
	// checking of correct file name (result: 1 - ok; 0 - error)
	if(!fname || !*fname)
		return 0; // file name is empty

	char ch;

	if(*fname==' ') // first symbol is space
		return 0;
	while((ch=*fname))
	{
		if(ch < ' ')
			return 0;
		switch(ch)
		{
			// check illegal characters
			case '/':
			case '\\':
			case '*':
			case '?':
				return 0;
			break;
		}
		fname++;
	}
	if(*(fname-1)==' ') // last symbol is space
		return 0;

	return 1;
}

//===============================================
int random(int range_min, int range_max)
//===============================================
{
   // Generate random numbers in interval [range_min, range_max]

    if(range_max < range_min)
		range_max = range_min;

	return range_min + rand() % (range_max - range_min + 1);
}

//===============================================
int random(int range_max)
//===============================================
{
   // Generate random numbers in interval [0, range_max]
	return rand() % (range_max + 1);
}

//===============================================
double rand_norm()
//===============================================
{
	// normalized random (-1..1)
    double x=0;
	for(short i=0;i<12;i++)
	{
		x += (double)(rand()%1000)*0.001;
	}
    return (x-6)/6;
}

//===============================================
double NoiseInt(int x)
//===============================================
{
	// static noise
    x = (x<<13) ^ x;
	return ( 1.0f - ( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

//===============================================
double Noise2D(int x, int y)
//===============================================
{
	// static 2d noise
	return NoiseInt(x + y * 57);
}

//===============================================
double AWGN_generator()
//===============================================
{
	/* Генерация аддитивного белого гауссовского шума с нулевым средним и стандартным отклонением, равным 1. */

	double t1,t2=0;

	t1 = cos( ( 2.0 * (double)M_PI ) * rand() / (double)RAND_MAX );
	while(t2==0)
	{
		t2 = (double)rand() / (double)RAND_MAX;
	}
	return sqrt(-2.0 * log(t2)) * t1;

}

//===============================================
char getchl()
//===============================================
{
	return tolower(getch());
}

//===============================================
char getchYN(bool print)
//===============================================
{
	char ch;
    while(true)
	{
		ch=getchl();
		if(ch=='y' || ch=='n')
		{
			if(print)
				printf("%c\n",ch);
			return ch;
		}
	}
}

char getchYN()
{
	return getchYN(false);
}

//===============================================
void pause(bool anykey, bool msg)
//===============================================
{
	if(msg)
		printf("\nPress %s to continue...", anykey ? "any key" : "ENTER");
	if(anykey)
		getch();
	else
		while(getch() != 13) {};
	if(msg)
		puts("");
}

//===============================================
void pause(bool anykey)
//===============================================
{
	pause(anykey, true);
}
//===============================================
void pause()
//===============================================
{
	pause(false, true);
}
