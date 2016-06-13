#include <sstream>
#include <iostream>

#ifdef __GNUC__
#include <unistd.h>
#endif

#include <mysql/mysql.h>

#ifdef _WIN32
// #include <Windows.h>
#endif

#include <cstring>
#include <string>
#include <map>
#include <vector>

#include <time.h>

#include "db.h"
#include "../functions/gstring.h"
#include "../dl.h"

#pragma comment(lib, "advapi32")

MYSQL *conn, mysql;
MYSQL_RES *res;
MYSQL_ROW row;

int query_state;

// extern "C" DbDriver dbdriver;

class MySQL : public Db {
private:
    bool blocked = false;
    
    void block()
    {
        // Wait for unblock
        while(blocked) {
			#ifdef __linux__
				usleep(100000);
			#elif _WIN32
				clock_t goal = 100 + clock();
				while (goal > clock());
			#endif
        }

        blocked = true;
    }

    void unblock()
    {
        blocked = false;
    }
    
public:
    virtual int connect(const char *host, const char *user, const char *passwd, const char *db, unsigned int port)
    {
        my_bool reconnect = 1;
        mysql_options(&mysql, MYSQL_OPT_RECONNECT, &reconnect);

        conn = mysql_real_connect(&mysql, host, user, passwd, db, port, NULL, 0);

        if(mysql_errno(&mysql)) {
            fprintf(stdout, "Error: %s\n", mysql_error(&mysql));
            return -1;
        }

        return 0;
    }

    virtual std::string real_escape_string(const char * str)
    {
        char* from = new char[strlen(&str[0]) * 3 + 1];
        mysql_real_escape_string(&mysql, from, &str[0], strlen(str));
        return std::string(from);
    }

    virtual int query(const char * query)
    {
        std::string qstr = str_replace("{pref}", db_prefix, query);
        block();
        
        // std::cout << "Time Query: " << time(0) << std::endl;
        // std::cout << "Query: " << qstr << std::endl;

        if (conn == 0) {
            fprintf(stderr, "MySQL: %s (%i)", mysql_error(&mysql), mysql_errno(&mysql));
            return -1;
        }
        
        if (mysql_query(&mysql, &qstr[0]) != 0) {
            std::cerr << "Query error (" << mysql_error(&mysql) << "): " << qstr << std::endl;
            unblock();
            return -1;
        }
        
        unblock();
        return 0;
    }

    virtual int query(const char * query, db_elems *results)
    {
        std::string qstr = str_replace("{pref}", db_prefix, query);
        block();

        // std::cout << "Time Query: " << time(0) << std::endl;
        // std::cout << "Query: " << qstr << std::endl;

        if (conn == 0) {
            fprintf(stderr, "MySQL: %s (%i)", mysql_error(&mysql), mysql_errno(&mysql));
            return -1;
        }

        if (mysql_ping(&mysql) != 0) {
            std::cerr << "MySQL server has gone away" << std::endl;
        }

        if (mysql_query(&mysql, &qstr[0]) != 0) {
            unblock();
            // std::cerr << "Query error: " << qstr << std::endl;
            std::cerr << "Query error (" << mysql_error(&mysql) << "): " << qstr << std::endl;
            return -1;
        }

        unblock();

        // Получаем дескриптор результирующей таблицы
        res = mysql_store_result(&mysql);
        if(res == NULL) {
            return -1;
        }

        results->num_rows = mysql_num_rows(res);
        results->num_fields = mysql_num_fields(res);

        MYSQL_FIELD *fields;

        fields = mysql_fetch_fields(res);

        while ((row = mysql_fetch_row(res)) != NULL) {
            db_row dbrow;
            for (int j = 0; j < results->num_fields; j++) {
                // std::cout << fields[j].name << " : " << row[j] << std::endl;
                dbrow.row.insert(std::pair<std::string,std::string>(fields[j].name, row[j]));
            }

            results->rows.insert(results->rows.end(), dbrow);
        }

        // delete dbrow;
        // dbrow = nullptr;

        mysql_free_result(res);

        return 0;
    }

    virtual void close()
    {
        mysql_close(&mysql);
    }
};

// the class factories
C_DLLEXPORT Db* create() {
	std::cout << "Export!" << std::endl;
    return new MySQL;
}

C_DLLEXPORT void destroy(Db* p) {
    delete p;
}

int main(int argc, char* argv[])
{
    return 0;
}
