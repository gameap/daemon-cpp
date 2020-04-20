#ifndef GDAEMON_BASE_OUTPUT_H
#define GDAEMON_BASE_OUTPUT_H

#include <memory>
#include <string>
#include <mutex>

namespace GameAP {
    class BaseOutput {
        public:
            /**
             * Initial output
             */
            virtual void init() = 0;

            /**
             * Append output
             * @param line
             */
            virtual void append(const std::string &line) = 0;

            /**
             * Get output
             * @param str_out
             */
            virtual void get(std::string *str_out) = 0;

            /**
             * Clear output buffer
             */
            virtual void clear() = 0;
    };
}

#endif //GDAEMON_BASE_OUTPUT_H
