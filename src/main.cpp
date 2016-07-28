// NEED LIBRARY: libgdiplus

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <gdiplus.h>

#include "bytearray.h"
#include "functions.h"
#include "crypt.h"

using namespace std;

const char* TOOL_VERSION="0.3 beta";

//const CLSID pngEncoderClsId = { 0x557cf406, 0x1a04, 0x11d3,{ 0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e } }; // png CLSID
CLSID	clsidPNG;

enum errCodes { // tool's exit codes
	errNone,
	errParam,
	errInImgLost,
	errInImgFormat,
	errInImgSize,
	errDatLost,
	errDatRead,
	errDatWrite,
	errDatSize,
	errOutImgName,
	errOutImgWrite,
	errOutImgConvert,
	errDecData
};

ULONG_PTR   	gdiplusToken=0;
Gdiplus::Bitmap*  img=NULL;

BOOL			debug=false;
int				debug_ch=0;

BYTE            imgBPP;      // bit per pixel of image
BOOL            imgAlpha;    // image has alpha channel
int				imgAlphaUse=-1; // Use alpha for data: -1 - use if alpha exists; 0 - don't use; 1 - use (and add if no alpha); 2 - don't use and delete alpha
UINT            imgPixels, imgW, imgH;
UINT			imgBytes;	// max bytes for data on image
BYTE			imgChannels; // number of channels per Pixel (3, 4 == RGB, ARGB)
int				imgBit;		// current bit of data in pixel (var)
double			imgOfs;		// offset of current pixel (var)
double			imgNoise=-1;
UINT			imgResStep; // reserved pixels for step (calc)
const UINT		imgResBPC=1;  // reserved pixels for BPÑ

crypt* 			datCrypt=NULL;
BOOL            datEncode=true; // true - encode, false - decode
UINT            datBPP;      // total bits per pixel of data
BYTE			datBPCmin=1; 	// minimum bits per channel for data (1..8)
BYTE			datBPCmax=4; 	// maximum bits per channel for data (1..8)
BYTE			datBPC=datBPCmin; 	// bits per channel for data (1..8)
FILE*           datFile=NULL;
//float			datStep=0;		// step (pixels)
double			datStep=0;		// step (pixels)
const UINT		datResBPC = sizeof(datBPC); // reserved bytes for used bit per channel
const UINT		datResStep = sizeof(datStep); // reserved bytes for step
const UINT		datResSize = sizeof(UINT); // reserved bytes for data size
const UINT		datReserved = datResBPC + datResStep + datResSize;
UINT			datBytesProcessed=0; // debug

BOOL			bOverwrite = false; // force overwrite output files
BOOL			bCrypt = false; // crypt data
BOOL			bPauseOnExit = true; // wait <any key> on errors

bytearray		sInImg;      // input image file name
bytearray		sOutImg;     // output image file name
bytearray		sData;       // data file name
bytearray		sPassword;	 // password

