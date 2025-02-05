/**
 * Irfan G. (c) 2025.
 */

#pragma once

#include <string>
#include <fstream>

namespace kdb {

using std::string;
using std::ofstream;

 class db {

   private:
     struct _db_header{
       char dbid[4];
       char magic[4];
       char version[4];
       char oname[100];
       char kek[512];
       int64_t idate;
     };

     const string db_path;
     bool  active;
     ofstream odbf;

     bool init() {
       odbf.open(db_path);
       if(odbf.is_open())
         return true;
       return false;
     }

   public:
     db(const db&) = delete;
     db& operator=(const db&) = delete;
     db(db&&) = delete;
     db& operator=(db&&) = delete;

     db(string path = "file.db") : db_path(path), active(false), odbf() {
       active = init();
     }

     bool is_open(){ return active; }

     // todo: read() / write() / delete() / find()

 };

}

