#pragma once
#include <format>
#include <functional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

//#include "../../deps/http-transmit/base64.h"
#include "../utils/obfuscation.hpp"

#include "../utils/vector.hpp"

#include <lua.hpp>

#pragma comment(lib, "lua51")

namespace lua {
	extern std::string convert_name(const std::string& name);
	extern void perform_error(const std::string& text, bool critical = false);

	struct table_t {
		using member_t = std::pair<std::variant<std::string, int>, std::variant<int, float, bool, std::string, table_t, vec2d, vec3d, lua_CFunction>>;

		table_t(const std::vector<member_t>& members = {}) : m_members(members) {}

		void for_each(std::function<void(const member_t&)> callback) const {
			for (const auto& member: m_members)
				callback(member);
		}

		size_t members_count() const { return m_members.size(); }

	private:
		std::vector<member_t> m_members{};
	};

	struct state_t final {
		state_t() : l(luaL_newstate()), m_overrided(false) {}
		state_t(lua_State* state) : l(state), m_overrided(true) {}

		~state_t() {
			if (!m_overrided)
				lua_close(l);
		}

		using lua_function_t = lua_CFunction;

		state_t(const state_t&) = delete;
		state_t& operator=(const state_t&) = delete;

		operator lua_State*() { return l; }
		lua_State* operator*() { return l; }
		operator const lua_State*() const { return l; }
		const lua_State* operator*() const { return l; }

		uintptr_t get_id() const { return (uintptr_t)l; }

