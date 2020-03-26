#include <sys/stat.h> //mkdir
#include <fstream>
#include <typeinfo>
#include "../formatters/helpers.hpp"
#ifndef SYS_CALLS_H_INCLUDED_
#define SYS_CALLS_H_INCLUDED_
using namespace std;

namespace sys_calls {
	enum FILE_FLAG{EXIST, DEL};

	bool file_handlers( string file_path, FILE_FLAG flag) {
		switch( flag ) {
			case EXIST:{
				struct stat buffer; 
				return stat( file_path.c_str(), &buffer) == 0;
			}
			break;

			case DEL:
				if( std::remove(file_path.c_str()) == 0) return true;
				else return false;
			break;

		}
		return false;
	}

	void make_dir( string path_dirname ) {
		size_t start_pos = path_dirname[0] == '/' ? 1 : 0;
		vector<string> recursive_paths = helpers::split(path_dirname, '/', true, start_pos);
		string make_me = recursive_paths[0];
		for(size_t i=0;i<recursive_paths.size();++i) {
			//logger::logger(__FUNCTION__, "Making dir: " + make_me, "stdout", false);
			if( i!=0) make_me += "/" + recursive_paths[i];
			if( mkdir( make_me.c_str(), 0777 ) == -1) {
				if( errno != 17 ) logger::logger_errno( errno );
			}
		}
		
		return;
	}

	string terminal_stdout(string command) {
		string data;
		FILE * stream;
		const int max_buffer = 1024;
		char buffer[max_buffer];
		command.append(" 2>&1");

		stream = popen(command.c_str(), "r");
		if (stream) {
			while (!feof(stream)) if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
			pclose(stream);
		}
		return data;
	}

	void terminal_stdout(map<string,string>& return_values, string command) {
		string data;
		FILE * stream;
		const int max_buffer = 1024;
		char buffer[max_buffer];
		command.append(" 2>&1");

		stream = popen(command.c_str(), "r");
		if (stream) {
			while (!feof(stream)) if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
			return_values.insert(make_pair("return", to_string(pclose(stream))));
		}
		return_values.insert(make_pair("data", data));
	}
	bool rename_file( string path_filename, string new_path_filename) {
		if(std::rename( path_filename.c_str(), new_path_filename.c_str()) == -1 ) {
			return false;
		}
		
		return true;
	}
	
	vector<string> get_modem_details ( string path_to_script, string index ) {
		vector<string> details;
		string type = index.find("192.168") != string::npos ? "ssh" : "mmcli";
		string modem_information = type == "ssh" ? sys_calls::terminal_stdout("ssh root@"+index+" -o 'ServerAliveInterval 10' deku") : sys_calls::terminal_stdout( path_to_script + "/modem_information_extraction.sh extract " + index );
		vector<string> ln_modem_information = helpers::split(modem_information, '\n', true);
		if( type == "ssh") {
			if(ln_modem_information.size() < 2) {
				//logger::logger(__FUNCTION__, "Incomplete data for modem at index: " + index, "stderr");
				return details;
			}
			
			if(ln_modem_information[0].find("deku:verified:") != string::npos) {
				details[0] = index; // equipment_id
				details[1] = helpers::to_upper(ln_modem_information[1]); // operator_name
				details[2] = type; // type
			}
		}
		else if( type == "mmcli") {
			if( ln_modem_information.size() != 3 ) {
				// Not valid modem
				return details;
			}

			details[0] = ln_modem_information[0]; // equipment_id
			details[1] = ln_modem_information[2]; // operator_name
			details[2] = type; // mmcli || ssh
		}
		return details;
	}

	map<string,map<string,string>> get_available_modems( string path_to_script ) {
		map<string,map<string,string>> available_modems;
		string list_of_modem_indexes = sys_calls::terminal_stdout( path_to_script + "/modem_information_extraction.sh list");
		vector<string> modem_indexes = helpers::split(list_of_modem_indexes, '\n', true);
		logger::logger(__FUNCTION__, "Listed " + to_string(modem_indexes.size()) + " index(es)");

		for(auto& index : modem_indexes ) {
			index = helpers::remove_char( index, ' ', 'E');
			vector<string> details = get_modem_details( path_to_script, index );
			if( details.size() != 4 ) continue;
			map<string,string> in_details = {
				{"imei", details[0]},
				{"signal_quality", details[1]},
				{"operator_name", details[2]},
				{"type", details[3]}
			};
			available_modems.insert(make_pair(details[0], in_details));
		}
		return available_modems;
	}
}


#endif