//===============================================
void Die(errCodes err=errNone, const char* msg=NULL, int showUsage=0, ...)
//===============================================
{
	// shutdown with error code and message
	// showUsage - show usage info (0 - disable, 1 - short info, 2 - full info)
	// msg - could be a format string, additional parameters must be set after showUsage

	if(msg)
	{
		va_list list;
		va_start (list, showUsage);
		printf("\n*Error*: ");
		vprintf(msg, list);
		printf("\n");
		va_end(list);
	}

	if(showUsage)
	{
		puts("\nUsage: png-crypt [<switches>] <image_file> <data_file> [<out_image>]\n");
		if(showUsage == 1)
			puts("png-crypt -? for more info\n");
		else
		{
			puts("Where <switches> could be:\n");

			puts("\t-e - encode <data_file> into image <image_file> [default]");
			puts("\t\tIf no <out_image> defined will be used: <image_file>.enc.png");
			puts("\t\tInput <image_file> could be: tif,gif,jpg,png,bmp\n");

			puts("\t-b:N - set min. bits per channel for encode (1..8) [default: 1]\n");

			puts("\t-B:N - set max. bits per channel for encode (1..8) [default: 4]\n");

			puts("\t-a:N - force disable/enable alpha channel for data (0..1, d)");
			puts("\t\t-a:0 - not use alpha channel for data");
			puts("\t\t-a:1 - use alpha channel for data");
			puts("\t\t-a:d - delete alpha channel\n");

			puts("\t-n[:N] - add noise (fake data) before encode");
			puts("\t\t-n:N - N is density of noise in % (1..100)");
			puts("\t\t-n - add random noise in range (5..50%)\n");

			puts("\t-p:PASSWORD - set password for encrypt/decrypt data\n");

			puts("\t-y - force overwrite output file if it exist\n");

			puts("\t-k - disable \"any key\" pause on program finish\n");

			puts("\t-d - decode encoded png <image_file>");
			puts("\t\tIf <data_file> defined the decoded data will be stored in it");
			puts("\t\totherwise file name will be extracted from the encoded data\n");
		}
	}

	// free resources
	delete img;
	if(gdiplusToken != 0)
		Gdiplus::GdiplusShutdown(gdiplusToken);
	if(datFile != NULL)
		fclose(datFile);
	delete datCrypt;

	if(debug || bPauseOnExit)
		pause(true);

	exit(err);
}

//===============================================
int ParseParams(int argc, char ** argv)
//===============================================
{
	// parse command line
	int		n,fn,vi;
	PCHAR 	p,v,viErr,vfErr;
	double	vf;
	BOOL	err=false;

	if(argc < 2)
		Die(errParam, NULL, 1); // show usage

	fn=0;
	for(n=1; n<argc; n++)
	{
		p = argv[n];

		if(!fn)
		{
			// check for switches
			if(p[0] != '-')
				fn = 1;
			else
			{
				// getting value of switch (after ':' symbol)
				vi=0;
				viErr = NULL;
				vf=0;
				vfErr = NULL;
				v = strchr(p, ':'); // search for splitter
				if(v)
				{
					*v = '\x00'; // split parameter and value
					v++; // string value
					vi = (int)strtol(v, &viErr, 10); // getting int value (on success viErr will point to the '\0' char)
					viErr = (PCHAR)((UINT)*viErr); // viErr == NULL (on success) or !NULL (on errors)
					vf = (double)strtof(v, &vfErr); // getting float value
					vfErr = (PCHAR)((UINT)*vfErr); // viErr == NULL (on success) or !NULL (on errors)
				}
				if(strlen(p)>2) // max length of parameter name (including '-' char)
					fn = 1;

				/*
				How to check values:
				!v == no value
				v == string representation of value

				!viErr == value is integer
				viErr == value is not integer
				vi - integer representation of value

				!vfErr == value is float
				vfErr == value is not float
				vf - float representation of value
				*/
			}
		}

		if(fn)
		{
			// parse file names
			switch(fn)
			{
				case 1: // input image
					sInImg.add(p);
				break;

				case 2: sData.add(p); break;  // data file

				case 3: sOutImg.add(p); break;  // out image

				default:
					return 1;
			}
			fn++;
		}
		else
		{
			// parse switches
			switch(p[1])
			{
				case '?':
				case 'h':
					// show help/usage
					Die(errNone, NULL, 2);
				break;

				case 'e': datEncode=true; break; // encode
				case 'd': datEncode=false; break; // decode
				case 'y': bOverwrite = true; break; // force overwrite output files
				case 'k': bPauseOnExit = false; break;// dont "press any key" on exit

				case 'b': // minimum bits per channel (1..8)
					if(!v || viErr || vi<1 || vi>8)
						err = true;
					else
						datBPCmin = vi;
				break;

				case 'B': // maximum bits per channel (1..8)
					if(!v || viErr || vi<1 || vi>8)
						err = true;
					else
						datBPCmax = vi;
				break;

				case 'a': // force alpha (0/1 == disable/enable)
					if(!strcmp(v, "d"))
						imgAlphaUse = 2; // delete alpha
					else if(!v || viErr || vi<0 || vi>1)
						err=true;
					else
						imgAlphaUse=vi;
				break;

				case 'n': // add noise (0..100%, 0 == disable)
                    if(viErr || vi<0 || vi > 100)
						err = true;
					else
					{
						if(!v)
							imgNoise = random(5,50); // random density of noise
						else if(vi >= 0)
							imgNoise = vi;
					}

				break;

				case 'p': // password
                    if(!v || !strlen(v))
						err = true;
					else
					{
						bCrypt = true;
						sPassword.add(v);
					}
				break;

				case '#': debug = true; break;

				default: // unknown switch
					err = true;
			} // switch
		} // else fn

		if(err)
			break;
	} // for n

	if(err)
	{
		if(v)
			v[-1] = ':'; // restore delimiter
		Die(errParam, "Bad parameter: %s", 1, p);
	}

	return 1;
}

