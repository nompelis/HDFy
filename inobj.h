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

   enum inObjPraseState {
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
   inObjPraseState pstate=OBJ_UNKNOWN;

   struct inObjGrp_s {
      int vs,ve;
      int ts,te;
      int ns,ne;
      int fs,fe;
   };

 private:
   std::string filename;
   FILE *fp=NULL;
   int num_lines=0;
   short num_groups=0;
   struct inObjGrp_s dgroup = { .vs=0, .ve=0,
                                .ts=0, .te=0,
                                .ns=0, .ne=0,
                                .fs=0, .fe=0 };
   std::vector< struct inObjGrp_s > groups;
   struct vec2_s { float u,v; };
   struct vec3_s { float x,y,z; };
   std::vector< vec3_s > vertex;
   std::vector< vec2_s > texel;
   std::vector< vec3_s > normal;
   std::vector< int > iadj;                    // CSR style adjacency
   std::vector< unsigned long int > jadj;      // CSR style adjacency

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

