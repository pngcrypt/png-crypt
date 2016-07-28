#ifndef CRYPT_H_INCLUDED
#define CRYPT_H_INCLUDED

#include "vigenere.h"

class crypt
{
	private:
		Vigenere* engine;

	public:
		crypt(const char* password);
		~crypt();

		void encodeBuf(void* buf, int len);
		void decodeBuf(void* buf, int len);
};


#endif // CRYPT_H_INCLUDED