//===============================================
void GetEncoderCLSID(GUID formatID, CLSID* encoderCLSID)
//===============================================
{
	// return encoder CLSID (encoderCLSID)
	// formatID - GUID of image format (Gdiplus::ImageFormatPNG, Gdiplus::ImageFormatBMP, etc)

	UINT  num,i;        // number of image encoders
	UINT  size;       // size, in bytes, of the image encoder array
	Gdiplus::ImageCodecInfo* pImageCodecInfo;

	memset(encoderCLSID, 0, sizeof(CLSID)); // default: null CLSID
	Gdiplus::GetImageEncodersSize(&num, &size);
	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size)); // Create a buffer large enough to hold the array of ImageCodecInfo
	GetImageEncoders(num, size, pImageCodecInfo);
	for(i=0; i<num; i++)
	{
		if(!memcmp(&pImageCodecInfo[i].FormatID, &formatID, sizeof(GUID)))
		{
			*encoderCLSID = pImageCodecInfo[i].Clsid;
			break;
		}
	}
	free(pImageCodecInfo);
}

//===============================================
int LoadImg(Gdiplus::Bitmap** img, char* fName)
//===============================================
{
	// create object & load image (BMP, JPG, TIF, GIF, PNG)
	int len = strlen(fName)+1;
	WCHAR* wc = new WCHAR[len];
	mbstowcs(wc, fName, len);

	*img = new Gdiplus::Bitmap(wc);

	delete [] wc;
	return (int)((*img)->GetLastStatus() == Gdiplus::Ok);
}

//===============================================
int SaveImg(Gdiplus::Bitmap** img, char* fName)
//===============================================
{
	// save img
	int len = strlen(fName)+1;
	WCHAR* wc = new WCHAR[len];
	mbstowcs(wc, fName, len);

	(*img)->Save(wc, &clsidPNG, NULL);

	delete [] wc;
	return (int)((*img)->GetLastStatus() == Gdiplus::Ok);
}

//===============================================
int ConvertImgPF(Gdiplus::Bitmap** img, int PixelFormat)
//===============================================
{
	Gdiplus::Bitmap* img2 = (*img)->Clone(0, 0, (*img)->GetWidth(), (*img)->GetHeight(), PixelFormat);
	delete *img;
	*img = img2;

	return (int)((*img)->GetLastStatus() == Gdiplus::Ok);
}

//===============================================
UINT BytesToPixels(UINT bytes)
//===============================================
{
	// returns the number of pixels required to store a defined number of bytes
    return ceil(double(bytes*8) / datBPP);
}

//===============================================
UINT PixelsToBytes(UINT pixels)
//===============================================
{
	// returns the number of bytes that fit into a defined number of pixels
    return floor(double(pixels*datBPP) / 8);
}

//===============================================
int PixelToCoord(UINT pixelNum, UINT* x, UINT* y)
//===============================================
{
	// converts pixel number to coords (x,y) of current image

    *y = UINT(pixelNum / imgW);
    *x = pixelNum - (*y * imgW);

	if(*y >= imgH)
	{
		*y = imgH-1;
		return 0;	// error - pixel out of image
	}
	return 1;
}

