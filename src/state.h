#ifndef GDAEMON_STATE_H
#define GDAEMON_STATE_H

#include <unordered_map>

/**
 * Global state
 */
class State {
    public:

        /**
         * Singleton
         *
         * @return State
         */
        static State& getInstance() {
            static State instance;
            return instance;
        }

        /**
         * Get state value
         *
         * @param key
         * @param value
         */
        void set(const std::string &key, const std::string &value);

        /**
         * Get state value
         *
         * @param key
         * @return
         * @throws out_of_range
         */
        std::string get(const std::string &key);

        /**
         * Check the element existence
         *
         * @param key
         * @return
         */
        bool exists(const std::string &key);

        /**
         * Remove element
         *
         * @param key
         */
        void remove(const std::string &key);
    private:
        State() {}
        State( const State&);
        State& operator=( State& );

        std::unordered_map<std::string, std::string> m_state;
};


#endif //GDAEMON_STATE_H
