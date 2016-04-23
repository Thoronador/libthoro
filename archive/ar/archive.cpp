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

#include "archive.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <archive_entry.h>
#include "../../filesystem/file.hpp"

namespace libthoro
{

namespace ar
{

archive::archive(const std::string& fileName)
: m_archive(nullptr),
  m_entries(std::vector<entry>()),
  m_fileName(fileName)
{
  m_archive = archive_read_new();
  if (nullptr == m_archive)
    throw std::runtime_error("libthoro::ar::archive: Could not allocate archive structure!");
  int ret = archive_read_support_format_ar(m_archive);
  if (ret != ARCHIVE_OK)
  {
    archive_read_free(m_archive);
    m_archive = nullptr;
    throw std::runtime_error("libthoro::ar::archive: Format not supported!");
  }
  ret = archive_read_open_filename(m_archive, fileName.c_str(), 4096);
  if (ret != ARCHIVE_OK)
  {
    archive_read_free(m_archive);
    m_archive = nullptr;
    throw std::runtime_error("libthoro::ar::archive: Failed to open file " + fileName + "!");
  }
  //fill entries
  fillEntries();
}

archive::~archive()
{
  int ret = archive_read_free(m_archive);
  if (ret != ARCHIVE_OK)
    throw std::runtime_error("libthoro::ar::archive: Could not close/free archive!");
  m_archive = nullptr;
}

void archive::fillEntries()
{
  m_entries.clear();
  struct archive_entry * ent;
  unsigned int retryCount = 0;
  bool finished = false;
  while (!finished)
  {
    const int ret = archive_read_next_header(m_archive, &ent);
    switch (ret)
    {
      case ARCHIVE_OK: //all OK
      case ARCHIVE_WARN: //success, but non-critical error occurred
           m_entries.push_back(ent);
           break;
      case ARCHIVE_EOF:
           //reached end of archive
           finished = true;
           break;
      case ARCHIVE_RETRY:
           //operation failed but can be retried, so let's do that
           ++retryCount;
           if (retryCount >= 100)
           {
             archive_read_free(m_archive);
             m_archive = nullptr;
             throw std::runtime_error("libthoro::ar::archive::fillEntries(): "
                     + std::string("Too many retries!"));
           }
           break;
      case ARCHIVE_FATAL:
           //fatal error
           archive_read_free(m_archive);
           m_archive = nullptr;
           throw std::runtime_error("libthoro::ar::archive::fillEntries(): "
                   + std::string("Fatal error while getting archive entries!"));
           break;
      default:
           //unknown error
           archive_read_free(m_archive);
           m_archive = nullptr;
           throw std::runtime_error("libthoro::ar::archive::fillEntries(): "
                   + std::string("Unknown error while getting archive entries!"));
           break;
    } //swi
  } //while
  //reopen file to start at beginning when getting next header
  reopen();
}

void archive::reopen()
{
  //close and re-open archive to get to the first entry again
  archive_read_free(m_archive);
  m_archive = nullptr;
  m_archive = archive_read_new();
  if (nullptr == m_archive)
    throw std::runtime_error("libthoro::ar::archive::reopen(): Could not allocate archive structure!");
  int r2 = archive_read_support_format_ar(m_archive);
  if (r2 != ARCHIVE_OK)
  {
    archive_read_free(m_archive);
    m_archive = nullptr;
    throw std::runtime_error("libthoro::ar::archive::reopen(): Format not supported!");
  }
  r2 = archive_read_open_filename(m_archive, m_fileName.c_str(), 4096);
  if (r2 != ARCHIVE_OK)
  {
    archive_read_free(m_archive);
    m_archive = nullptr;
    throw std::runtime_error("libthoro::ar::archive::reopen(): Failed to re-open file " + m_fileName + "!");
  }
}

std::vector<entry> archive::entries() const
{
  return m_entries;
}

bool archive::contains(const std::string& fileName) const
{
  std::vector<entry>::const_iterator it = m_entries.begin();
  while (it != m_entries.end())
  {
    if (it->name() == fileName)
      return true;
    ++it;
  } // while
  return false;
}

bool archive::extractTo(const std::string& destFileName, const std::string& arFilePath)
{
  //If file does not exist in archive, it cannot be extracted.
  if (!contains(arFilePath))
    return false;

  /* Check whether destination file already exists, we do not want to overwrite
     existing files. */
  if (libthoro::filesystem::file::exists(destFileName))
  {
    std::cerr << "ar::archive::extractTo: error: destination file "
              << destFileName << " already exists!" << std::endl;
    return false;
  }

  struct archive_entry * ent;
  bool beenToEOF = false;
  unsigned int retryCount = 0;
  while (true)
  {
    int ret = archive_read_next_header(m_archive, &ent);
    if ((ret == ARCHIVE_OK) || (ret == ARCHIVE_WARN))
    {
      entry e(ent);
      if (e.name() == arFilePath)
      {
        //found the file, extract data
        //open/create destination file
        std::ofstream destination;
        destination.open(destFileName, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        if (!destination.good() || !destination.is_open())
        {
          std::cerr << "ar::archive::extractTo: error: destination file "
                    << destFileName << " could not be created/opened for writing!"
                    << std::endl;
          return false;
        }

        int64_t totalBytesRead = 0;
        const unsigned int bufferSize = 4096;
        char buffer[bufferSize];
        while (totalBytesRead < e.size())
        {
          int bytesRead = 0;
          if (totalBytesRead + bufferSize <= e.size())
            bytesRead = archive_read_data(m_archive, buffer, bufferSize);
          else
            bytesRead = archive_read_data(m_archive, buffer, e.size() % bufferSize);
          if (bytesRead >= 0)
          {
            //write bytes to file
            destination.write(buffer, bytesRead);
            if (!destination.good())
            {
              std::cerr << "ar::archive::extractTo: error: Could not write data to file "
                        << destFileName << "." << std::endl;
              destination.close();
              filesystem::file::remove(destFileName);
              return false;
            } //if write failed
            totalBytesRead += bytesRead;
          }
          else
          {
            std::cerr << "ar::archive::extractTo: error while reading data from archive!"
                      << std::endl;
            destination.close();
            filesystem::file::remove(destFileName);
            return false;
          } //else (error)
        } //while
        //close destination file
        destination.close();
        return true;
      } //if
    } //if ARCHIVE_OK or ARCHIVE_WARN
    else if (ret == ARCHIVE_EOF)
    {
      if (beenToEOF)
      {
        //Already been here, quit.
        std::cerr << "ar::archive::extractTo: Could not find file " << arFilePath
                  << " in archive!" << std::endl;
        return false;
      }
      //set EOF flag for later detection
      beenToEOF = true;
      //close and re-open archive to get to the first entry again
      reopen();
    } //if ARCHIVE_EOF
    else if (ret == ARCHIVE_RETRY)
    {
      //retry
      ++retryCount;
      if (retryCount >= 100)
      {
        std::cerr << "libthoro::ar::archive::extractTo(): Too many re-tries!" << std::endl;
        return false;
      }
    } //if retry
    else
    {
      //May be ARCHIVE_FATAL or similar
      std::cerr << "libthoro::ar::archive::extractTo(): Fatal or unknown error!" << std::endl;
      return false;
    } //else
  } //while
}

bool archive::isAr(const std::string& fileName)
{
  std::ifstream stream;
  stream.open(fileName, std::ios_base::binary | std::ios_base::in);
  if (!stream.good() || !stream.is_open())
    return false;

  char start[7];
  std::memset(start, '\0', 7);
  stream.read(start, 7);
  if (!stream.good() || stream.gcount() != 7)
    return false;
  stream.close();

  /* magic literal for Ar files is "!<arch>".*/
  return (std::string(start, 7) == "!<arch>");
}

} //namespace

} //namespace
