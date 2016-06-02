#include "consts.h"

#include <boost/filesystem.hpp>

#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

#include <jsoncpp/json/json.h>

#include <sys/types.h>
#include <signal.h>

#include "db/db.h"
#include "game_server.h"

#include "functions/gsystem.h"
#include "functions/gstring.h"

#include "models/tasks.h"

using namespace GameAP;

namespace bp = ::boost::process;
using namespace boost::process;
using namespace boost::process::initializers;
/*

bp::child start_child() 
{ 
    std::string exec = "ls";

    std::vector<std::string> args; 
    args.push_back("--version");

    bp::context ctx; 
    ctx.stdout_behavior = bp::silence_stream(); 

    return bp::launch(exec, args, ctx); 
} 

int GameServers::start_server()
{
    bp::child c = start_child(); 

    bp::pistream &is = c.get_stdout(); 
    std::string line; 
    while (std::getline(is, line)) 
        std::cout << line << std::endl; 
}
*/

// ---------------------------------------------------------------------

GameServer::GameServer(ulong mserver_id)
{
    server_id = mserver_id;

    std::string qstr = str(boost::format(
        "SELECT {pref}servers.*,\
            {pref}games.remote_repository AS game_remote_repository, {pref}games.local_repository AS game_local_repository,\
            {pref}dedicated_servers.work_path,\
            {pref}game_types.remote_repository AS gt_remote_repository, {pref}game_types.local_repository AS gt_local_repository\
        FROM `{pref}servers`\
        RIGHT JOIN {pref}dedicated_servers ON {pref}dedicated_servers.id={pref}servers.ds_id\
        RIGHT JOIN {pref}games ON {pref}games.code={pref}servers.game\
        RIGHT JOIN {pref}game_types ON {pref}game_types.id={pref}servers.game_type\
        WHERE {pref}servers.`id` = %1%\
        LIMIT 1\
        "
    ) % server_id);

    db_elems results;
    if (db->query(&qstr[0], &results) == -1) {
        fprintf(stdout, "Error query\n");
        return;
    }

    work_path = results.rows[0].row["work_path"];   // DS dir
    work_path /= results.rows[0].row["dir"];        // Game server dir

    ip              = results.rows[0].row["server_ip"];
    server_port     = (ulong)atoi(results.rows[0].row["server_port"].c_str());
    query_port      = (ulong)atoi(results.rows[0].row["query_port"].c_str());
    rcon_port       = (ulong)atoi(results.rows[0].row["rcon_port"].c_str());

    start_command   = results.rows[0].row["start_command"];

    game_localrep  = results.rows[0].row["game_local_repository"];
    game_remrep    = results.rows[0].row["game_remote_repository"];
    gt_localrep    = results.rows[0].row["gt_local_repository"];
    gt_remrep      = results.rows[0].row["gt_remote_repository"];

    Json::Value jaliases;

    Json::Reader jreader(Json::Features::strictMode());
    jreader.parse(results.rows[0].row["aliases"], jaliases, false);

    for( Json::ValueIterator itr = jaliases.begin() ; itr != jaliases.end() ; itr++ ) {
        aliases.insert(std::pair<std::string, std::string>(itr.key().asString(), (*itr).asString()));
    }
}

// ---------------------------------------------------------------------

void GameServer::_append_cmd_output(std::string line)
{
    cmd_output = cmd_output + line + '\n';
}

// ---------------------------------------------------------------------

void GameServer::replace_shortcodes(std::string &cmd)
{
    cmd = str_replace("{dir}", work_path.string(), cmd);
    cmd = str_replace("{name}", screen_name, cmd);
    
    cmd = str_replace("{ip}", ip, cmd);

	#ifdef __GNUC__
		cmd = str_replace("{id}", std::to_string(server_id), cmd);
		cmd = str_replace("{port}", std::to_string(server_port), cmd);
		cmd = str_replace("{query_port}", std::to_string(query_port), cmd);
		cmd = str_replace("{rcon_port}", std::to_string(rcon_port), cmd);
	#elif _WIN32
		cmd = str_replace("{id}", std::to_string((_ULonglong)server_id), cmd);
		cmd = str_replace("{port}", std::to_string((_ULonglong)server_port), cmd);
		cmd = str_replace("{query_port}", std::to_string((_ULonglong)query_port), cmd);
		cmd = str_replace("{rcon_port}", std::to_string((_ULonglong)rcon_port), cmd);
	#endif
    
    cmd = str_replace("{user}", user, cmd);

    // Aliases
    for (std::map<std::string, std::string>::iterator itr = aliases.begin(); itr != aliases.end(); ++itr) {
        cmd = str_replace("{" + itr->first + "}", itr->second, cmd);
    }
}