//===============================================
int CoordToPixel(UINT x, UINT y, UINT* pixelNum)
//===============================================
{
	// convert coords (x,y) to pixel number of current image
	if(y >= imgH)
		y = imgH-1;
	if(x >= imgW)
		x = imgW-1;
	*pixelNum = (y*imgW)+x;

	return 1;
}

//===============================================
void SavePixel(UINT ofs, BYTE* rgba)
//===============================================
{
	// save pixel into current image by offset ofs
	UINT x,y;
	Gdiplus::Color	col;

	if(debug)
	{
		// show used pixels
		y = 0x80;
		for(x=8; x>datBPC; x--)
		{
			rgba[debug_ch] |= y;
			rgba[3] |= y;
			y = y >> 1;
		}
	}

	PixelToCoord(ofs, &x, &y);
	if(imgAlpha)
		col=Gdiplus::Color(rgba[3],rgba[0],rgba[1],rgba[2]);
	else
		col=Gdiplus::Color(rgba[0],rgba[1],rgba[2]);
	img->SetPixel(x,y,col);
}

//===============================================
void LoadPixel(UINT ofs, BYTE* rgba)
//===============================================
{
	// get pixel of current image by offset ofs
	UINT x,y;
	Gdiplus::Color	col;

	PixelToCoord(ofs, &x, &y);
	img->GetPixel(x,y, &col);
	rgba[0] = col.GetR();
	rgba[1] = col.GetG();
	rgba[2] = col.GetB();
	rgba[3] = col.GetA();
}

//===============================================
int WriteData(void* buf, UINT bytes, double step=datStep, BYTE bitPerByte=8, BYTE bitPerPixel=datBPP, BYTE chPerPixel=imgChannels)
//===============================================
{
	// write number of bytes from data buffer into current image

	// Next params can be used to force change the default params of encoding:
	// step - step of pixel increment
	// bitPerByte - number of bits in 1 byte of data (1..8)
	// bitPerPixel - bits per pixel of data
	// chPerPixel - used channels in pixel (1..4) (R,G,B,A)

	UINT		n,bt;
	BYTE		dat;
	BOOL		changed=false;
	static BYTE	rgba[4];	// R,G,B,A values of current pixel
	static BYTE chN=0; 		// number of channel (R,G,B,A == 0..imgChannels)
	static BYTE bitN=0;		// current bit of channel (0..datBPC)
	static BYTE	bitMask;

	for(n=0; n<bytes; n++)
	{
		dat = ((BYTE*)buf)[n];
		for(bt=0; bt<bitPerByte; bt++)
		{
			if(imgBit < 0)
			{
				// load next pixel
				LoadPixel(floor(imgOfs),rgba);
				imgBit = 0;
				chN = 0;
				bitN=0;
				bitMask=0xFE; // reset mask to 0-bit
			}
			rgba[chN] = (rgba[chN] & bitMask) | ((dat & 1) << bitN);

			dat = dat >> 1; // next bit of data
			chN++; // next channel
			if(chN >= chPerPixel)
			{
				// all bitN bits of each channel have been filled, go to the next bit
				chN = 0;
				bitMask = (bitMask << 1) | 1; // rotate mask to next left bit
				bitN++;
			}
			imgBit++; // total bits used in pixel

			if(imgBit >= bitPerPixel)
			{
				// all bits of current pixel have been filled
				SavePixel(floor(imgOfs),rgba);
				imgBit = -1;
				imgOfs += step; // increment to next pixel
				if(imgOfs >= imgPixels)
					return 0;	// overflow (out of bounds)
				changed = false;
			}
			else
				changed = true;
		} // for b
		datBytesProcessed++; // debug
	} // for n

	if(changed)
		SavePixel(floor(imgOfs),rgba); // force save changed pixel

	return 1;
}

