#pragma once
#include "sdk.hpp"

#include "../../sdk/game_events.hpp"
#include "../../sdk/input.hpp"

#include "../features/network.hpp"

namespace lua {
	extern void init();
	extern void init_state(state_t& s);
	extern void callback(const std::string& name, const table_t& args = {}, std::function<void(state_t&)> callback = nullptr);
	extern void config_callback(const std::string& event_name, const network::config_t& config);
	extern void game_event(const std::string& name, sdk::game_event_t* e);
	extern void user_cmd_callback(const std::string& name, sdk::user_cmd_t* cmd);

	extern void load_script(const std::string& name, const std::string& id, const std::string& encoded);
	extern bool is_loaded(const std::string& name);
	extern void unload_script(const std::string& name);
	extern std::optional<std::string> get_script_id(lua_State* state);
} // namespace lua