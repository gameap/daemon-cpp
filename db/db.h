#ifndef DB_H
#define DB_H

#include <map>

#define MAX_ROWS 32
#define MAX_ROW_CONTENT 4096

struct db_elems {
    int num_rows;
    int num_fields;
    std::map <std::string,std::string> rows[MAX_ROWS];
};

class Db {
public:
    // Db();
    // Db(std::string dr) : driver("mysql") { };
    
    virtual int connect(const char *host, const char *user, const char *passwd, const char *db, unsigned int port){ return -1; };
    virtual std::string real_escape_string(std::string str){ return ""; };
    virtual int query(std::string query, db_elems &results){ return -1; };
    virtual void close(){ };
};

typedef Db* create_t();
typedef void destroy_t(Db*);

int load_db(Db* &db, char * driver);

#endif