//===============================================
int ReadData(void* buf, UINT bytes, double step=datStep, BYTE bitPerByte=8, BYTE bitPerPixel=datBPP, BYTE chPerPixel=imgChannels)
//===============================================
{
	// read data from image into buffer

	// Next params can be used to force change the default params of decoding:
	// step - step of pixel increment
	// bitPerByte - number of bits in 1 byte of data (1..8)
	// bitPerPixel - bit per pixel of data
	// chPerPixel - used channels in pixel (1..4) (R,G,B,A)

	UINT		n,bt;
	BYTE		dat;
	static BYTE	rgba[4];
	static BYTE chN=0;
	static BYTE bitN=0;

	for(n=0; n<bytes; n++)
	{
		dat = 0;
		for(bt=0; bt<bitPerByte; bt++)
		{
			// Gathering the next byte of data
			if(imgBit < 0)
			{
				// load next pixel
				LoadPixel(floor(imgOfs),rgba);
				imgBit = 0;
				chN = 0;
				bitN=0;
			}
			dat |= ((rgba[chN] >> bitN) & 1) << bt; // getting data bit from current channel (rotate to 0-pos, then rotate to current byte position of data)

			chN++; // next channel
			if(chN >= chPerPixel)
			{
				// all bitN bits of each channel have been read, go to the next bit
				chN = 0;
				bitN++;
			}
			imgBit++;

			if(imgBit >= bitPerPixel)
			{
				// all bits of current pixel have been read
				imgOfs += step; // increment to next pixel
				imgBit = -1;
				if(imgOfs >= imgPixels)
					return 0;	// overflow (out of bounds)
			}
		} // for b
		((BYTE*)buf)[n] = dat; // save data byte
		datBytesProcessed++; // debug
	} // for n

	return 1;
}

//===============================================
void AddNoise(double density)
//===============================================
{
	// density - density of noise (percent 1..100)
	double nn;
	UINT dat;
	int	i,n,rn;

	density = -0.12*log(0.009*density); // (0.01 == 95%) .. (0.42 == 1%)

	n = ((datBPP-1) / 8) + 1; // bytes in pixel for data

	rn=0;
	imgOfs = 0;
	while(imgOfs < imgPixels)
	{
		nn = rand_norm();
		if(nn<0)
			nn *= -1.0;
		if(nn>=density)
		{
			rn++; // real noise percent (debug)
			dat = 0;
			for(i=0; i<n; i++) // generate random data
				dat |= (UINT)random(255) << (i*8);
			WriteData(&dat, 1, 0, datBPP); // write random data to current pixel
		}
		imgOfs += 1;
	}
	if(debug)
		printf("# Actual Noise: %.2f%% (%.3f)\n", ((double)rn/imgPixels)*100, density);
}

