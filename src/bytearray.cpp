#include "bytearray.h"

//============================================
unsigned char* bytearray::arr_ptr(size_t ofs)
//============================================
{
	if(!arr || ofs >= arr_size)
		return nullptr;
	return &arr[ofs];
}

//============================================
bytearray::bytearray()
//============================================
{
	// constructor
}

//============================================
bytearray::bytearray(size_t arrsize)
//============================================
{
	// constructor with array init
	bytearray::size(arrsize,0);
}

//============================================
bytearray::~bytearray()
//============================================
{
	// destructor
	bytearray::clear();
}

//============================================
unsigned char &bytearray::operator[](size_t ofs)
//============================================
{
	// access to array by index arr[N]
	return bytearray::arr_ptr(ofs)[0];
}

//============================================
unsigned char* bytearray::end()
//============================================
{
	// return pointer of last element || null
	return arr_ptr(arr_size-1);
}

//============================================
unsigned char* bytearray::begin()
//============================================
{
	// return pointer of first element || null
	return arr_ptr(0);
}

//============================================
void bytearray::clear()
//============================================
{
	// free array
	free(arr);
	arr_size = 0;
	arr_maxsize = 0;
	arr = nullptr;
}

//============================================
void bytearray::reset()
//============================================
{
	// set size to 0, but dont free memory (reserve memory)
	arr_size = 0;
}

//============================================
size_t bytearray::size()
//============================================
{
	// return array size
	return arr_size;
}

//============================================
size_t bytearray::size(size_t newsize)
//============================================
{
	// resize array (return: new size || 0)
	unsigned char* newarr;
	newarr = (unsigned char*)realloc((void*)arr, newsize);
	if(!newarr)
		newsize = 0; // no free memory
	else
	{
		arr = newarr;
		arr_size = newsize;
		arr_maxsize = newsize;
	}

	return newsize;
}

//============================================
size_t bytearray::size(size_t newsize, unsigned char fill_ch)
//============================================
{
	// resize array with fill the new added space by fill_ch
	size_t oldsize = arr_size;
	newsize = bytearray::size(newsize);
	if(newsize && oldsize < newsize)
		memset(&arr[oldsize], fill_ch, newsize-oldsize);
	return newsize;
}

//============================================
unsigned char* bytearray::add(void* buf, size_t bufsize)
//============================================
{
	// add any buffer
	if(!buf)
		return arr;

	size_t oldsize = arr_size;
	size_t newsize = oldsize + bufsize;

	if(newsize > arr_maxsize)
	{
		if(!bytearray::size(newsize))
			return arr;
	}
	memcpy(&arr[oldsize], buf, bufsize);
	arr_size = newsize;
	return arr;
}

//============================================
unsigned char* bytearray::add(bytearray &a2)
//============================================
{
	// add array
	return bytearray::add(a2.begin(), a2.size());
}

//============================================
char* bytearray::add(const char* str, int len)
//============================================
{
	// if len > 0 - add first len-chars of string str to array
	// if len < 0 - add last abs(len)-chars of string str to array

	int sl = strlen(str);

	if(!str)
		return (char*)arr;
	if(len < 0)
	{
		sl = sl + len;
		if(sl<0)
			sl = 0;
	}
	else
	{
		if(len > sl)
			len = sl;
		sl = 0;
	}

	if(arr_size && *bytearray::end()==0) // check if array is string ('\0' at the end)
		arr_size--;

	bytearray::add((void*)&str[sl], len);
	sl = 0;
	bytearray::add(&sl, 1); // add '\0' char
	return (char*)arr;
}

//============================================
char* bytearray::add(const char* str)
//============================================
{
	// add string str to array
	return bytearray::add(str, strlen(str));
}

//============================================
unsigned char* bytearray::get(size_t ofs, void* buf, size_t bufsize)
//============================================
{
	// copy data form array to buf (if bufsize + ofs > array size, return null)

	if(!buf || ofs+bufsize > arr_size)
		return nullptr;
	unsigned char* pch = &arr[ofs];
	memcpy(buf, pch, bufsize);
	return pch;
}

//============================================
unsigned char* bytearray::get(size_t ofs, unsigned char* ch)
//============================================
{
	// get char from array: .get(2, &ch)
	return bytearray::get(ofs, ch, 1);
}

//============================================
char* bytearray::str()
//============================================
{
	// return array as char* string
	return (char*)arr;
}

//============================================
char* bytearray::str(const char* s)
//============================================
{
	// copy string s to array, set size of array to length of s
	bytearray::size(strlen(s)+1);
	memcpy(arr, s, arr_size);
	return (char*)arr;
}
