#ifndef GDAEMON_GAME_SERVERS_LIST_H
#define GDAEMON_GAME_SERVERS_LIST_H

#include <memory>
#include <unordered_map>

#include "models/server.h"

namespace GameAP {

    struct GameServersListStats {
        unsigned int active_servers_count;
    };

    class GameServersList {
        public:
            static GameServersList& getInstance() {
                static GameServersList instance;
                return instance;
            }

            /**
             * Get Server model by server ID
             * @param server_id
             * @return  Server model or nullptr if server not exists
             */
            Server * get_server(unsigned long server_id);

            /**
             * Delete server from list
             * @param server_id
             */
            void delete_server(unsigned long server_id);

            /**
             * Some actions in loop (start crashed, update stats, etc.)
             */
            void loop();

            /**
             * Synchronization all servers details data to API
             */
            void sync_all();

            /**
             * Get game servers list stats (not server stats)
             * @return
             */
            GameServersListStats stats();

            /**
             * Set server install status (SERVER_NOT_INSTALLED, SERVER_INSTALLED, SERVER_INSTALL_IN_PROCESS)
             * @param server_id
             * @param status
             */
            void set_install_status(unsigned long server_id, Server::install_status status);
        private:
            std::unordered_map<unsigned long, std::shared_ptr<Server>> servers_list;

            /**
             * Time to live cache (seconds)
             */
            unsigned int cache_ttl;

            /**
             * Last updated servers list. Calling update_all method
             */
            time_t last_updated;

            /**
             * Synchronization server details data from API
             * @param server_id
             */
            void sync_from_api(unsigned long server_id);

            /**
             * Update, get servers from API. Cached (see cache_ttl and last_updated)
             */
            int update_list();

            /**
             * Update server stats and update servers data on api
             */
            void stats_process();

            /**
             * Check crashed servers and start them
             */
            void start_down_servers();

            /**
             * Singleton class. Private constructor.
             */
            GameServersList() {
                this->cache_ttl = 60;
                this->last_updated = 0;

                update_list();
            }

            GameServersList( const GameServersList&);
            GameServersList& operator=( GameServersList& );
    };

/* End namespace GameAP */
}

#endif //GDAEMON_GAME_SERVERS_LIST_H
