/*
 * converttool.c
 *
 * Converts a web content file to a C string or array (with -b).
 */
#include <stdio.h>
#include <string.h>

int main (int nArgC, char **ppArgV)
{
	int bBinary = 0;
	char *pFileName;
	FILE *pFile;
	int nChar;

	char *pArg0 = *ppArgV++;
	nArgC--;

	if (   nArgC >= 1
	    && strcmp (*ppArgV, "-b") == 0)
	{
		bBinary = 1;

		ppArgV++;
		nArgC--;
	}

	if (nArgC != 1)
	{
		fprintf (stderr, "\nUsage: %s [-b] file\n\n", pArg0);

		return 1;
	}

	pFileName = *ppArgV++;
	nArgC--;

	pFile = fopen (pFileName, bBinary ? "rb" : "r");
	if (pFile == NULL)
	{
		fprintf (stderr, "\n%s: File not found: %s\n\n", pArg0, pFileName);

		return 1;
	}

	if (!bBinary)
	{
		printf ("\"");

		while ((nChar = fgetc (pFile)) != EOF)
		{
			switch (nChar)
			{
			case '\"':
				printf ("\\\"");
				break;

			case '\t':
				printf ("\\t");
				break;

			case '\n':
				printf ("\\n\"\n\"");
				break;

			case '\r':
				break;

			default:
				putchar (nChar);
				break;
			}
		}

		printf ("\"");
	}
	else
	{
		unsigned nOffset = 0;

		while ((nChar = fgetc (pFile)) != EOF)
		{
			printf ("0x%02X,%c", nChar, ++nOffset % 16 == 0 ? '\n' : ' ');
		}

		printf ("\n");
	}

	fclose (pFile);

	return 0;
}
