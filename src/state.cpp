#include "state.h"

/**
 * Get state value
 *
 * @param key
 * @param value
 */
void State::set(const std::string &key, const std::string &value)
{
    m_state.insert(
            std::pair<std::string, std::string>(key, value)
    );
}

/**
 * Get state value
 *
 * @param key
 * @return
 * @throws out_of_range
 */
std::string State::get(const std::string &key)
{
    return m_state.at(key);
}

/**
 * Check the element existence
 *
 * @param key
 * @return
 */
bool State::exists(const std::string &key)
{
    return m_state.find(key) != m_state.end();
}

/**
 * Remove element
 *
 * @param key
 */
void State::remove(const std::string &key)
{
    m_state.erase(key);
}