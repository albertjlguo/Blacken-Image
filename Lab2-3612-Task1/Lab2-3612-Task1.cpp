/*
 *  Lab2-3612-Task1
 *  
 *	Heart of the program
 *	written by 
 *	{el17jg}@leeds.ac.uk
 *	Jialeng Guo
 *	Packaged at 2019/2/20 22:06
 *
 *  Created by Dr Chris Trayner on 2010-04-24.
 *	Variant for Microsoft Visual C++ and normal systems (CT 2012nov14)
 *	Further changes for VisStudio 2010 & 2012 (CT 2014mar)
 *
 */

/*	-	-	Microsoft Visual C++ or not?	-	-	*/

#include "stdafx.h"
/* stdafx.h is a Microsoft Visual C include file.
 My non-Microsoft systems have a 'fake' one which merely 
 #declares ELEC3612_5611_NotMicrosoftVisualC. This allows
 the following #if to detect which system (at least of two)
 is in use.
*/ 
#ifdef ELEC3612_5611_NotMicrosoftVisualC
	#define MicrosoftVisualC	0
#else
	#define MicrosoftVisualC	1
	#pragma warning(disable: 4996)
		/* This is needed for VisStudio 2012 but not 2010.
		VS2012 throws out certain calls like strncpy and scanf, saying that they 
		are not safe (not thread-safe?) and deprecated.
		This pragma (found on a Microsoft forum) turns off this checking.
		Heaven only knows exactly what (else) it does.
		CT 2014mar07 */
#endif


/* -	-	-	-	-	-	-	-	-	*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ProgName	"Lab2-3612-Task1"
#define LabNo		2
#define ModuleNo	3612
#define TaskNo		1

#define AllowWeighting			0
#define PrintHeaders			0
#define OutFileNameIncludesWeightings	0
	/*	0: Filenames of the form Lab2-3612-Task1Output.bmp
		1: Filenames of the form mono_R33-G33-B33_BMW.bmp	*/
#define TryImagesLink			1
	/*	If an image can't be found in the directory where the prog is being run,
		try it in any subdirectory pointed to by a symbolic link Images. */

// ---------------------------- Cludge to cope with Microsoft Visual C++ ---------------


// A sort of typedef:
#define DoExit		1
#define ExitAfterPause	2	// Tends to work badly - prefer DontExit
#define	DontExit	3

/* Choose behaviour:
	Probably best left at DoExit for normal systems,
				DontExit for Microsoft */
#if MicrosoftVisualC
	#define ExitBehaviour	DontExit
#else
	#define ExitBehaviour	DoExit
#endif

void my_exit( int exit_no )
{
#if ExitBehaviour == DoExit
	exit( exit_no );
#else
	while (1) {
		getchar();
		#if ExitBehaviour == ExitAfterPause
			exit( exit_no );
		#endif
	}
#endif
} // my_exit


// --------------------------------------- End of cludge -------------------------------

/* Type definitions for BMP headers: */

typedef struct {
	/* magicWord cannot be part of this structure as the compiler 
	 optimisation expands it to a 4-byte field. Grrr...	*/
	int		fileSize;	
	short int	resWord1;
	short int	resWord2;
	int		imageOffset;
} BmpFileHeaderType;

typedef struct {
	int		headerSize;
	int		width;
	int		height;
	short int	nPlanes;
	short int	bitsPerPixel;
	int		compression;
	int		imageSize;
	int		xScale;
	int		yScale;
	int		nColsUsed;
	int		nImportantCols;
} DibHeaderType;



void FileErrorCheck( FILE *inFile )
{
	printf( "===> EOF has%s been met.\n", feof(inFile) ? "" : " not" );
	printf( "===> ferror = %d\n", ferror(inFile) );
} // FileErrorCheck

