#ifndef VIGENERE_H_INCLUDED
#define VIGENERE_H_INCLUDED

#include <string.h>
#include "sha256.h"

class Vigenere
{
	private:
        char** 	crypt_table=nullptr;

        SHA256_BIN	pass_org; // original password
        SHA256_BIN	pass_cur; // current password for encrypt/decrypt
        int		pass_pos=0; // current position of symbol in password

        void Constructor(const char*);
		char ROL(char, unsigned char);
		char getPassChar();

	public:
		Vigenere(const char*);
		Vigenere();
		~Vigenere();

		void setPassword(const char*);
		void reset();

		char encodeChar(char);
		void encodeBuf(void*, void*, int);
		void encodeBuf(void*, int);

		char decodeChar(char);
		void decodeBuf(void*, void*, int);
		void decodeBuf(void*, int);
};


#endif // VIGENERE_H_INCLUDED
