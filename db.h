#include <map>
#include <boost/any.hpp>

#include "dl.h"

#define MAX_ROWS 32
#define MAX_ROW_CONTENT 4096

struct db_elems {
    int num_rows;
    int num_fields;
    // std::vector<std::string> rows[];
    // std::vector<std::string> rows[32];
    std::map <char *,std::string> rows[MAX_ROWS];
};

class Db
{
    private:
        char db[16]= "mysql";
        DLHANDLE dbHandle;
    public:
        int load(const char *driver_name);
        
        int connect(const char *host, const char *user, const char *passwd, const char *db, unsigned int port);
        typedef int db_connect_sign(const char *host, const char *user, const char *passwd, const char *db, unsigned int port);
        db_connect_sign* db_connect;

        std::string real_escape_string(std::string string);
        typedef std::string db_real_escape_string_sign(std::string string);
        db_real_escape_string_sign* db_real_escape_string;
        
        int query(std::string query, db_elems &results);
        typedef int db_query_sign(std::string query, db_elems &results);
        db_query_sign* db_query;
        
        int close();
        typedef int db_close_sign();
        db_close_sign* db_close;

        typedef int db_test();
        db_test* test_func;

        // typedef boost::any rows_sign;
        // rows_sign* rows;
        
        // typedef char rows_sign[32][4096];
        // rows_sign* db_rows;
};