int main( int argc, char *argv[] )
{
	char				inFileName[1000], outFileName[1000], answer[1000];
						/* Computer memory comes in GBytes nowadays, so why 
						cut it down to the minimum needed? Bugs caused by 
						writing over the end of a string can be very hard to find. */
	FILE				*inFile, *outFile;
	char				magicWord[2];
	unsigned char		colourPalette[256][4];	// Max possible size
	unsigned char		*image;
	BmpFileHeaderType	BmpFileHeader;
	DibHeaderType		DibHeader;
	int			headersRead = 0, nAlreadyMono = 0, weighted = 0, 
				colourPaletteSize = 0, coloursRead, coloursUsed, colour, 
				targetColour = 0, targetLoc = 0, targetFound = 0, 
				colsPadded = 0, imageSizePadded = 0, row, col;
	float			weightR, weightG, weightB, 
				weightRrel = 1.0/3.0, weightGrel = 1.0/3.0, weightBrel = 1.0/3.0;
	
	
	printf( "%s (%s)\n", ProgName, MicrosoftVisualC ? "Microsoft Visual C" : "not Visual C" );
	
// First a check that the compiler has generated suitable code:
	if ((sizeof(int) != 4) || (sizeof(short int) != 2)) {
		printf( "FATAL ERROR: int is %ld bytes, short int %ld bytes\n", 
			   sizeof(int), sizeof(short int) );
		my_exit( -1 );	/* This causes the program to halt immediately.
					 The value of -1 (which is commonly used to flag an error) 
					 will be available to any process which wishes to check it, 
					 e.g. any script which called the program. */
	}
	
// Get the file name and open the file:
	if (argc >= 2) // Name is given as command-line arg
		strncpy( inFileName, argv[1], 1000 );
	else { // Get from keyboard
		printf( "Name of BMP file to read (including extension)?\n" );
		scanf( "%s", inFileName );	// Read the name from keyboard.
	}
	printf( "Input image file %s\n", inFileName );
	
	// Open it for read:
	inFile = fopen( inFileName, "rb" );
	/* The b means binary. Without it, the Microsoft system assumes it to be a text file (!).
	This means that ^Z in the file is taken as EOF, losing most of the file. */
	/* If this succeeds, inFile will be a 'handle' (some sort of internal reference number) which 
	the read commands below can use to access the file. 
	If the fopen fails, inFile will be set to a value NULL. */
#if TryImagesLink
	if (inFile == NULL) {
	char	newName[1000];
		sprintf( newName, "Images/%s", inFileName );
		inFile = fopen( newName, "rb" );	/* No check whether it 
			succeeded - just let the following check find it. */
	} // Try Images
#endif
	if (inFile == NULL) {
		printf( "FATAL ERROR - couldn't open file \"%s\" for input\n", inFileName );
#if TryImagesLink
		puts( "\t(not even in the Images subdirectory)" );
#endif
		my_exit( -1 );
	}
	
// Read the headers:
	/* The fread command returns the number of items it read. In this program, each fread 
	 should read one item. This can be used to check that all was read. */
	headersRead += fread( &magicWord, sizeof(magicWord), 1, inFile );	/* This has to be read 
						separately from the rest of the BMP file header: see the comment 
						in the definition of BmpFileHeaderType above. */
	headersRead += fread( &BmpFileHeader, sizeof(BmpFileHeader), 1, inFile );
	headersRead += fread( &DibHeader, sizeof(DibHeader), 1, inFile );
	if (headersRead != 3) {	// Check it all worked
		printf( "FATAL ERROR: only read %d header items, not 3\n", headersRead );
		my_exit( -1 );
	}
	
// Check this is a valid file:
	if ((magicWord[0] != 'B') || (magicWord[1] != 'M')) {
		printf( "FATAL ERROR: not a BMP file: the first two bytes are hex %02X %02X, not BM\n", 
			   magicWord[0] & 0xFF, magicWord[1] & 0xFF );
		my_exit( -1 );
	}
	if (DibHeader.headerSize != 40) {
		printf( "FATAL ERROR: this is a BMP file, but not with a 40-byte header.\n" );
		printf( "This program only copes with 40-byte headers.\n" );
		my_exit( -1 );
	}
	if (DibHeader.bitsPerPixel != 8) {
		printf( "FATAL ERROR: this is a BMP file, but with %d-bit pixels.\n", DibHeader.bitsPerPixel );
		printf( "This program only copes with 8-bit pixels.\n" );
		my_exit( -1 );
	}
	if (DibHeader.compression != 0) {
		printf( "FATAL ERROR: this is a compressed BMP file.\n" );
		printf( "This program cannot cope with compression.\n" );
		my_exit( -1 );
	}
	
// Read the colour map:
	colourPaletteSize = (BmpFileHeader.imageOffset 
					 - sizeof(magicWord) - sizeof(BmpFileHeader) - sizeof(DibHeader)) 
	/ 4;
	coloursRead = fread( colourPalette, 4, colourPaletteSize, inFile );
	if (coloursRead != colourPaletteSize) {
		printf( "FATAL ERROR: only %d colours read from colour map, not %d\n", 
			   coloursRead, colourPaletteSize );
		my_exit( -1 );
	}
	coloursUsed = DibHeader.nColsUsed;
	if (coloursUsed == 0)
		coloursUsed = 1 << DibHeader.bitsPerPixel;
	if (coloursUsed != colourPaletteSize)
		printf( "WARNING: DIB header claims %d colours, but colour map has space for %d\n", 
			   coloursUsed, colourPaletteSize );
	
// Read the pixels:
	colsPadded = ((DibHeader.width + 3) / 4) * 4;
	imageSizePadded = colsPadded * abs( DibHeader.height );
	image = (unsigned char*)malloc( imageSizePadded );
	if (image == NULL) {
		printf( "FATAL ERROR: malloc() failed to get image array\n" );
		my_exit( -1 );
	}
	if (fread( image, imageSizePadded, 1, inFile ) != 1) {
		printf( "FATAL ERROR: could not read image\n" );
		my_exit( -1 );
	}
	
	fclose( inFile );
	
	
#if PrintHeaders
// Print the BMP file header:
	printf( "\nBMP file header:\n" );
	printf( "\tMagic word:\t\t%c%c\n", magicWord[0], magicWord[1] );
	printf( "\tFile size:\t\t%d\n", BmpFileHeader.fileSize );
	printf( "\tReserved word 1:\t%d\n", BmpFileHeader.resWord1 );
	printf( "\tReserved word 2:\t%d\n", BmpFileHeader.resWord2 );
	printf( "\tOffset to image start:\t%d\n", BmpFileHeader.imageOffset );

// Print the DIB header:
	printf( "\nDIB header:\n" );
	printf( "\tHeader size:\t\t%d\n", DibHeader.headerSize );
	printf( "\tImage width:\t\t%d\n", DibHeader.width );
	printf( "\tImage height:\t\t%d%s\n", DibHeader.height, 
				(DibHeader.height < 0) ? "\tNOTE: upside-down image" : "" );
	printf( "\tNo. of colour planes:\t%d\n", DibHeader.nPlanes );
	printf( "\tBits per pixel:\t\t%d\n", DibHeader.bitsPerPixel );
	if (DibHeader.compression == 0)
		printf( "\tCompression:\t\tnone\n" );
	else	printf( "\tCompression:\t\ttype%d\n", DibHeader.compression );
	printf( "\tImage size:\t\t%d bytes\n", DibHeader.imageSize );
	printf( "\tHoriz image scale:\t%d pixel/metre\n", DibHeader.xScale );
	printf( "\tVert image scale:\t%d pixel/metre\n", DibHeader.yScale );
	printf( "\tNo. of colours used:\t%d", DibHeader.nColsUsed );
	if (DibHeader.nColsUsed == 0)
		printf( ", meaning all 2^%d", DibHeader.bitsPerPixel );
	printf( "\n" );
	if (DibHeader.nImportantCols == 0)
		printf( "\tImportant colours:\tAll are used\n" );
	else	printf( "\tNo. important colours:\t%d\n", DibHeader.nImportantCols );
	
// Calculated data:
	printf( "\nData calculated from the above:\n" );
	printf( "\tColour map size:\t%d colours\n", colourPaletteSize );
	if ( DibHeader.width == colsPadded)
		printf( "\tImage area:\t\t%d columns is integral multiple of 4, so no padding\n", 
			   DibHeader.width );
	else	printf( "\tImage area:\t\t%d columns padded to %d\n", DibHeader.width, colsPadded );
	printf( "\t\t\t\t%d bytes in all\n", imageSizePadded );
	
	printf( "\n" );
#endif
	

	
/* ****************************** Heart of the program ************************** */

	targetLoc = 0;

	// i, j, and the index of image starts from 0;
	// i = [0, colsPadded);
	// j = [0, height);
	// index = width * j + i.
	for (int i = 0; i < colsPadded; i++){
		for (int j = 0; j < DibHeader.height; j++){
			if (j < 100){
				image[abs(colsPadded * j + i)] = targetLoc;
			}
		}
	}

/* ****************************** End of the heart of the program ***************
			The rest writes the modified image to a file
				of the same name with "Output" just before the extension.
 */

#if OutFileNameIncludesWeightings
	sprintf( outFileName, "mono_R%02.0f-G%02.0f-B%02.0f_%s", 
			weightRrel*100.0, weightGrel*100.0, weightBrel*100.0, inFileName );
#else
	sprintf( outFileName, "%sOutput.bmp", ProgName );
#endif
	outFile = fopen( outFileName, "wb" );
	if (outFile == NULL) {
		printf( "FATAL ERROR - couldn't open file \"%s\" for output\n", outFileName );
		my_exit( -1 );
	}
	printf( "\nModified image will be written to %s\n", outFileName );
	headersRead = 0;
	headersRead += fwrite( &magicWord, sizeof(magicWord), 1, outFile );
	headersRead += fwrite( &BmpFileHeader, sizeof(BmpFileHeader), 1, outFile );
	headersRead += fwrite( &DibHeader, sizeof(DibHeader), 1, outFile );
	headersRead += fwrite( colourPalette, 4, colourPaletteSize, outFile );
	headersRead += fwrite( image, imageSizePadded, 1, outFile );
	fclose( outFile );
	if (headersRead != coloursRead + 4) {
		printf( "FATAL ERROR: %d blocks written; should have been %d\n", 
			   headersRead, coloursRead+4 );
		my_exit( -1 );
	}
	
	my_exit( 0 );	/* All C programs should return a value; 0 is normally used to 
				 indicate successful completion of the program.
			Normally return(0) would be used; my_exit has been used for 
				Microsoft compatability. */
	return 0;	// Retained to prevent a compiler warning.
} // main()
