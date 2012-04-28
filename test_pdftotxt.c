// test_pdftotxt.c file by ziga.zupanec@gmail.com (agiz@github)
// test.pdf: Harry Zhang, The Optimality of Naive Bayes
//           Faculty of Computer Science University of 
//           New Brunswick Fredericton, New Brunswick, Canada
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdftotxt.h"

#define OUTPUT_BUFFER 10

int main()
{
	FILE * fp;
	char *buf_in, *buf_out;
	long len_in;
	size_t res, len_out = 0;

	fp = fopen("test.pdf", "rb");
	if (fp == NULL)
	{
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	len_in = ftell(fp);
	rewind(fp);

	buf_in = (char *)malloc(sizeof(char) * len_in);
	if (buf_in == NULL)
	{
		return -1;
	}

	res = fread(buf_in, 1, len_in, fp);
	if (res != (size_t)len_in)
	{
		return -1;
	}
	
	buf_out = (char *)malloc(sizeof(char) * res * OUTPUT_BUFFER);
	if (buf_out == NULL)
	{
		return -1;
	}

	pdftotxt(buf_out, (size_t *)&len_out, buf_in, res);
	printf("extracted: %lu characters\n\n", len_out);
	printf("output:\n%s\n", buf_out);

	fclose(fp);
	free(buf_in);
	free(buf_out);

	return 0;
}
