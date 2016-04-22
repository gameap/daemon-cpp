#include <sstream>

#include <iostream>

#ifdef WIN32
#include <winsock2.h>
#endif

#include <mysql/mysql.h>

#include <cstring>
#include <string>
#include <map>
#include <vector>

// #include <boost/any.hpp>
#include "db.h"

MYSQL conn, mysql;
MYSQL_RES *res;
MYSQL_ROW row;

int query_state;

// extern "C" DbDriver dbdriver;

class MySQL : public Db {
public:
    virtual int connect(const char *host, const char *user, const char *passwd, const char *db, unsigned int port)
    {
        mysql_real_connect(&conn, host, user, passwd, db, port, NULL, 0);
    
        if(mysql_errno(&conn)) {
            fprintf(stdout, "Error: %s\n", mysql_error(&conn));
            return -1;
        }

        return 0;
    }
    
    virtual std::string real_escape_string(std::string str)
    {
        char* from = new char[strlen(str.c_str()) * 3 + 1];
        mysql_real_escape_string(&conn, from, &str[0], str.size());
        return std::string(from);
    }
    
    virtual int query(std::string query, db_elems &results)
    {
        if (mysql_query(&conn, &query[0]) != 0) {
            return -1;
        }

        // Получаем дескриптор результирующей таблицы
        res = mysql_store_result(&conn);
        if(res == NULL) {
            // puterror("Error: can't get the result description\n");
        }
        
        // Получаем первую строку из результирующей таблицы
        // row = mysql_fetch_row(res);
        // if(mysql_errno(&conn) > 0) puterror("Error: can't fetch result\n");
        
        // Выводим результат в стандартный поток
        // fprintf(stdout, "Version1: %s\n", row[0]);
        // const char *rw = row[0];
        // rows[0] = row[0];
        
        results.num_rows = mysql_num_rows(res);
        results.num_fields = mysql_num_fields(res);

        int i = 0;
        MYSQL_FIELD *fields;
        
        fields = mysql_fetch_fields(res);
        while ((row = mysql_fetch_row(res)) != NULL) {
            
            for (int j = 0; j < results.num_fields; j++) {
                // results.rows.insert(results.rows.end(), row[i]);
                // results.rows[0].insert (row[j]);
                
                results.rows[i][fields[j].name] = row[j];
                // std::cout << fields[j].name << " : " << results.rows[i][fields[j].name] << std::endl;
            }
            
            i++;
        }

        // for (int i = 0; i < results.count; i++) {
            // results.rows.insert(results.rows.end(), row[i]);
        // }
        
        // fprintf(stdout, "Version2: %s\n", results);

        // Освобождаем память, занятую результирующей таблицей
        mysql_free_result(res);

        return 0;
    }
    
    virtual void close()
    {
        mysql_close(&conn);
    }
};

// the class factories
extern "C" Db* create() {
    return new MySQL;
}

extern "C" void destroy(Db* p) {
    delete p;
}

int main(int argc, char* argv[])
{
    return 0;
}
