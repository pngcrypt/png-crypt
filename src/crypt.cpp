
// this is class to implement encryption engine

#include "crypt.h"

crypt::crypt(const char* password)
{
	// constructor
	engine = new Vigenere(password); // Vigenere encryption (for example)
}

crypt::~crypt()
{
	// destructor
	delete engine;
}

void crypt::encodeBuf(void* buf, int len)
{
	engine->encodeBuf(buf, len);
}

void crypt::decodeBuf(void* buf, int len)
{
	engine->decodeBuf(buf, len);
}
