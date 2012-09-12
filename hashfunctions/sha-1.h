/*
 -----------------------------------------------------------------------------
    This file is part of the Thoronador's common code library.
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

#ifndef LIBTHORO_SHA_1_H
#define LIBTHORO_SHA_1_H

#include <stdint.h>
#include <string>
#include "sha-256_sources.h"

namespace SHA1
{
  //alias for types that are shared with SHA-256
  typedef SHA256::MessageSource MessageSource;
  typedef SHA256::BufferSource  BufferSource;
  typedef SHA256::FileSource    FileSource;
  typedef SHA256::MessageBlock  MessageBlock;

  //the MessageDigest strucure for SHA-1
  struct MessageDigest
  {
    uint32_t hash[5];

    /* default constructor */
    MessageDigest();

    /* returns the message digest's representation as hexadecimal string */
    std::string toHexString() const;

    /* set the message digest according to the given hexadecimal string and
       returns true in case of success, or false if the string does not re-
       present a valid hexadecimal digest

       parameters:
           digestHexString - the string containing the message digest as hex
                             digits (must be all lower case)
    */
    bool fromHexString(const std::string& digestHexString);

    /* returns true, if all hash bits are set to zero */
    bool isNull() const;

    /* sets all bits of the hash to zero */
    void setToNull();

    /* equality operator */
    bool operator==(const MessageDigest& other) const;

    /* inequality operator */
    bool operator!=(const MessageDigest& other) const;

    /* comparison operator */
    bool operator<(const MessageDigest& other) const;
  };

  /* computes and returns the message digest of data in the given buffer of the
     given length

     parameters:
         data                - pointer to the message data buffer
         data_length_in_bits - length of data in bits. Value is rounded up to
                               the next integral multiple of eight, i.e. only
                               full bytes are allowed.
  */
  MessageDigest computeFromBuffer(uint8_t* data, const uint64_t data_length_in_bits);

  /* computes and returns the message digest of the given file's contents

     parameters:
         fileName - name of the file
  */
  MessageDigest computeFromFile(const std::string& fileName);

  MessageDigest computeFromSource(MessageSource& source);
}//SHA1 namespace

#endif // LIBTHORO_SHA_1_H
