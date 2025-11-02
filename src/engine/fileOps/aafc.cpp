/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2025 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "fileOpsCommon.h"
#include <cstdio>
#include <cstring>

#define AAFC_IMPLEMENTATION
#include "../../extern/aafc/aafc.h" // >:

class DivEngine;

// Architect's Audio Format Container
// (a niche, easy to use audio format)

void DivEngine::loadAAFC(FILE* f, DivSample* s){
  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  unsigned char* data = new unsigned char[fsize];
  if(fread(data, 1, fsize, f) == 0){
    logE("really?");
    delete[] data;
    return;
  }

  AAFCDECOUTPUT dec = aafc_import(data);
  if(dec.header.signature != AAFC_SIGNATURE){
    logE("that is not valid aafc data!");
    delete[] data;
    return;
  }

  delete[] data;

  logI("aafc data loaded");

  s->rate = dec.header.freq;
  s->centerRate = dec.header.freq;

  s->depth = DIV_SAMPLE_DEPTH_16BIT;

  // force multi-channel clips to mono due a quirk
  if(dec.header.channels > 1){
    const unsigned int splen = dec.header.samplelength / dec.header.channels;
    unsigned char chn = 0;
    float accu = 0.0f;
    const float scale = 1.0f / dec.header.channels;
    for (unsigned int i = 0; i < splen; i++) {
      for (chn = 0, accu = 0; chn < dec.header.channels; chn++)
        accu += dec.data[i * dec.header.channels + chn];
      dec.data[i] = accu * scale;
    }
    dec.header.samplelength = splen;
  }

  s->init(dec.header.samplelength);
  for(unsigned int i = 0; i < dec.header.samplelength; i++){
    s->data16[i] = CLAMP(dec.data[i] * 32767.0f, -32768.0f, 32767.0f);
  }

  s->loop = dec.header.loopst != 0 || dec.header.loopend != 0;
  s->loopStart = dec.header.loopst;
  s->loopEnd = dec.header.loopend;
  delete[] dec.data;
}