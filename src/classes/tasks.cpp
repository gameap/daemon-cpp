#include "tasks.h"
#include "config.h"
#include "consts.h"

#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/format.hpp>

#include <cstring>

#include "functions/restapi.h"
#include "functions/gsystem.h"

using namespace boost::process;
using namespace GameAP;

namespace fs = boost::filesystem;
namespace bp = ::boost::process;

void Task::run()
{
    if (m_status != waiting) {
        return;
    }

    if (m_server_id <= 0) {
        return;
    }

    m_task_started = time(nullptr);
    m_status = working;

    std::string qstr;

    try {
        Json::Value jdata;
        jdata["status"] = m_status;
        jdata["time_stchange"] = (uint) m_task_started;
        Gameap::Rest::put("/gdaemon_api/tasks/" + std::to_string(m_task_id), jdata);
    } catch (Gameap::Rest::RestapiException &exception) {
        std::cerr << "Error updating task status [to status code " + std::to_string(m_status) + "]: "
                  << exception.what()
                  << std::endl;

        return;
    }

    std::cout << "task: " << m_task << std::endl;

    int result_status;
    GameServersList& gslist = GameServersList::getInstance();

    m_gserver = gslist.get_server(m_server_id);
    m_gserver->update(true);
    
    if (! strcmp(m_task, TASK_GAME_SERVER_START)) {
        try {
            if (m_gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            m_gserver->clear_cmd_output();
            result_status = m_gserver->start_server();

            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_gserver->status_server();
        } catch (std::exception &e) {
            m_status = error;
            std::cerr << "gsstart error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(m_task, TASK_GAME_SERVER_STOP)) {
        try {
            if (m_gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            m_gserver->clear_cmd_output();
            result_status = m_gserver->stop_server();
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
            m_gserver->status_server();
        } catch (std::exception &e) {
            m_status = error;
            std::cerr << "gsstop error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(m_task, TASK_GAME_SERVER_RESTART)) {
        try {
            if (m_gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            m_gserver->clear_cmd_output();
            result_status = m_gserver->stop_server();
            result_status = m_gserver->start_server();

            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_gserver->status_server();
        } catch (std::exception &e) {
            m_status = error;
            std::cerr << "gsstop error: " << e.what() << std::endl;
        }
    }
    else if (!strcmp(m_task, TASK_GAME_SERVER_INSTALL) || !strcmp(m_task, TASK_GAME_SERVER_UPDATE)) {
        try {
            if (m_gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            m_gserver->clear_cmd_output();
            result_status = m_gserver->update_server();
        } catch (std::exception &e) {
            m_status = error;
            std::cerr << "gsinst error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(m_task, TASK_GAME_SERVER_DELETE)) {
        try {
            if (m_gserver == nullptr) {
                throw std::runtime_error("gslist.get_server error");
            }
            
            m_gserver->clear_cmd_output();
            result_status = m_gserver->delete_files();

            if (result_status == SUCCESS_STATUS_INT) {
                gslist.delete_server(m_server_id);
            }
        } catch (std::exception &e) {
            m_status = error;
            std::cerr << "gsdelete error: " << e.what() << std::endl;
        }
    }
    else if (! strcmp(m_task, TASK_GAME_SERVER_MOVE)) {
        // Move game server to other ds
        m_gserver->clear_cmd_output();
    }
    else if (! strcmp(m_task, TASK_GAME_SERVER_EXECUTE)) {
        try {
            if (m_server_id != 0) {
                m_gserver = gslist.get_server(m_server_id);

                if (m_gserver == nullptr) {
                    throw std::runtime_error("gslist.get_server error");
                }

                m_gserver->clear_cmd_output();
                result_status = m_gserver->cmd_exec(m_cmd);
            } else {
                m_cmd_output = "";
                result_status = _exec(m_cmd);
            }
        } catch (std::exception &e) {
            m_status = error;
            std::cerr << "cmdexec error: " << e.what() << std::endl;
        }
    }
    else {
        // Unknown m_task
        result_status = ERROR_STATUS_INT;
    }

    if (result_status == SUCCESS_STATUS_INT) {
        m_status = success;
    }
    else {
        m_status = error;
    }

    try {
        Json::Value jdata;
        jdata["status"] = m_status;
        Gameap::Rest::put("/gdaemon_api/tasks/" + std::to_string(m_task_id), jdata);
    } catch (Gameap::Rest::RestapiException &exception) {
        std::cerr << "Error updating task status [to status code " + std::to_string(m_status) + "]: "
                  << exception.what()
                  << std::endl;
    }
}

// ---------------------------------------------------------------------

int Task::_exec(std::string cmd)
{
    std::vector<std::string> split_lines;
    boost::split(split_lines, cmd, boost::is_any_of("\n\r"));

    for (std::vector<std::string>::iterator itl = split_lines.begin(); itl != split_lines.end(); ++itl) {
        if (*itl == "") continue;
        _single_exec(*itl);
    }

    return 0;
}

// ---------------------------------------------------------------------

int Task::_single_exec(std::string cmd)
{
    std::cout << "CMD Exec: " << cmd << std::endl;
    _append_cmd_output(fs::current_path().string() + "# " + cmd);

    int exit_code = exec(cmd, [this](std::string line) {
        m_cmd_output.append(line + "\n");
    });

    return exit_code;
}

// ---------------------------------------------------------------------

void Task::_append_cmd_output(std::string line)
{
    m_cmd_output.append(line + "\n");
}

// ---------------------------------------------------------------------

std::string Task::get_output()
{
    if (m_server_id != 0 && m_gserver != nullptr) {
        std::string output;

        output = m_gserver->get_cmd_output();

        if (output.size()-m_cur_outpos > 0) {
            std::string output_part = output.substr(m_cur_outpos, output.size());
            m_cur_outpos += (output.size() - m_cur_outpos);

            return output_part;
        } else {
            return "";
        }
    }

    if (m_server_id == 0) {
        if (m_cmd_output.size()-m_cur_outpos > 0) {
            std::string output_part = m_cmd_output.substr(m_cur_outpos, m_cmd_output.size());
            m_cur_outpos += (m_cmd_output.size() - m_cur_outpos);
            return output_part;
        }
    }

    return "";
}
