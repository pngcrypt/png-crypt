// Improved encrypting algorithm of Vigenere

#include "vigenere.h"

//===============================================
Vigenere::Vigenere(const char* password)
//===============================================
{
	// constructor
	Vigenere::Constructor(password);
}

//===============================================
Vigenere::Vigenere()
//===============================================
{
	Vigenere::Constructor("");
}

//===============================================
void Vigenere::Constructor(const char* password)
//===============================================
{
	// common constructor
	Vigenere::setPassword(password);
}

//===============================================
Vigenere::~Vigenere()
//===============================================
{
	// destructor
	if(crypt_table)
	{
		for(int i=0; i<256; i++)
			delete [] crypt_table[i];

		delete [] crypt_table;
	}
}


//===============================================
void Vigenere::setPassword(const char* password)
//===============================================
{
	// set new password for new encode/decode
	sha256_bin(password, &pass_org);
	Vigenere::reset();
}

//===============================================
void Vigenere::reset()
//===============================================
{
	// reset all parameters for new encode/decode
	memcpy(pass_cur, pass_org, SHA256_BIN_SIZE);
	pass_pos = 0;

	// -- CREATE CRYPT TABLE --

	int i,j;
	unsigned char b,bc;

	b = pass_cur[0]; // get first char for table from password

	bool bCreate = (bool)(!crypt_table);

	if(bCreate)
		crypt_table = new char* [256];

	for(i=0; i<256; i++)
	{
		// filling the table
		if(bCreate)
			crypt_table[i] = new char[256];
		bc = b;
		for(j=0; j<256; j++)
		{
			crypt_table[i][j] = bc;
			bc++;
		}
		b++;
	}

}

//===============================================
char Vigenere::ROL(char ch, unsigned char num)
//===============================================
{
	// cyclical rotation of bits to left on num times (num = 0..8)
	unsigned b;
	b = (unsigned char)ch << num;
	b = (b & 0xFF) | (b >> 8);

	return (char)(b & 0xFF);
}

//===============================================
char Vigenere::getPassChar()
//===============================================
{
	// return current char of password
	char ch = pass_cur[pass_pos];

	pass_pos++; // next char
	if(pass_pos >= SHA256_BIN_SIZE)
	{
		// new password iteration
		int i;
		pass_pos = 0;

		// rotate all bits of each char
		for(i=0; i<SHA256_BIN_SIZE; i++)
		{
			pass_cur[i] = Vigenere::ROL(pass_cur[i], 1);
		}
	}

	return ch;
}

//===============================================
char Vigenere::encodeChar(char ch)
//===============================================
{
	// encode one char and return result
	unsigned char pch,ech;

	pch = getPassChar();
	ech = crypt_table[pch][(unsigned char)ch];
	return ech;
}

//===============================================
void Vigenere::encodeBuf(void* dst_buf, void* src_buf, int buf_len)
//===============================================
{
	// encode all chars from buffer src_buf, result will be placed in dst_buf
	int i;

	for(i=0; i<buf_len; i++)
	{
		((char*)dst_buf)[i] = Vigenere::encodeChar(((char*)src_buf)[i]);
	}
}

//===============================================
void Vigenere::encodeBuf(void* src_buf, int buf_len)
//===============================================
{
	// encode all chars in buffer src_buf (with replace original chars)
	Vigenere::encodeBuf(src_buf, src_buf, buf_len);
}

//===============================================
char Vigenere::decodeChar(char ch)
//===============================================
{
	// decode one char and return result
	char pch;
	char* ech;
	int i;

	pch = getPassChar();
	ech = crypt_table[(unsigned char)pch];
	for(i=0; i<256; i++)
	{
		if(ech[i] == ch)
			return (char)i;
	}

	return ch; // this should not happen
}

//===============================================
void Vigenere::decodeBuf(void* dst_buf, void* src_buf, int buf_len)
//===============================================
{
	// decode all chars from buffer src_buf, result will be placed in dst_buf
	int i;

	for(i=0; i<buf_len; i++)
	{
		((char*)dst_buf)[i] = Vigenere::decodeChar(((char*)src_buf)[i]);
	}
}

//===============================================
void Vigenere::decodeBuf(void* src_buf, int buf_len)
//===============================================
{
	// decode all chars in buffer src_buf (with replace original chars)
	Vigenere::decodeBuf(src_buf, src_buf, buf_len);
}