// ---------------------------------------------------------------------

int GameServer::start_server()
{
    DedicatedServer& deds = DedicatedServer::getInstance();
    std::string cmd  = str_replace("{command}", start_command, deds.get_script_cmd(DS_SCRIPT_START));
    replace_shortcodes(cmd);

    int result = _exec(cmd);
    
    if (result == -1) {
        return -1;
    } else {
        return 0;
    }
}

// ---------------------------------------------------------------------

int GameServer::stop_server()
{
    DedicatedServer& deds = DedicatedServer::getInstance();
    // std::string cmd  = str_replace("{command}", "", deds.get_script_cmd(DS_SCRIPT_STOP));
    std::string cmd  = deds.get_script_cmd(DS_SCRIPT_STOP);
    replace_shortcodes(cmd);

    int result = _exec(cmd);

    if (result == -1) {
        return -1;
    } else {
        return 0;
    }
}

// ---------------------------------------------------------------------

int GameServer::update_server()
{
    std::cout << "Update Start" << std::endl;

    ushort game_install_from =  INST_NO_SOURCE;
    ushort gt_install_from =    INST_NO_SOURCE;

    // Install game
    if (game_localrep.size() > 0)       game_install_from = INST_FROM_LOCREP;
    else if (game_remrep.size() > 0)    game_install_from = INST_FROM_REMREP;
    else if (false)                     game_install_from = INST_FROM_STEAM;
    else {
        // No Source to install =(
        std::cerr << "No source to intall" << std::endl;
        return -1;
    }

    if (gt_localrep.size() > 0)         gt_install_from = INST_FROM_LOCREP;
    else if (gt_remrep.size() > 0)      gt_install_from = INST_FROM_REMREP;
    else if (false)                     gt_install_from = INST_FROM_STEAM;
    else {
        // No Source to install. No return -1
    }

    ushort game_source =    INST_NO_SOURCE;
    ushort gt_source =      INST_NO_SOURCE;

    boost::filesystem::path source_path;
    
    if (game_install_from == INST_FROM_LOCREP) {

        if (boost::filesystem::is_regular_file(game_localrep)) {
            game_source = INST_FILE;
            source_path = game_localrep;
        }
        else if (boost::filesystem::is_directory(game_localrep)) {
            game_source = INST_DIR;
            source_path = game_localrep;
        } else {
            game_install_from = INST_FROM_REMREP;
        }

        if (!boost::filesystem::exists(source_path)) {
            std::cerr << "Local rep not found: " << source_path << std::endl;
            game_install_from = INST_FROM_REMREP;
            source_path = game_remrep;
        }
    }

    if (game_install_from == INST_FROM_REMREP) {
        // Check rep available
        // TODO ...
        game_source = INST_FILE;
        source_path = game_remrep;
    }

    if (game_install_from == INST_FROM_STEAM) {
        // TODO
    }

    // Mkdir
    if (!boost::filesystem::exists(work_path)) {
        boost::filesystem::create_directories(work_path);
    }

    // Wget/Copy and unpack
    if (game_install_from == INST_FROM_LOCREP && game_source == INST_FILE) {
        _unpack_archive(game_localrep);
    }
    else if (game_install_from == INST_FROM_LOCREP && game_source == INST_DIR) {
        _copy_dir(source_path, work_path);
    }
    else if (game_install_from == INST_FROM_REMREP) {
        std::string cmd = str(boost::format("wget -N -c %1% -P %2% ") % source_path.string() % work_path.string());

        if (_exec(cmd) == -1) {
            return -1;
        }
        
        std::string archive = str(boost::format("%1%/%2%") % work_path.string() % source_path.filename().string());
        
        _unpack_archive(archive);
        boost::filesystem::remove(archive);
    }

    // Game Type Install

    if (gt_install_from == INST_FROM_LOCREP) {

        if (boost::filesystem::is_regular_file(gt_localrep)) {
            gt_source = INST_FILE;
            source_path = gt_localrep;
        }
        else if (boost::filesystem::is_directory(gt_localrep)) {
            gt_source = INST_DIR;
            source_path = gt_localrep;
        } else {
            gt_install_from = INST_FROM_REMREP;
        }

        if (!boost::filesystem::exists(source_path)) {
            std::cerr << "Local rep not found: " << source_path << std::endl;
            gt_install_from = INST_FROM_REMREP;
            source_path = gt_remrep;
        }
    }

    if (gt_install_from == INST_FROM_REMREP) {
        // Check rep available
        // TODO ...
        gt_source = INST_FILE;
        source_path = gt_remrep;
    }

    // Wget/Copy and unpack
    if (gt_install_from == INST_FROM_LOCREP && gt_source == INST_FILE) {
        _unpack_archive(game_localrep);
    }
    else if (gt_install_from == INST_FROM_LOCREP && gt_source == INST_DIR) {
        _copy_dir(source_path, work_path);
    }
    else if (gt_install_from == INST_FROM_REMREP) {
        std::string cmd = str(boost::format("wget -N -c %1% -P %2% ") % source_path.string() % work_path.string());

        if (_exec(cmd) == -1) {
            return -1;
        }
        
        std::string archive = str(boost::format("%1%/%2%") % work_path.string() % source_path.filename().string());
        
        _unpack_archive(archive);
        boost::filesystem::remove(archive);
    }

    return 0;
}

