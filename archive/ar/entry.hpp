/*
 -----------------------------------------------------------------------------
    This file is part of the Thoronador's common code library.
    Copyright (C) 2016  Thoronador

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

#ifndef LIBTHORO_AR_ENTRY_HPP
#define LIBTHORO_AR_ENTRY_HPP

#include <cstdint>
#include <ctime>
#include <string>
#include <archive_entry.h>
#include "../entry.hpp"

namespace libthoro
{

namespace ar
{

/** \brief class to represent an entry within an Ar archive
 */
class entry: public archive::entry
{
  public:
    /** \brief constructor to create entry from libarchive entry
     *
     * \param statbuf   pointer to libarchive entry, may not be null
     */
    entry(struct archive_entry * ent);
}; //class

} //namespace

} //namespace

#endif // LIBTHORO_ISO9660_ENTRY_HPP
