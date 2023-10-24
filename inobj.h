/******************************************************************************
 Copyright (c) 2023, Ioannis Nompelis
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

#ifndef _INOBJ_H_
#define _INOBJ_H_

#include <stdio.h>
#include <stdlib.h>

enum inObjState {
   Unknown = -1,
   Open = 1,
   Ready = 0
};


#ifdef __cplusplus

#include <list>
#include <vector>
#include <map>
#include <string>


//
// a OBJ file's contents
//

class inObj {
 public:
   inObj();
   virtual ~inObj();

   int getState( void ) const;

   int read( const char filename_[] );

   void clear();

   int dumpTecplot( const char filename[] ) const;

 protected:
   unsigned int istate;

   enum inObjParseState {
      OBJ_UNKNOWN,
      OBJ_OPEN,
      OBJ_COMMENT,
      OBJ_BLANK,
      OBJ_OBJECT,
      OBJ_GROUP,
      OBJ_VERTEX,
      OBJ_NORMAL,
      OBJ_TEXEL,
      OBJ_FACE,
      OBJ_SMOOTH,
      OBJ_MTLLIB,
      OBJ_USEMTL,
      OBJ_READY,
      OBJ_ERROR
   };
   inObjParseState pstate=OBJ_UNKNOWN;

   struct inObjGrp_s {
      std::string name;
      int fs,fe;
   };

   enum inMtlParseState {
      MTLLIB_UNKNOWN,
      MTLLIB_OPEN,
      MTLLIB_NEWMTL,
      MTLLIB_KA,        // ambient
      MTLLIB_KD,        // diffuse
      MTLLIB_KS,        // specular
      MTLLIB_NS,        // specular exponent
      MTLLIB_NI,        // specular index
      MTLLIB_D,         // disolve factor (transparency/opacity)
      MTLLIB_ILLUM,
      MTLLIB_MAPKD,
      MTLLIB_READY,
      MTLLIB_ERROR
   };
   inMtlParseState mstate=MTLLIB_UNKNOWN;

   enum inFileMagic {
      FILEMAGIC_UNKNOWN = 0,
      FILEMAGIC_PNG = 1,
      FILEMAGIC_JPEG = 2,
      FILEMAGIC_TIFF = 3
   };

   struct inImage_s {
      inFileMagic type;
      unsigned int width,height;
      int irgb;
      void* img_data;   // Interpreted differently for diff. types
   };

   struct inObjMtl_s {
      std::string name;
      float Ka[3];
      float Kd[3];
      float Ks[3];
      float Ns;
      float Ni;
      float d;
      unsigned short illum;
      std::string map_Kd;
      struct inImage_s img;
   };

 private:
   std::string filename;
   FILE *fp=NULL;
   int num_lines=0;
   short num_groups=0;
   struct inObjGrp_s dgroup = { .fs=0, .fe=0 };
   std::vector< struct inObjGrp_s > groups;
   std::string mtllib_name;
   std::vector< struct inObjMtl_s > mtls;
   struct vec2_s { float u,v; };
   struct vec3_s { float x,y,z; };
   std::vector< vec3_s > vertex;
   std::vector< vec2_s > texel;
   std::vector< vec3_s > normal;
   std::vector< int > icsr;                    // CSR style segmented polygons
   std::vector< unsigned long int > jcsr;      // CSR style segmentes polygons

   int parse();
   int readLine();
   int handleLine();
   int handleVertex( std::vector< std::string > & strings );
   int handleNormal( std::vector< std::string > & strings );
   int handleTexel( std::vector< std::string > & strings );
   int handleFace( std::vector< std::string > & strings );
   int handleGroup( std::vector< std::string > & strings );
   int handleSmooth( std::vector< std::string > & strings );
   int handleObject( std::vector< std::string > & strings );
   int handleMtllib( std::vector< std::string > & strings );
   int parseMtllib();
   int handleMtlLine( struct inObjMtl_s & mtl_, int & have_one );
   int determineFileType( const char* filepath ) const;
   int unifyTexture( struct inImage_s* s );

   char* buf=NULL,*buf2=NULL;
   size_t nbytes=0;
};

#endif


#ifdef __cplusplus
extern "C" {
#endif

void* readObjFile( const char filename_[] );

int clearObj( void* p );

int dumpTecplot( void* p, const char filename[]  );

#ifdef __cplusplus
}
#endif

#endif

