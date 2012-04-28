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

#include "zlib.h"

extern void pdftotxt
(
	char *buf_out,
	size_t *len_out,
	char *buf_in,
	long len_in
);
