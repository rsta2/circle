#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned char bitmap[320*240];

int main(int argc, char *argv[])
{
	FILE *inf, *outf;
	char *outfname;
	unsigned char c;
	int x, y, z, i;
	char name[256];
	struct {
	char ident[2];
	long size;
	short res1, res2;
	long offset;
	long sizeheader;
	long width, height;
	short planes;
	short bpp;
	long compression;
	long imgsize;
	long widthpels;
	long heightpels;
	long numcolors;
	long importantcolors;
	} bmpinfo;

	if (argc == 1)
	{
	fprintf(stderr, "bmp2bin: No input file specified.\n");
	exit(-1);
	}

	if ((inf = fopen(argv[1], "rb")) == NULL)
	{
	fprintf(stderr, "bmp2bin: Can't open input file \"%s\".\n", argv[1]);
	exit(-1);
	}

	if (fread(&bmpinfo.ident, sizeof(bmpinfo.ident), 1, inf) != 1)
	{
	fprintf(stderr, "bmp2bin: Can't read bitmap identifier. (Wow that's weird.)\n");
	fclose(inf);
	exit(-1);
	}

	if (fread(&bmpinfo.size, sizeof(bmpinfo) - 2, 1, inf) != 1)
	{
	fprintf(stderr, "bmp2bin: Can't read bitmap info. (Wow that\'s weird.)\n");
	fclose(inf);
	exit(-1);
	}

	if (memcmp(&(bmpinfo.ident), "BM", 2))
	{
	fprintf(stderr, "bmp2bin: File is not a bitmap.\n");
	fclose(inf);
	exit(-1);
	}

	if ((bmpinfo.width != 32) || (bmpinfo.height != 32))
	{
	fprintf(stderr, "bmp2bin: Picture width x height is %ld x %ld\n", bmpinfo.width, bmpinfo.height);
//	fclose(inf);
//	exit(-1);
	}

	if (bmpinfo.bpp != 1)
	{
	fprintf(stderr, "bmp2bin: %d-bit color bitmap.\n", bmpinfo.bpp);
//	fclose(inf);
//	exit(-1);
	}

	if (bmpinfo.numcolors != 0)
	{
		fprintf(stderr, "bmp2bin: # of Colors = %d\n", bmpinfo.numcolors );
	}
	
	if (bmpinfo.compression != 0)
	{
	fprintf(stderr, "bmp2bin: Bitmap is compressed. (That's bad.)\n");
	fclose(inf);
	exit(-1);
	}

	if ((outfname = (char *)malloc(strlen(argv[1]) + 5)) == NULL)
	{
	fprintf(stderr, "bmp2bin: Can\'t allocate memory. (Fer Chrissake, it\'s only %d bytes!)\n", strlen(argv[1] + 5));
	fclose(inf);
	exit(-1);
	}

	outfname = strcat(argv[1], ".c");

	if ((outf = fopen(outfname, "wt")) == NULL)
	{
	fprintf(stderr, "bmp2bin: Can\'t open output file \"%s\".\n", outfname);
	fclose(inf);
	fclose(outf);
	exit(-1);
	}

	strcpy(name, argv[1]);
	for(int i=0; i< strlen(argv[1]);i++)
		if (name[i] == '.')
			name[i] = '_';
	fprintf(outf, "char %s[] = {\n", name);

	fseek(inf, bmpinfo.offset, SEEK_SET);
	fread(&bitmap[y*bmpinfo.width], 1, bmpinfo.height * bmpinfo.width, inf);
	for (y = bmpinfo.height - 1; y >= 0; y--)
	{
		fprintf(outf, "\t");
		for (x = 0; x < bmpinfo.width; x++)
		{
			fprintf(outf, "0x%02x,", bitmap[y*bmpinfo.width+x]);
		}
		fprintf(outf, "\n");
	}
	fprintf(outf, "\t};\n", argv[1]);
}