//===============================================
int EncodeData()
//===============================================
{
	// read input data file, encode it into image, and save to output image

	UINT	tmp,fSize,nameSize,bufSize;
	bytearray buf;
	UINT bufmax=65535;
	size_t totalbytes;
	char*	ext;
	char*	sName=NULL;
	errCodes err;

	fSize = 0;
	nameSize = 0;

	err = errDatLost;
	// checking file names, file size
	while(sData.size()) // fake loop
	{
		if(!file_exist(sData.str()))
			break;

		if(!(datFile = fopen(sData.str(), "rb")))
		{
			err = errDatRead;
			break;
		}

		fSize = file_size(datFile); // get file size

		// get pointer to file name without path
		sName = strrchr(strchr_replace(sData.str(), '/', '\\'), '\\');
		if(!sName)
			sName = sData.str();
		else
			sName++;
		nameSize = strlen(sName); // data file name length
		if(nameSize > 255)
		{
			// trim long name
			sName = sName + nameSize - 255;
			nameSize = 255;
		}
		err = errNone;
		break;
	}
	bufSize = fSize+nameSize+1;

	// -- CALC BEST BPC between Min and Max --
	if(datBPCmax < datBPCmin)
		datBPCmax = datBPCmin;
	tmp = datResBPC + datResStep + datResSize + bufSize; // total bytes need for all of data
	datBPC=datBPCmin;
	while(datBPC<=datBPCmax)
	{
		datBPP = imgChannels * datBPC; // total bits per pixel of data
		imgBytes = PixelsToBytes(imgPixels); // total free bytes in image
		if(imgBytes >= tmp)
			break;
		datBPC++;
	}
	if(datBPC > datBPCmax)
		datBPC--;
	// <--

	imgResStep = BytesToPixels(datResStep); // reserved pixels for datStep
	imgBytes -= datReserved+nameSize+1; // free bytes for pure data (without headers)

	// -- SHOW INFO --
	if(imgBytes < 1)
		Die(errInImgSize, "Input image is too small");

	printf("Total pixels: %u\nFree bytes: %u (%ubpc, %ubpp)\n", imgPixels, imgBytes, datBPC, datBPP);

	if(!sData.size())
		Die(errDatLost, "Data file not defined");
	printf("\nInput data file: %s\n", sData.str());
	if(err != errNone)
	{
		if(err == errDatLost)
			Die(err, "Data file not found: %s", 0, sData.str());
		else
			Die(errDatRead, "Can't read file");
	}

	printf("File size: %u bytes\n\n", fSize);
	if(fSize > imgBytes)
		Die(errDatSize, "Data file is too big. Try -B:N switch");


	// -- GET OUTPUT FILE NAME --
	if(sOutImg.size())
	{
		// name defined by user
		ext = strrchr(sOutImg.str(), '.');
		if(!ext || !stricmp(".png", ext)) // checking extension
			Die(errOutImgName, "Output image file must be .PNG");
	}
	else
	{
		// file name not defined, making it from sInImg
        ext = strrchr(sInImg.str(), '.'); // pointer to extension
        if(!ext)
			tmp = sInImg.size(); // no extension
		else
			tmp = ext - sInImg.str();
		sOutImg.add(sInImg.str(), (int)tmp); // copy filename without extension
		sOutImg.add(".enc.png"); // new extension
	}

	// -- CALC STEP --
	tmp = BytesToPixels(bufSize + datResSize)+1; // pixels needs for data (+size)
	datStep = (double)(imgPixels - imgResStep - imgResBPC) / tmp; // float step - it is necessary for the uniform distribution of data
	datStep = floor_d(datStep, 1000); // rounding to the thousandths
	if(datStep<1)
		datStep = 1;

	printf("Used pixels: %u\n", tmp);
	printf("Original image changed: %.1f%%\n", ((double)(tmp*datBPP)/(imgPixels*imgBPP)*100.0)); // how many bits was changed, %
	if(debug)
		printf("# Step: %.2f\n", datStep);

	// -- WRITE NOISE --
	debug_ch=0; // red channel for noise debug
	if(imgNoise < 0 && bCrypt)
		imgNoise = random(25,50); // random noise when password used
	if(imgNoise > 0)
		AddNoise(imgNoise); // add noise
	debug_ch=1; // green for data debug

	// -- PACK HEADERS INTO IMAGE --
	imgOfs = 0; // begin of image
	imgBit = -1; // force load pixel
	tmp = datBPC | (imgAlphaUse << 3); // pack: BPC in 0..2 bits, imgAlphaUse in 3rd bit
	WriteData(&tmp, datResBPC, 1, 4, 4, 3); // write BPC & imgAlphaUse in 1st pixel

	buf.clear();
	buf.add(&datStep, datResStep);
	buf.add(&nameSize, 1);
	buf.add((void*)&sName[0], nameSize);
	buf.add(&fSize, datResSize);

	if(bCrypt)
		datCrypt->encodeBuf(&buf[0], buf.size());

	WriteData(&buf[0], datResStep, 1); // write step value
	WriteData(&buf[datResStep], buf.size() - datResStep); // file name & data size

	// -- PACK DATA FILE INTO IMAGE --
	if(fSize<bufmax)
		bufmax = fSize;

	buf.size(bufmax); // file read buffer
	datBytesProcessed=0; // calc for debug
	totalbytes=0; // total bytes read

	while(totalbytes < fSize)
	{
		tmp=fread(&buf[0], 1, bufmax, datFile); // read next file block
		if(!tmp)
			break; // EOF

		totalbytes += tmp;

		if(bCrypt)
			datCrypt->encodeBuf(&buf[0], buf.size());

		tmp=WriteData(&buf[0], tmp); // write data into image
		if(!tmp)
			Die(errDatWrite, "DATA OVERFLOW");
	}
	if(debug)
		printf("# Bytes written: %u\n",datBytesProcessed);
	if(file_close(&datFile) || totalbytes!=fSize) // file_close: 0 - success, !0 - fail
		Die(errDatRead, "Read error");

	buf.clear();

	// -- SAVE OUTPUT IMAGE --
	printf("Out image: %s\n", sOutImg.str());

	if(!imgAlpha)
		ConvertImgPF(&img, PixelFormat24bppRGB); // converting image to RGB 24bpp because no alpha was used

	if(!bOverwrite && file_exist(sOutImg.str()))
	{
		printf("\nFile '%s' is already exist.\nOverwrite? [Y/N] ", sOutImg.str());
		if(getchYN(true) == 'n')
			Die(errOutImgWrite, "Aborted.");
	}

	if(!SaveImg(&img, sOutImg.str()))
		Die(errOutImgWrite, "Image write error");

	return 1;
}

