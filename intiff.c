/******************************************************************************
 Copyright (c) 2014-2023, Ioannis Nompelis
 All rights reserved.

 Redistribution and use in source and binary forms, with or without any
 modification, are permitted provided that the following conditions are met:
 1. Redistribution of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistribution in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. All advertising materials mentioning features or use of this software
    must display the following acknowledgement:
    "This product includes software developed by Ioannis Nompelis."
 4. Neither the name of Ioannis Nompelis and his partners/affiliates nor the
    names of other contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.
 5. Redistribution or use of source code and binary forms for profit must
    have written permission of the copyright holder.
 
 THIS SOFTWARE IS PROVIDED BY IOANNIS NOMPELIS ''AS IS'' AND ANY
 EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL IOANNIS NOMPELIS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>    // thanks ChatGPT

#include "intiff.h"

int intif_ReadImage( char *filename,
                     unsigned int *iwidth, unsigned int *iheight,
                     unsigned int **data)
{
#ifdef _DEBUG_
   char *FUNC = "intif_ReadImage";
#endif
   TIFF *tif;
   size_t npixels;
   uint32_t width,height;
   tdata_t raster;

   tif = TIFFOpen( filename, "r" );
   if( tif == NULL ) {
      fprintf( stdout," [Error]  Could not open file \"%s\" \n",filename );
      return 1;
   } else {
#ifdef _DEBUG_
      fprintf( stdout," [%s]  Reading file: \"%s\" \n",FUNC, filename );
#endif
   }


   TIFFGetField( tif, TIFFTAG_IMAGEWIDTH, &width );
   TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &height );
#ifdef _DEBUG_
   fprintf( stdout,"    Width = %d    Height = %d \n",(int) width,(int) height);
#endif
   npixels = (size_t) (width*height);

   raster = (uint32_t *) _TIFFmalloc( npixels * sizeof(tdata_t) );
   if( raster == NULL ) {
      fprintf( stdout," [Error]  Could not allocate memory for raster data\n");
      TIFFClose( tif );
      return 2;
   }

   if( TIFFReadRGBAImage( tif, width, height, raster, 0 ) == 1 ) {
#ifdef _DEBUG_
      fprintf( stdout," [%s]  Read successfully \n", FUNC );
#endif
   } else {
      fprintf( stdout," [Error]  Could not read raster data \n" );
      _TIFFfree( raster );
      TIFFClose( tif );
      return 3;
   }

//
// TIFFRGBAImage img;
// char emsg[1024];
// if( TIFFRGBAImageBegin( &img, tif, 0, emsg ) ) {
//    npixels = (size_t) (img.width*img.height);
// }
// TIFFRGBAImageEnd( &img );
//

   TIFFClose( tif );

   *iwidth = (unsigned int) width;
   *iheight = (unsigned int) height;
// _TIFFfree(raster);
   *data = (unsigned int *) raster;

// What you have now is an array (raster) of pixel values, one for each
// pixel in the image, and each pixel is a 4-byte value. Each of the bytes
// represent a channel of the pixel value (RGBA). Each channel is represented
// by an integer value from 0 to 255.
// To get each of the individual channel of a pixel use the function:
// char X=(char )TIFFGetX(raster[i]);  where X can be the channels R, G, B,
// and A. i is the index of the pixel in the raster.

   return 0;
}


int intif_PeekImage( char *filename,
                     unsigned int *iwidth, unsigned int *iheight )
{
#ifdef _DEBUG_
   char *FUNC = "intif_PeekImage";
#endif
   TIFF *tif;
   int npixels;
   uint32_t width,height;

   tif = TIFFOpen( filename, "r" );
   if(tif == NULL) {
      fprintf (stdout," [Error]  Could not open file \"%s\" \n", filename );
      return(1);
   } else {
#ifdef _DEBUG_
      fprintf( stdout," [%s}  Peeking file: \"%s\" \n", FUNC, filename );
#endif
   }

   TIFFGetField( tif, TIFFTAG_IMAGEWIDTH, &width );
   TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &height );
   npixels = (int) (width*height);
#ifdef _DEBUG_
   fprintf( stdout,"    Width = %d    Height = %d     (%d pixels) \n",
          (int) width,(int) height,npixels);
#endif

   TIFFClose( tif );

   *iwidth = (unsigned int) width;
   *iheight = (unsigned int) height;

   return 0;
}



/*
int main() {

   char *filename = "TIF.tif";
   unsigned int width,height, *udata;
   int n,i,j;
FILE *fp=fopen("crap","w");


   (void) intif_ReadImage(filename, &width, &height, &udata);

 fprintf(fp,"variables = x y R G B A \n");
 fprintf(fp,"zone T=\"1\", i=%d, j=%d, f=point\n",width,height);
   for(j=0;j<height;++j)
   for(i=0;i<width;++i) {
      n = i*width + j;
 fprintf(fp," %d %d   %d %d %d  %d\n",j,i,
      TIFFGetR( udata[n] ),
      TIFFGetG( udata[n] ),
      TIFFGetB( udata[n] ),
      TIFFGetA( udata[n] ) );
   }
fclose(fp);

   return(0);
}

*/