// ---------------------------------------------------------------------

int GameServer::_unpack_archive(boost::filesystem::path const & archive)
{
    std::string cmd;

    if (archive.extension().string() == ".xz")           cmd = str(boost::format("tar -xpvJf %1% -C %2%") % archive.string() % work_path.string());
    else if (archive.extension().string() == ".gz")      cmd = str(boost::format("tar -xvf %1% -C %2%") % archive.string() % work_path.string());
    else if (archive.extension().string() == ".bz2")     cmd = str(boost::format("tar -xvf %1% -C %2%") % archive.string() % work_path.string());
    else if (archive.extension().string() == ".tar")     cmd = str(boost::format("tar -xvf %1% -C %2%") % archive.string() % work_path.string());
    else cmd = str(boost::format("unzip -o %1% -d %2%") % archive.string() % work_path.string());

    if (_exec(cmd) == -1) {
        return -1;
    }
}

int GameServer::_exec(std::string cmd)
{
    _append_cmd_output("CMD# " + cmd);

    boost::process::pipe out = boost::process::create_pipe();

    boost::iostreams::file_descriptor_source source(out.source, boost::iostreams::close_handle);
    boost::iostreams::stream<boost::iostreams::file_descriptor_source> is(source);
    std::string s;
    
    std::cout << "Exec: " << cmd << std::endl;

    size_t pid;

    child c = exec(cmd, out);
    
    while (!is.eof()) {
        std::getline(is, s);
        _append_cmd_output(s);
    }

    auto exit_code = wait_for_exit(c);

    return 0;
}

// ---------------------------------------------------------------------

bool GameServer::_copy_dir(
    boost::filesystem::path const & source,
    boost::filesystem::path const & destination
) {
    try {
        // Check whether the function call is valid
        if(!boost::filesystem::exists(source) || !boost::filesystem::is_directory(source)) {
            std::cerr << "Source directory " << source.string()
                << " does not exist or is not a directory." << '\n';
            return false;
        }
        
        if (!boost::filesystem::exists(destination)) {
            // Create the destination directory
            if(!boost::filesystem::create_directory(destination)) {
                std::cerr << "Unable to create destination directory"
                    << destination.string() << '\n';
                return false;
            }
        }
        
    } catch(boost::filesystem::filesystem_error const & e) {
        std::cerr << e.what() << std::endl;
        return false;
    }

    // Iterate through the source directory
    for(boost::filesystem::directory_iterator file(source);
        file != boost::filesystem::directory_iterator(); ++file
    ) {
        try {
            boost::filesystem::path current(file->path());
            if(boost::filesystem::is_directory(current)) {
                // Found directory: Recursion
                if(!_copy_dir(current, destination / current.filename())) {
                    return false;
                }
            } else {
                // Found file: Copy
                boost::filesystem::copy(current, destination / current.filename());
            }
        } catch(boost::filesystem::filesystem_error const & e) {
            std:: cerr << e.what() << std::endl;
        }
    }
    
    return true;
}

// ---------------------------------------------------------------------

int GameServer::delete_server()
{
    try {
        boost::filesystem::remove_all(work_path);
    }
    catch (boost::filesystem::filesystem_error &e) {
        std::cerr << "Error remove: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