//===============================================
int DecodeData()
//===============================================
{
	// decode data from image
	UINT	tmp,dSize;
	bytearray buf;
	bytearray sName;
	UINT bufmax=65535;
	size_t totalbytes;
	//char*	ext;

	printf("\nDecoding data...\n");

	imgOfs = 0; // begin of image (1st pixel)
	imgBit = -1; // force load pixel

	// -- READ BPC & ALPHA --
	imgAlphaUse = 0;
	ReadData(&imgAlphaUse, datResBPC, 1, 4, 4, 3); // read 4 bits of first pixel

	datBPC = imgAlphaUse & 7; // extract BPC (0..2 bits)
	imgAlphaUse = !!(imgAlphaUse & 8); // extract AlphaUse flag (3rd bit)
	if(imgAlpha && !imgAlphaUse)
		imgAlpha = false; // disable alpha by flag only if alpha exist in image
	imgChannels = imgAlpha ? 4 : 3; // RGBA or RGB
	datBPP = imgChannels * datBPC; // total bits per pixel of data

	// -- READ STEP --
	datStep = 0;
	if(!ReadData(&datStep, datResStep, 1))	// read step of data
		return 0;
	if(bCrypt)
		datCrypt->decodeBuf(&datStep, datResStep);
	if(datStep < 1)
		return 0; // wrong data

	// -- READ FILE NAME --
	dSize = 0;
	if(!ReadData(&dSize, 1)) // read length of file name (1 byte)
		return 0;
	if(bCrypt)
		datCrypt->decodeBuf(&dSize, 1);

	sName.size(dSize+1, 0);
	if(!ReadData(&sName[0], dSize)) // read file name
		return 0;
	if(bCrypt)
		datCrypt->decodeBuf(&sName[0], dSize);

	/*
	ext = strrchr(strchr_replace(sName.str(), '/', '\\'), '\\'); // pointer to last slash
	if(ext)
		return 0; // wrong data: path found in filename - it's exploit
	*/

	if(!fname_correct(sName.str())) // check for correct file name (restricted symbols: *, ?, /, \\, etc)
		return 0;

	if(!sData.size())
		sData = sName;

	// -- READ DATA SIZE --
	dSize = 0;
	if(!ReadData(&dSize, datResSize)) // read length of data block
		return 0;
	if(bCrypt)
		datCrypt->decodeBuf(&dSize, datResSize);

	tmp = BytesToPixels(dSize);
	if(ceil(imgOfs + datStep*tmp) >= imgPixels) // check for correct data size (out of bounds)
		return 0; // wrong data

	// -- PRINT INFO --
	if(debug)
		printf("# Bits per channel: %u\n# Bits per pixel: %u\n# Alpha used: %s\n# Data step: %.2f\n", datBPC, datBPP, (imgAlphaUse ? "yes" : "no"), datStep);
	printf("Data size: %u bytes\n", dSize);
	printf("Original file name: %s\n", sName.str());
	printf("Output file name: %s\n", sData.str());

	// -- OPEN OUTPUT FILE --
	if(!bOverwrite && file_exist(sData.str()))
	{
		printf("\nFile '%s' is already exist.\nOverwrite? [Y/N] ", sData.str());
		if(getchYN(true) == 'n')
			Die(errDatWrite, "Aborted.");
	}
	if(!(datFile = fopen(sData.str(), "wb+")))
		Die(errDatWrite, "Can't create data file");

	// -- READ DATA & WRITE TO FILE --
	if(dSize < bufmax)
		bufmax = dSize;

	buf.size(bufmax); // read buffer
	datBytesProcessed=0; // calc for debug
	totalbytes=0;

	while(totalbytes < dSize)
	{
		tmp = ReadData(&buf[0], bufmax);
		if(!tmp)
			Die(errDecData, "DATA OVERFLOW");
		if(bCrypt)
			datCrypt->decodeBuf(&buf[0], bufmax);
		fwrite(&buf[0], 1, bufmax, datFile);

        totalbytes += bufmax;
        if(dSize - totalbytes < bufmax)
			bufmax = dSize - totalbytes;
	}
	file_close(&datFile);
	if(debug)
		printf("# Bytes read: %u\n",datBytesProcessed);

	buf.clear();

	return 1;
}

