#pragma once
#include "stdafx.h"

typedef struct {
  u32 magic;
  u32 debugFlag;
  u32 infoOffset;
  u32 unknown1;
  u32 headSize;
  u32 itemCount;
  u64 packageSize;
  u64 dataOffset;
  u64 dataSize;
} pkg_header2;

u64 get_u64(void* vd) {
  u8 *d = (u8*)vd;
  return ((u64)d[0]<<56) | ((u64)d[1]<<48) | ((u64)d[2]<<40) | ((u64)d[3]<<32) | (d[4]<<24) | (d[5]<<16) | (d[6]<<8) | d[7];
}

void set_u64(void* vd, u64 v) {
  u8 *d = (u8*)vd;
  d[0] = v>>56;
  d[1] = v>>48;
  d[2] = v>>40;
  d[3] = v>>32;
  d[4] = v>>24;
  d[5] = v>>16;
  d[6] = v>>8;
  d[7] = v>>0;
}

void set_u32(void* vd, u32 v) {
  u8 *d = (u8*)vd;
  d[0] = v>>24;
  d[1] = v>>16;
  d[2] = v>>8;
  d[3] = v>>0;
}

void set_u16(void* vd, u16 v) {
  u8 *d = (u8*)vd;
  d[0] = v>>8;
  d[1] = v>>0;
}

u32 get_u32(void* vd) {
  u8 *d = (u8*)vd;
  return (d[0]<<24) | (d[1]<<16) | (d[2]<<8) | d[3];
}

float get_float(u8* d) {
  float ret;
  u32 inter = (d[0]<<24) | (d[1]<<16) | (d[2]<<8) | d[3];
  memcpy(&ret, &inter, 4);
  return ret;
}

u32 get_u16(void* vd) {
  u8 *d = (u8*)vd;
  return (d[0]<<8) | d[1];
}

void hexdump(u8* d, int l) {
  int i;
  for(i=0;i<l;i++) {
    if(i!=0 && (i%16)==0) ConLog.Write("\n");
    ConLog.Write("%2.2X ", d[i]);
  }
  ConLog.Write("\n");
}


void hexdump_nl(u8* d, int l) {
  int i;
  for(i=0;i<l;i++) {
    ConLog.Write("%2.2X ", d[i]);
  }
  ConLog.Write("\n");
}

void hexdump_ns(u8* d, int l) {
  int i;
  for(i=0;i<l;i++) {
    ConLog.Write("%2.2X", d[i]);
  }
}

void hexdump_c(u8* d, int l) {
  int i;
  for(i=0;i<l;i++) {
    ConLog.Write("0x%2.2X", d[i]);
    if(i!=(l-1)) {
      ConLog.Write(",");
    }
  }
}

void hexdump_32(u8* d, int l) {
  int i;
  for(i=0;i<l;i+=4) {
    if(i!=0 && (i%16)==0) printf("\n");
    ConLog.Write("%8X ", get_u32(d+i));
  }
  ConLog.Write("\n");
}