		bool init() {
			if (!open_libs())
				return false;

			// convient way to declare structs prototypes from C++ code
			if (!eval(STR(R"(
				new_object = {}
				function new_object:new(obj) self.__index = self return setmetatable(obj or {}, self) end)")))
				return false;

			return true;
		}

		bool open_libs() {
			luaL_openlibs(l);
			luaL_loadstring(l, "require 'ffi'");
			lua_pcall(l, 0, 0, 0);
			return true;
		}

		void new_table() { lua_newtable(l); }
		void set_global(const std::string& name) { lua_setglobal(l, name.c_str()); }
		void get_global(const std::string& name) { lua_getglobal(l, name.c_str()); }

		void bind_global_function(const std::string& name, lua_function_t function) {
			lua_pushcfunction(l, function);
			lua_setglobal(l, convert_name(name).c_str());
		}

		void bind_global_table(const std::string& name, const std::vector<std::pair<std::string, lua_function_t>>& functions, lua_function_t meta_callback = nullptr) {
			lua_newtable(l);

			for (const auto& [func_name, function]: functions) {
				lua_pushstring(l, convert_name(func_name).c_str());
				lua_pushcfunction(l, function);
				lua_settable(l, -3);
			}

			if (meta_callback != nullptr) {
				// Push a new metatable on the stack
				lua_newtable(l);

				// Push the __index key on the stack
				lua_pushstring(l, "__index");

				// Push a C++ function on the stack. This function will be called when a non-existent field is accessed.
				lua_pushcfunction(l, meta_callback);

				// The metatable now has an __index function associated with it
				lua_settable(l, -3);

				// Set the metatable for the table
				lua_setmetatable(l, -2);
			}

			lua_setglobal(l, convert_name(name).c_str());
		}

		void set_field(const std::string& name, int index) {
			lua_setfield(l, index, name.c_str());
		}

		int type(int index) {
			return lua_type(l, index);
		}

		void pop(int count = 1) {
			lua_pop(l, count);
		}

		bool is_table(int index) { return lua_istable(l, index); }
		bool is_function(int index) { return lua_isfunction(l, index); }
		bool is_string(int index) { return lua_isstring(l, index); }

		template<typename T>
		T get_field(const std::string& name, int index) {
			assert(false); // not implemented
		}

		template<>
		float get_field(const std::string& name, int index) {
			lua_getfield(l, index, name.c_str());
			const auto value = lua_tonumber(l, -1);
			lua_pop(l, 1);
			return (float)value;
		}

		template<>
		int get_field(const std::string& name, int index) {
			lua_getfield(l, index, name.c_str());
			const auto value = lua_tointeger(l, -1);
			lua_pop(l, 1);
			return (int)value;
		}

		template<typename T>
		std::optional<T> get_arg_impl(int index) {
			assert(false); // not implemented
		}

		template<>
		std::optional<int> get_arg_impl(int index) {
			if (!lua_isnumber(l, index))
				return std::nullopt;

			return (int)lua_tointeger(l, index);
		}

		template<>
		std::optional<float> get_arg_impl(int index) {
			if (!lua_isnumber(l, index))
				return std::nullopt;

			return (float)lua_tonumber(l, index);
		}

		template<>
		std::optional<bool> get_arg_impl(int index) {
			if (!lua_isboolean(l, index))
				return std::nullopt;

			return lua_toboolean(l, index);
		}

		template<>
		std::optional<std::string> get_arg_impl(int index) {
			if (!lua_isstring(l, index))
				return std::nullopt;

			return lua_tostring(l, index);
		}

		template<>
		std::optional<vec2d> get_arg_impl(int index) {
			if (!is_table(index))
				return std::nullopt;

			return vec2d{
				get_field<float>(STR("x"), index),
				get_field<float>(STR("y"), index)
			};
		}

		template<>
		std::optional<vec3d> get_arg_impl(int index) {
			if (!is_table(index))
				return std::nullopt;

			return vec3d{
				get_field<float>(STR("x"), index),
				get_field<float>(STR("y"), index),
				get_field<float>(STR("z"), index)
			};
		}

		template<>
		std::optional<color_t> get_arg_impl(int index) {
			if (!is_table(index))
				return std::nullopt;

			return color_t{
				get_field<int>(STR("r"), index),
				get_field<int>(STR("g"), index),
				get_field<int>(STR("b"), index),
				get_field<int>(STR("a"), index)
			};
		}

		template<>
		std::optional<std::vector<std::string>> get_arg_impl(int index) {
			if (!is_table(index))
				return std::nullopt;

			std::vector<std::string> result{};
			int len = lua_objlen(l, index);
			for (int i = 1; i <= len; ++i) {
				lua_pushinteger(l, i); // push index
				lua_gettable(l, -2);   // get table element
				const char* str = lua_tostring(l, -1);
				if (str != nullptr) result.emplace_back(str);
				lua_pop(l, 1); // pop the string value
			}

			if (result.empty())
				return std::nullopt;

			return result;
		}

		template<typename T>
		T arg(int index) {
			const auto x = get_arg_impl<T>(index);
			if (!x.has_value()) {
				perform_error(dformat(STRS("invalid argument at {} index"), index));
				return T{};
			}

			return *x;
		}

		template<typename T>
		T arg_or(const T& value, int index) {
			return get_arg_impl<T>(index).value_or(value);
		}

		void bind_struct(const std::string& ctor, const std::vector<std::string>& props) {
			std::string code = ctor + STR(" = function(");
			for (size_t i = 0; i < props.size(); i++) {
				code += props[i];
				if (i < props.size() - 1)
					code += STR(", ");
			}

			code += STR(") return new_object:new{");

			for (size_t i = 0; i < props.size(); i++) {
				code += props[i] + STR(" = ") + props[i];
				if (i < props.size() - 1)
					code += ", ";
			}

			code += STR("} end");

			if (luaL_dostring(l, code.c_str()))
				lua_pop(l, 1);
		}

		template<typename... Args>
		void push_struct(const std::string& ctor, Args&&... args) {
			lua_getglobal(l, ctor.c_str());
			push_values(std::forward<Args>(args)...);
		}

		void push(const std::string& value) { lua_pushstring(l, value.c_str()); }
		void push(lua_function_t fn) { lua_pushcfunction(l, fn); }
		void push(int value) { lua_pushinteger(l, value); }
		void push(unsigned int value) { lua_pushinteger(l, value); }
		void push(float value) { lua_pushnumber(l, value); }
		void push(bool value) { lua_pushboolean(l, value); }
		void push(std::nullptr_t) { lua_pushnil(l); }
		void push(const table_t& table) {
			new_table();
			table.for_each([this](const table_t::member_t& member) {
				std::visit(
						[this](auto&& key, auto&& value) {
							push(key);	 // push the key
							push(value); // push the value
						},
						member.first, member.second);
				lua_settable(l, -3);
			});
		}

		void push(vec2d v) {
			get_global(STR("vector"));

			push(v.x);
			push(v.y);

			call(2, 1);
		}

		void push(const vec3d& v) {
			get_global(STR("vector"));

			push(v.x);
			push(v.y);
			push(v.z);

			call(3, 1);
		}

		void push(color_t v) {
			get_global(STR("color"));

			push(v.r());
			push(v.g());
			push(v.b());
			push(v.a());

			call(4, 1);
		}

		void push_value(int index) { lua_pushvalue(l, index); }
		bool call(int args, int results, int errfunc = 0) {
			if (lua_pcall(l, args, results, errfunc) != 0) {
				perform_error(STR("pcall error: ") + lua_tostring(l, -1));
				lua_pop(l, 1); // remove error message from the stack
				return false;
			}

			return true;
		}

		bool eval(const std::string& code) {
			// Load the bytecode
			if (luaL_dostring(l, code.c_str()) != 0) {
				// Handle error. The error message is on the top of the stack.
				perform_error(lua_tostring(l, -1));
				lua_pop(l, 1); // remove error message from the stack
				return false;
			}

			return true;
		}

		template<typename... Args>
		int return_values(Args&&... args) {
			return _return_values(0, std::forward<Args>(args)...);
		}

		bool eval(const std::vector<uint8_t>& buf) {
			if (buf.empty())
				return false;

			// Load the bytecode
			if (luaL_loadbuffer(l, (const char*)buf.data(), buf.size(), nullptr) != 0) {
				// Handle error. The error message is on the top of the stack.
				perform_error(STRS("load buffer error: ") + lua_tostring(l, -1));
				lua_pop(l, 1); // remove error message from the stack
				return false;
			}

			try {
				if (lua_pcall(l, 0, LUA_MULTRET, 0) != 0) {
					// Handle error. The error message is on the top of the stack.
					perform_error(STRS("bytecode error: ") + lua_tostring(l, -1));
					lua_pop(l, 1); // remove error message from the stack
					return false;
				}
			} catch (std::exception& e) {
				perform_error(STRS("failed to load bytecode: ") + e.what());
				return false;
			} catch (...) {
				perform_error(STRS("unknown error"));
				return false;
			}

			return true;
		}

		bool eval_base64(const std::string& code) {
			return true/*eval(base64::decode(code))*/;
		}

	private:
		template<typename... Args>
		void push_values(Args&&... args) {
			_push_values(0, std::forward<Args>(args)...);
		}

		void _push_values(int count) {
			if (count > 0)
				lua_call(l, count, 1);
		}

		template<typename Arg, typename... Args>
		void _push_values(int count, Arg&& arg, Args&&... args) {
			push(std::forward<Arg>(arg));
			_push_values(count + 1, std::forward<Args>(args)...);
		}

		int _return_values(int count) { return count; }

		template<typename Arg, typename... Args>
		int _return_values(int count, Arg&& arg, Args&&... args) {
			push(std::forward<Arg>(arg));
			return _return_values(count + 1, std::forward<Args>(args)...);
		}

		lua_State* l{};
		bool m_overrided{};
	};
} // namespace lua

#define LUA_STATE(l) \
	auto s = lua::state_t { l }