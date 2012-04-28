// This file contains extremely crude C source code to extract plain text
// from a PDF file. It is only intended to show some of the basics involved
// in the process and by no means good enough for commercial use.
// But it can be easily modified to suit your purpose. Code is by no means
// warranted to be bug free or suitable for any purpose.
// 
// Adobe has a web site that converts PDF files to text for free,
// so why would you need something like this? Several reasons:
// 
// 1) This code is entirely free including for commericcial use. It only
//    requires ZLIB (from www.zlib.org) which is entirely free as well.
// 
// 2) This code tries to put tabs into appropriate places in the text,
//    which means that if your PDF file contains mostly one large table,
//    you can easily take the output of this program and directly read it
//    into Excel! Otherwise if you select and copy the text and paste it into
//    Excel there is no way to extract the various columns again.
// 
// This code assumes that the PDF file has text objects compressed
// using FlateDecode (which seems to be standard).
// 
// This code is free. Use it for any purpose.
// The author assumes no liability whatsoever for the use of this code.
// Use it at your own risk!
//
// PDF file strings (based on PDFReference15_v5.pdf from www.adobve.com:
// 
// BT = Beginning of a text object, ET = end of a text object
// 5 Ts = superscript
// -5 Ts = subscript
// Td move to start next line
//
// No precompiled headers, but uncomment if need be:
// Your project must also include zdll.lib (ZLIB) as a dependency.
// ZLIB can be freely downloaded from the internet, www.zlib.org
// Use 4 byte struct alignment in your project!
//
// modified to work with POSIX(?) C by
// ziga.zupanec@gmail.com (agiz@github)
// requirements: zlib-devel
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
//#include <windows.h>
//#include "zlib.h"

#include "pdftotxt.h"

// TODO: Assume output will fit into 10 times input buffer:
#define OUTPUT_BUFFER 10

// Find a string in a buffer:
size_t findstringinbuffer
(
	char* buffer,
	char* search,
	size_t buffersize
)
{
	size_t i;
	char* buffer0 = buffer;
	size_t len = strlen(search);
	int fnd = 0;

	while (fnd != 1)
	{
		fnd = 1;
		for (i=0; i<len; i++)
		{
			if (buffer[i] != search[i])
			{
				fnd = 0;
				break;
			}
		}
		if (fnd == 1)
		{
			return buffer - buffer0;
		}
		buffer = buffer + 1;
		if (buffer - buffer0 + len >= buffersize)
		{
			return -1;
		}
	}
	return -1;
}

// Keep this many previous recent characters for back reference:
#define oldchar 15

// Convert a recent set of characters into a number if there is one. Otherwise return -1:
float extract_number
(
	const char* search,
	int lastcharoffset
)
{
	int i = lastcharoffset;
	while (i>0 && search[i] == ' ')
	{
		i--;
	}
	while (i>0 && (isdigit(search[i]) || search[i] == '.'))
	{
		i--;
	}

	float flt = -1.0;
	char buffer[oldchar + 5];
	bzero(buffer,sizeof(buffer));
	strncpy(buffer, search+i+1, lastcharoffset-i);
	if (buffer[0] && sscanf(buffer, "%f", &flt))
	{
		return flt;
	}
	return -1.0;
}

// Check if a certain 2 character token just came along (e.g. BT):
int seen2
(
	const char* search,
	char* recent
)
{
	if (
		recent[oldchar-3] == search[0] 
		&& recent[oldchar-2] == search[1] 
		&& (recent[oldchar-1] == ' ' || recent[oldchar-1] == 0x0d || recent[oldchar-1] == 0x0a) 
		&& (recent[oldchar-4] == ' ' || recent[oldchar-4] == 0x0d || recent[oldchar-4] == 0x0a)
	)
	{
		return 0;
	}
	return -1;
}