//===============================================
int main(int argc, char ** argv)
//===============================================
{
	Gdiplus::GdiplusStartupInput si;
	UINT         pf;

	srand(time(NULL)); // init random
	rand();

	printf("png-crypt v%s\n", TOOL_VERSION);

	ParseParams(argc, argv);

	if(!sInImg.size())
		Die(errParam, "Input image not defined", 1);

	if(!file_exist(sInImg.str()))
		Die(errInImgLost, "Input image not found: %s", 0, sInImg.str());

	// initialize gdiplus
	GdiplusStartup(&gdiplusToken,&si,0);
	GetEncoderCLSID(Gdiplus::ImageFormatPNG, &clsidPNG); // init PNG CLSID

	if(!LoadImg(&img, sInImg.str()))
		Die(errInImgFormat, "Bad format of input image");

	pf = img->GetPixelFormat();
	imgBPP = Gdiplus::GetPixelFormatSize(pf);
	imgW = img->GetWidth();
	imgH = img->GetHeight();
	imgAlpha = !!(pf & PixelFormatAlpha); // need better decision (gif with transparent not detected)
	imgPixels = imgW*imgH;
	imgChannels = imgAlpha ? 4 : 3; // number of channels (RGBA or RGB)

	printf("\nInput image: %s\nSize: %u x %u, %ubpp", sInImg.str(), imgW, imgH, imgBPP);

	// -- ALPHA SETTINGS --
	printf(" (%salpha", (imgAlpha ? "" : "no "));

	if(!datEncode)
		imgAlphaUse = -1; // disable for decode
	else if(imgAlphaUse  > -1)
	{
		// use or not alpha for data ?
		if(!imgAlphaUse)
		{
			// don't use, but save alpha if it exist
			imgChannels = 3;
			if(imgAlpha)
				printf(", not used");
		}
		else
		{
			if(imgAlphaUse == 2)
			{
				// don't use and delete
				if(imgAlpha)
					printf(", deleted");
				imgAlpha = false;
				imgAlphaUse = 0;
				imgChannels = 3;
			}
			else if(imgAlphaUse == 1)
			{
				// use and add if not exist
				printf(", %s", (imgAlpha ? "used" : "added & used"));
				imgAlpha = true;
				imgChannels = 4;
			}
		}
	}
	else
	{
		// use alpha if it exist only
		imgAlphaUse = (int)imgAlpha;
		if(imgAlpha)
			printf(", used");
	}

	puts(")");

	ConvertImgPF(&img, PixelFormat32bppARGB); // force converting to ARGB 32bpp (for processing)

	if(bCrypt)
		datCrypt = new crypt(sPassword.str()); // init crypt

	if(datEncode)
		EncodeData();
	else
	{
		if(!DecodeData())
			Die(errDecData, "Data not found or protected");
	}

	printf("\nDone.\n");

	Die();
}
