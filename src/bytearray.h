#ifndef BYTEARRAY_H_INCLUDED
#define BYTEARRAY_H_INCLUDED

#include <stdlib.h>
#include <memory.h>

class bytearray
{
private:
	unsigned char 	*arr=nullptr;
	size_t			arr_size=0;
	size_t			arr_maxsize=0;

	unsigned char* arr_ptr(size_t ofs);

public:
	bytearray();
	bytearray(size_t arrsize);
	~bytearray();

	unsigned char &operator[](size_t ofs);

	size_t size();
	size_t size(size_t newsize);
	size_t size(size_t newsize, unsigned char fill_ch);

	void clear();
	void reset();
	unsigned char* end();
	unsigned char* begin();

	unsigned char* add(void* buf, size_t bufsize);
	unsigned char* add(bytearray &a2);
	char* add(const char* str, int len);
	char* add(const char* str);

	unsigned char* get(size_t ofs, void* buf, size_t bufsize);
	unsigned char* get(size_t ofs, unsigned char* ch);

	char* str();
	char* str(const char* s);
};

#endif // BYTEARRAY_H_INCLUDED