// This method processes an uncompressed Adobe (text) object and extracts text.
void doconvert
(
	char *buffer,
	size_t *len_out,
	char *output,
	size_t len
)
{
	// Are we currently inside a text object?
	int intextobject = 0;

	// Is the next character literal (e.g. \\ to get a \ character or \( to get ( ):
	int nextliteral = 0;
	
	// () Bracket nesting level. Text appears inside ()
	int rbdepth = 0;

	// Keep previous chars to get extract numbers etc.:
	char oc[oldchar];
	size_t i;
	size_t b;
	int j = 0;
	
	for (j=0; j<oldchar; j++)
	{
		oc[j] = ' ';
	}

	for (b=0, i=0; i<len; i++)
	{
		char c = output[i];

		if (intextobject == 1)
		{
			if (rbdepth == 0 && (seen2("TD", oc) == 0))
			{
				// Positioning. See if a new line has to start or just a tab:
				float num = extract_number(oc, oldchar-5);
				if (num > 1.0)
				{
					buffer[b] = 0x0d;
					b++;
					buffer[b] = 0x0a;
					b++;
				}
				if (num<1.0)
				{
					buffer[b] = '\t';
					b++;
				}
			}
			if (rbdepth == 0 && (seen2("ET", oc) == 0))
			{
				// End of a text object, also go to a new line.
				intextobject = 0;
				buffer[b] = 0x0d;
				b++;
				buffer[b] = 0x0a;
				b++;
			}
			else if (c == '(' && rbdepth == 0 && (nextliteral != 1)) 
			{
				// Start outputting text!
				rbdepth=1;
				// See if a space or tab (>1000) is called for by looking at the number in front of (
				int num = extract_number(oc, oldchar - 1);
				if (num > 0)
				{
					if (num > 1000.0)
					{
						buffer[b] = '\t';
						b++;
					}
					else if (num > 100.0)
					{
						buffer[b] = ' ';
						b++;
					}
				}
			}
			else if (c == ')' && rbdepth == 1 && (nextliteral != 1))
			{
				// Stop outputting text.
				rbdepth = 0;
			}
			else if (rbdepth == 1) 
			{
				// Just a normal text character:
				if (c == '\\' && (nextliteral != 1))
				{
					// Only print out next character no matter what. Do not interpret.
					nextliteral = 1;
				}
				else
				{
					nextliteral = 0;
					if (((c >= ' ') && (c <= '~')) || ((c >= 128) && (c < 255)))
					{
						buffer[b] = c;
						b++;
					}
				}
			}
		}

		for (j=0; j<oldchar-1; j++)
		{
			oc[j] = oc[j+1];
		}
		oc[oldchar-1] = c;

		if (intextobject != 1)
		{
			if (seen2("BT", oc) == 0)
			{
				// Start of a text object:
				intextobject = 1;
			}
		}
	}

	*len_out = b;
}

// Extern function:
void pdftotxt
(
	char *buf_out,		// prepared buffer to fill with text
	size_t *len_out,	// pointer that is to be returned size of read text
	char *buf_in,		// input buffer of _prepared_ pdf data
	long len_in			// size of input buffer
)
{
	int morestreams = 1;
	size_t totout = 0;
	size_t outsize = 0;
	size_t prevoutsize = 0;
	char *reout;
	char *out = NULL;

	// Now search the buffer repeated for streams of data:
	while (morestreams == 1)
	{
		// Search for stream, endstream. We ought to first check the filter of the object to make sure it if FlateDecode, but skip that for now!
		size_t streamstart = findstringinbuffer(buf_in, "stream", len_in);
		size_t streamend   = findstringinbuffer(buf_in, "endstream", len_in);

		if (streamstart > 0 && streamend > streamstart)
		{
			// Skip to beginning and end of the data stream:
			streamstart += 6;

			if (buf_in[streamstart] == 0x0d && buf_in[streamstart + 1] == 0x0a)
			{
				streamstart += 2;
			}
			else if (buf_in[streamstart] == 0x0a)
			{
				streamstart++;
			}

			if (buf_in[streamend - 2] == 0x0d && buf_in[streamend-1] == 0x0a)
			{
				streamend -= 2;
			}
			else if (buf_in[streamend - 1] == 0x0a)
			{
				streamend--;
			}

			outsize = (streamend - streamstart) * OUTPUT_BUFFER;

			char* output;
			output = (char *)malloc(outsize * sizeof(char));
			bzero(output, outsize);

			// Now use zlib to inflate:
			z_stream zstrm;
			bzero(&zstrm, sizeof(zstrm));

			zstrm.avail_in = streamend - streamstart + 1;
			zstrm.avail_out = outsize;
			zstrm.next_in = (Bytef*)(buf_in + streamstart);
			zstrm.next_out = (Bytef*)output;

			int rsti = inflateInit(&zstrm);
			if (rsti == Z_OK)
			{
				int rst2 = inflate(&zstrm, Z_FINISH);
				if (rst2 >= 0)
				{
					// Ok, got something, extract the text:
					prevoutsize = totout;
					totout = totout + zstrm.total_out;

					reout = (char *)realloc(out, totout * sizeof(char));
					if (reout != NULL)
					{
						out = reout;
					}
					else
					{
						free(out);
						exit(1);
					}
					memcpy(out+prevoutsize, output, zstrm.total_out);
				}
			}
			free(output);
			output = 0;
			buf_in += streamend + 7;
			len_in -= streamend + 7;
		}
		else
		{
			morestreams = 0;
		}
	}
	
	// Convert buffer to ascii text:
	doconvert(buf_out, len_out, out, totout);
	free(out);
}
