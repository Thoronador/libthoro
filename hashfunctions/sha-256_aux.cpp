/*
 -----------------------------------------------------------------------------
    This file is part of the Thoronador's random stuff.
    Copyright (C) 2012 thoronador

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 -----------------------------------------------------------------------------
*/

#include "sha-256_aux.h"
#include <cstring>

namespace SHA256
{

#if BYTE_ORDER == LITTLE_ENDIAN
// byte order adjustment functions (Shouldn't there be a std header for that?)

/* reverses little endian to big endian

   parameters:
       w - little endian value
       x - var. to store the converted value
*/
void reverse32(const uint32_t w, uint32_t& x)
{
  uint32_t tmp = w;
  tmp = (tmp >> 16) | (tmp << 16);
  x = ((tmp & 0xff00ff00UL) >> 8) | ((tmp & 0x00ff00ffUL) << 8);
}

/* reverses little endian to big endian

   parameters:
       w - little endian value
       x - var. to store the converted value
*/
void reverse64(const uint64_t w, uint64_t& x)
{
  uint64_t tmp = w;
  tmp = (tmp >> 32) | (tmp << 32);
  tmp = ((tmp & 0xff00ff00ff00ff00ULL) >> 8) |
        ((tmp & 0x00ff00ff00ff00ffULL) << 8);
  x = ((tmp & 0xffff0000ffff0000ULL) >> 16) |
      ((tmp & 0x0000ffff0000ffffULL) << 16);
}
#endif

/* MessageSource functions */
MessageSource::MessageSource()
{
  m_BitsRead = 0;
  m_PaddingBuffer = (uint8_t*) 0;
  m_Status = psUnpadded;
}

MessageSource::~MessageSource()
{
  if (m_PaddingBuffer!=(uint8_t*) 0)
  {
    delete[] m_PaddingBuffer;
    m_PaddingBuffer = 0;
  }
}

/* BufferSource functions */
BufferSource::BufferSource(uint8_t* data, uint64_t data_length_in_bits)
: MessageSource(), m_BufferPointer(data)
{
  //do the padding here
  m_BufferSize = (data_length_in_bits/8) + ((data_length_in_bits%8)>0); //we want full bytes only
  const unsigned int remainingDataBits = (m_BufferSize*8)%512;
  if (remainingDataBits>440)
  {
    m_PaddingBuffer = new uint8_t[1024/8];
    //zero out all bits
    memset(m_PaddingBuffer, 0, 1024/8);
    //copy remainder
    memcpy(m_PaddingBuffer, &m_BufferPointer[m_BufferSize-(m_BufferSize%64)], m_BufferSize%64);
    //add data length in bits
    data_length_in_bits = m_BufferSize * 8;
    #if BYTE_ORDER == LITTLE_ENDIAN
    reverse64(data_length_in_bits, data_length_in_bits);
    #endif
    memcpy(&m_PaddingBuffer[(1024-64)/8], &data_length_in_bits, 8);
    m_Status = psPadded1024;
  }
  else
  {
    m_PaddingBuffer = new uint8_t[512/8];
    //zero out all bits
    memset(m_PaddingBuffer, 0, 512/8);
    //copy remainder
    memcpy(m_PaddingBuffer, &m_BufferPointer[m_BufferSize-(m_BufferSize%64)], m_BufferSize%64);
    //add data length in bits
    data_length_in_bits = m_BufferSize * 8;
    #if BYTE_ORDER == LITTLE_ENDIAN
    reverse64(data_length_in_bits, data_length_in_bits);
    #endif
    memcpy(&m_PaddingBuffer[(512-64)/8], &data_length_in_bits, 8);
    m_Status = psPadded512;
  }
  //add 1-bit
  m_PaddingBuffer[remainingDataBits/8] = 0x80;
  m_BitsRead = 0;
}

BufferSource::~BufferSource()
{
  m_BufferSize = 0;
  m_BufferPointer = 0;
}

bool BufferSource::getNextMessageBlock(MessageBlock& mBlock)
{
  if (m_BitsRead+512<=m_BufferSize)
  {
    memcpy(&(mBlock.words[0]), &m_BufferPointer[m_BitsRead/8], 64);
    m_BitsRead += 512;
    #if BYTE_ORDER == LITTLE_ENDIAN
    unsigned int i;
    for (i=0; i<16; ++i)
    {
      reverse32(mBlock.words[i], mBlock.words[i]);
    }
    #endif
    return true;
  }
  else
  {
    switch (m_Status)
    {
      case psPadded512:
           memcpy(&(mBlock.words[0]), &m_PaddingBuffer[0], 64);
           m_Status = psPaddedAndAllRead;
           break;
      case psPadded1024:
           memcpy(&(mBlock.words[0]), &m_PaddingBuffer[0], 64);
           m_Status = psPadded1024And512Read;
           break;
      case psPadded1024And512Read:
           memcpy(&(mBlock.words[0]), &m_PaddingBuffer[64], 64);
           m_Status = psPaddedAndAllRead;
           break;
      case psPaddedAndAllRead:
           return false;
           break;
      case psUnpadded:
           //should never happen
           throw 42;
           return false;
    }//swi
    m_BitsRead += 512;
    #if BYTE_ORDER == LITTLE_ENDIAN
    unsigned int i;
    for (i=0; i<16; ++i)
    {
      reverse32(mBlock.words[i], mBlock.words[i]);
    }
    #endif
    return true;
  }//else
}

} //namespace
