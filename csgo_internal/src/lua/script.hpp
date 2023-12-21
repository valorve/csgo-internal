#pragma once
#include "sdk.hpp"
#include "../../deps/imgui/imgui.h"
#include "../features/menu.hpp"

namespace lua {
	struct script_t final {
		std::mutex m_mutex{};
		std::string m_id{};
		script_t() {
			m_state = std::make_unique<state_t>();
			m_state->init();
		}

		bool load(const std::string& id, const std::string& encoded);

		void callback(const std::string& name, const table_t& args, std::function<void(state_t&)> callback = nullptr);

		~script_t() {
			delete m_atlas;
		}

		const state_t& get_state() const { return *m_state; }

	private:
		std::unique_ptr<state_t> m_state{};
		ImFontAtlas* m_atlas{};
		LPDIRECT3DTEXTURE9 m_font_texture{};

		bool dx9_create_font_textures();
	};

	template<typename T>
	using script_storage_impl_t = std::pair<std::string, T>;

	class version_t {
	public:
		version_t(const std::string& value) : m_value(value) {}

		bool is_backward_compatibility(const version_t& other) const {
			std::vector<int> this_parts = parse(m_value);
			std::vector<int> other_parts = parse(other.m_value);

			size_t min_length = std::min<size_t>(this_parts.size(), other_parts.size());

			for (size_t i = 0; i < min_length; ++i) {
				if (this_parts[i] < other_parts[i])
					return false;

				if (this_parts[i] > other_parts[i])
					return true;
			}

			// If all compared parts are equal, the longer version is not backward compatible
			return this_parts.size() >= other_parts.size();
		}

		std::string get() const { return m_value; }

		static std::vector<int> parse(const std::string& version) {
			std::vector<int> version_parts;
			std::string part;

			for (char c: version) {
				if (c == '.') {
					version_parts.push_back(std::stoi(part));
					part.clear();
				} else if (isdigit(c))
					part += c;
				else
					return {};
			}

			if (!part.empty())
				version_parts.push_back(std::stoi(part));

			return version_parts;
		}

	private:
		std::string m_value;
	};

	struct tab_t {
		std::string m_name{};
		std::string m_icon{};
		std::pair<color_t, color_t> m_colors{};
		//menu_t::content_items_t m_items{};
	};

	struct script_storage_t final {
		struct font_t {
			void** m_pointer{};
			int m_flags{};
			std::string m_path{};
			float m_size{};
		};

		struct texture_t {
			void** m_pointer{};
			std::string m_path{};
		};

		std::vector<script_storage_t::font_t> m_font_queue{};
		std::vector<script_storage_t::texture_t> m_texture_queue{};

		std::vector<std::shared_ptr<tab_t>> m_tabs{};

		std::vector<gui::i_renderable*> m_menu_items{};

		std::list<script_storage_impl_t<incheat_vars::types::bool_t>> m_bools{};
		std::list<script_storage_impl_t<incheat_vars::types::int_t>> m_ints{};
		std::list<script_storage_impl_t<incheat_vars::types::color_t>> m_colors{};

		std::string m_name{};
		version_t m_version = { "1.0" };

		~script_storage_t() {
			for (auto& font: m_font_queue)
				delete (uintptr_t*)font.m_pointer;

			for (auto& texture: m_texture_queue)
				delete (uintptr_t*)texture.m_pointer;

			gui::containers::recursive_delete(m_menu_items);
		}
	};

	inline std::unordered_map<uintptr_t, script_storage_t> scripts_storage{};
} // namespace lua