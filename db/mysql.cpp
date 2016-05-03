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
#include "../functions/gstring.h"

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
    
    virtual std::string real_escape_string(const char * str)
    {
        char* from = new char[strlen(&str[0]) * 3 + 1];
        mysql_real_escape_string(&conn, from, &str[0], strlen(str));
        return std::string(from);
    }
    
    virtual int query(const char * query, db_elems *results)
    {
        std::string qstr = str_replace("{pref}", db_prefix, query);

        std::cout << "query: " << qstr << std::endl;
        
        if (mysql_query(&conn, &qstr[0]) != 0) {
            std::cerr << "Query error: " << qstr << std::endl;
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
        
        results->num_rows = mysql_num_rows(res);
        results->num_fields = mysql_num_fields(res);

        MYSQL_FIELD *fields;

        fields = mysql_fetch_fields(res);
        
        db_row * dbrow = new db_row;
        while ((row = mysql_fetch_row(res)) != NULL) {

            // dbrow = new db_row;
            for (int j = 0; j < results->num_fields; j++) {
                // std::cout << fields[j].name << " : " << row[j] << std::endl;
                dbrow->row.insert(std::pair<std::string,std::string>(fields[j].name, row[j]));
            }
            
            results->rows.insert(results->rows.end(), *dbrow);
            dbrow = nullptr;
            
        }

        // for (auto itv = results.rows.begin(); itv != results.rows.end(); ++itv) {
            // for (auto itr = itv->row.begin(); itr != itv->row.end(); ++itr) {
                // std::cout << (*itr).first << ": " << (*itr).second << std::endl;
            // }
        // }

        delete dbrow;
        // dbrow = nullptr;

        // Освобождаем память, занятую результирующей таблицей
        mysql_free_result(res);

        // for (auto itv = results.rows.begin(); itv != results.rows.end(); ++itv) {
            // for (auto itr = itv->row.begin(); itr != itv->row.end(); ++itr) {
                // std::cout << (*itr).first << ": " << (*itr).second << std::endl;
            // }
        // }

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
