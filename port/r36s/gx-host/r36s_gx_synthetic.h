// Real GX command stream containing a green quad and a red triangle.
// Adapted from the project's headless gx_gl test cases for R36S hardware.
// Produces two EFB->XFB copies; checks the XFB's GPU pixels.
#pragma once
#include "re4_gx_frame.h"
#include <GLES3/gl3.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <utility>
#include <algorithm>
#include <cstdlib>

namespace r36s_synthetic {

struct Demo {
  std::vector<std::uint32_t> ops;
  std::vector<std::uint8_t> vertices;
  std::vector<std::uint16_t> indices;
  Re4GxChunk chunk{};

  void Op(std::uint32_t op,const std::uint32_t* words,std::uint32_t n){
    ops.push_back(op|(n<<8));
    if(n)ops.insert(ops.end(),words,words+n);
  }
  void Bp(unsigned r,std::uint32_t v){
    std::uint32_t word=(r<<24)|(v&0xffffff);
    Op(RE4_GX_OP_BP,&word,1);
  }
  void Xf(std::uint32_t at,std::vector<std::uint32_t> values){
    values.insert(values.begin(),at);
    Op(RE4_GX_OP_XF,values.data(),static_cast<std::uint32_t>(values.size()));
  }
  void XfF(std::uint32_t at,std::vector<float> values){
    std::vector<std::uint32_t> bits(values.size());
    std::memcpy(bits.data(),values.data(),values.size()*sizeof(float));
    Xf(at,std::move(bits));
  }
  void V(float x,float y,float z,const std::uint8_t color[4]){
    const float v[3]={x,y,z};
    const auto* p=reinterpret_cast<const std::uint8_t*>(v);
    vertices.insert(vertices.end(),p,p+12);
    vertices.insert(vertices.end(),color,color+4);
  }
  void Draw(unsigned count,bool quad){
    Re4GxDraw d{};
    d.primitive=(quad?0x80u:0x90u)<<8|RE4_GX_TRIANGLES;
    d.layout=RE4_GX_V_COLOR0;
    d.stride=16;
    d.vertexOffset=static_cast<std::uint32_t>(vertices.size()-count*16);
    d.vertexCount=count;
    d.indexOffset=static_cast<std::uint32_t>(indices.size());
    if(quad){
      const std::uint16_t t[]={0,1,2,0,2,3};
      indices.insert(indices.end(),t,t+6);
    }else{
      const std::uint16_t t[]={0,1,2};
      indices.insert(indices.end(),t,t+3);
    }
    d.indexCount=quad?6:3;
    d.matrices[0]=0|60u<<8|60u<<16|60u<<24;
    d.matrices[1]=60u|60u<<8|60u<<16|60u<<24;
    d.matrices[2]=60u;
    Op(RE4_GX_OP_DRAW,reinterpret_cast<const std::uint32_t*>(&d),sizeof(d)/4);
  }
  void Copy(const std::uint8_t color[4],bool clear){
    Bp(0x4F,color[0]|color[3]<<8);
    Bp(0x50,color[2]|color[1]<<8);
    Bp(0x51,0xFFFFFF);
    Bp(0x4B,(0x80460000u>>5)&0x1FFFFF);
    Bp(0x52,(clear?1u<<11:0)|1u<<14);
  }
  Demo(){
    const std::uint8_t blue[4]={0,0,255,255};
    const std::uint8_t green[4]={0,255,0,255};
    const std::uint8_t red[4]={255,0,0,255};
    std::vector<std::uint32_t> state(256,0);
    state[0x00]=1u<<4; // one color channel
    state[0xC0]=10u|15u<<4|15u<<8|15u<<12|1u<<19;
    state[0xC1]=5u<<4|7u<<7|7u<<10|7u<<13|1u<<19;
    state[0x28]=0x3F7u<<12;
    for(int i=0;i<4;i++){
      state[0xF6+i*2]=0|1<<2;
      state[0xF7+i*2]=2|3<<2;
    }
    state[0xF3]=7u<<16|7u<<19;
    state[0x40]=1|3<<1|1<<4;
    state[0x41]=1<<3|1<<4;
    state[0x43]=1;
    state[0x20]=342|342u<<12;
    state[0x21]=(342+527)|(342u+639)<<12;
    state[0x59]=171|171u<<10;
    for(int i=0;i<8;i++){
      state[0x30+i*2]=63;
      state[0x31+i*2]=63;
    }
    state[0x49]=0;
    state[0x4A]=639|527u<<10;
    Op(RE4_GX_OP_BP_STATE,state.data(),static_cast<std::uint32_t>(state.size()));
    XfF(0,{1,0,0,0,0,1,0,0,0,0,1,0});
    XfF(60*4,{1,0,0,0,0,1,0,0,0,0,1,0});
    XfF(0x400,{1,0,0,0,1,0,0,0,1});
    XfF(0x500+61*4,{1,0,0,0,0,1,0,0,0,0,1,0});
    Xf(0x1009,{1});
    Xf(0x100E,{1});
    Xf(0x1010,{1});
    Xf(0x103F,{0});
    XfF(0x101A,{320.f,-264.f,16777215.f,662.f,606.f,16777215.f});
    XfF(0x1020,{1.f/320.f,-1.f,-2.f/528.f,1.f,-1.f,-1.f});
    Xf(0x1026,{1});
    Copy(blue,true);
    V(10,20,0,green); V(110,20,0,green);
    V(110,70,0,green); V(10,70,0,green);
    Draw(4,true);
    V(500,400,0,red); V(600,400,0,red); V(500,500,0,red);
    Draw(3,false);
    Copy(blue,false);
    chunk.version=RE4_GX_FRAME_VERSION;
    chunk.sequence=1;
    chunk.flags=RE4_GX_CHUNK_RESET|RE4_GX_CHUNK_XFB;
    chunk.ops=ops.data();
    chunk.opWords=static_cast<std::uint32_t>(ops.size());
    chunk.vertices=vertices.data();
    chunk.vertexBytes=static_cast<std::uint32_t>(vertices.size());
    chunk.indices=indices.data();
    chunk.indexCount=static_cast<std::uint32_t>(indices.size());
    chunk.xfbWidth=640;
    chunk.xfbHeight=528;
  }
};

inline bool Pixels(GLuint xfb,unsigned width,unsigned height){
  if(!xfb || width!=640 || height!=528){
    std::printf("SYNTHETIC_XFB_INVALID=tex:%u size:%ux%u\n",xfb,width,height);
    return false;
  }
  GLint prior=0;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&prior);
  GLuint fbo=0;
  glGenFramebuffers(1,&fbo);
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,xfb,0);
  bool ok=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
  struct Sample {int x,y;int r,g,b;const char* label;} tests[]={
      {50,40,0,255,0,"GREEN_QUAD"},{300,300,0,0,255,"BLUE_CLEAR"},
      {520,420,255,0,0,"RED_TRIANGLE"}};
  for(const auto &s:tests){
    std::uint8_t rgba[4]{};
    glReadPixels(s.x,s.y,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    bool good=std::abs((int)rgba[0]-s.r)<=18 &&
              std::abs((int)rgba[1]-s.g)<=18 &&
              std::abs((int)rgba[2]-s.b)<=18;
    std::printf("GX_PIXEL_%s=%u,%u,%u,%u STATUS=%s\n",s.label,rgba[0],rgba[1],rgba[2],rgba[3],
       good?"OK":"FAILED");
    ok=ok&&good;
  }
  GLenum err=glGetError();
  std::printf("GX_PIXEL_GL_ERROR=0x%x\n",err);
  if(err!=GL_NO_ERROR)ok=false;
  glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)prior);
  glDeleteFramebuffers(1,&fbo);
  return ok;
}

} // namespace r36s_synthetic
