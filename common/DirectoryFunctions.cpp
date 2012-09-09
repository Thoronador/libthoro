/*
 -------------------------------------------------------------------------------
    This file is part of the Thoronador's common code library.
    Copyright (C) 2011, 2012  Thoronador

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
 -------------------------------------------------------------------------------
*/

#include "DirectoryFunctions.h"
#include <sys/stat.h>

#if defined(_WIN32)
  #include <Windows.h>
#elif defined(__linux__) || defined(linux)
  #include <unistd.h>
  #include <pwd.h>
#endif

bool directoryExists(const std::string& dirName)
{
  struct stat buffer;
  if (stat(dirName.c_str(), &buffer)==0)
  {
    //stat() was successful
    return ((buffer.st_mode & S_IFMT) == S_IFDIR);
  }//if
  //An error occured, so return false.
  return false;
}

bool createDirectory(const std::string& dirName)
{
  #if defined(_WIN32)
    //WinAPI's CreateDirectory() returns nonzero on success
    return (CreateDirectory(dirName.c_str(), NULL)!=0);
  #elif defined(__linux__) || defined(linux)
    //mkdir() returns zero on success
    return (0==mkdir(dirName.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
  #else
    #error "Unknown operating system!"
  #endif
}

bool createDirectoryRecursive(const std::string& dirName)
{
  const std::string::size_type delimPos = dirName.rfind(Thoro::pathDelimiter);
  if (delimPos==std::string::npos) return createDirectory(dirName);

  if (directoryExists(dirName.substr(0, delimPos)))
  {
    //parent directory already exists, just create the requested dir
    return createDirectory(dirName);
  }
  //recursive creation neccessary, parent directory does not exist
  if (createDirectoryRecursive(dirName.substr(0, delimPos)))
  {
    //creation of parent directory completed, go on with the final directory
    return createDirectory(dirName);
  }
  //creation of parent directory failed
  return false;
}

bool getHomeDirectory(std::string& result)
{
  #if defined(_WIN32)
    #warning Not implemented yet!
    return false;
  #elif defined(__linux__) || defined(linux)
    const long int buf_size = sysconf(_SC_GETPW_R_SIZE_MAX);
    char * buffer = new char[buf_size];
    struct passwd info;
    struct passwd * pwd_ptr = NULL;
    int error = getpwuid_r(getuid(), &info, buffer, buf_size, &pwd_ptr);
    if (error!=0)
    {
      //getpwuid_r() failed
      delete [] buffer;
      return false;
    }
    //success
    result = std::string(info.pw_dir);
    delete [] buffer;
    return true;
  #else
    #error Unknown operating system!
  #endif
}
