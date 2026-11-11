// Special Characters v0.1.0 for Geode (GD 2.2081, Windows)
//
// NOTE: written without being able to compile against the Geode SDK. Hook
// signatures were checked against the 2.2081 bindings by reading them, but if
// the compiler complains, fix the signature to match your SDK's headers.
//
// Two halves:
//   1. Typing: GD text fields only accept the characters on their allow list, so
//      you type a short code (backslash + e0, w, ...) and the mod swaps in the
//      real UTF-8 symbol.
//   2. Showing: not done in v0.1.1. Swapping the label font crashed GD on
//      startup (see README), so symbols typed here are not drawn by GD's fonts yet.

#include <Geode/Geode.hpp>
#include <Geode/modify/CCTextInputNode.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>

using namespace geode::prelude;

namespace sc {

	struct Code {
		char const* name;
		char const* glyphs;
	};

	// Typed as <prefix><name>, e.g. \e0 or \w. No name is the start of another
	// name, so a code is replaced the moment its last character is typed.
	static constexpr Code kCodes[] = {
		{"e0", "ε₀"},
		{"eps0", "ε₀"},
		{"a0", "ℵ₀"},
		{"aleph0", "ℵ₀"},
		{"w", "ω"},
		{"W", "Ω"},
		{"alpha", "α"},
		{"beta", "β"},
		{"gamma", "γ"},
		{"delta", "δ"},
		{"theta", "θ"},
		{"lambda", "λ"},
		{"mu", "μ"},
		{"pi", "π"},
		{"sigma", "σ"},
		{"phi", "φ"},
		{"inf", "∞"},
	};

	// Replaces every <prefix><code> in `in`. `changed` says whether anything was replaced.
	inline std::string replaceCodes(std::string const& in, std::string const& prefix, bool& changed) {
		changed = false;
		if (prefix.empty()) return in;

		std::string out;
		out.reserve(in.size());
		size_t i = 0;
		while (i < in.size()) {
			if (in.compare(i, prefix.size(), prefix) == 0) {
				bool matched = false;
				for (auto const& code : kCodes) {
					size_t n = std::strlen(code.name);
					if (in.compare(i + prefix.size(), n, code.name) == 0) {
						out += code.glyphs;
						i += prefix.size() + n;
						matched = true;
						changed = true;
						break;
					}
				}
				if (matched) continue;
			}
			out += in[i++];
		}
		return out;
	}

	// Keep in sync with CODEPOINTS in tools/make_font.py.
	inline bool isSpecialCodepoint(uint32_t cp) {
		return (cp >= 0x391 && cp <= 0x3A9)       // capital Greek
			|| (cp >= 0x3B1 && cp <= 0x3C9)       // lowercase Greek
			|| (cp >= 0x2080 && cp <= 0x2089)     // subscript digits
			|| cp == 0x2135                       // aleph
			|| cp == 0x221E;                      // infinity
	}

	// True if the UTF-8 string contains a symbol our font provides. Cheap: ASCII is skipped byte by byte.
	inline bool hasSpecial(char const* s) {
		if (!s) return false;
		auto p = reinterpret_cast<unsigned char const*>(s);
		while (*p) {
			if (*p < 0x80) {
				++p;
				continue;
			}
			uint32_t cp = 0;
			int len = 1;
			if ((*p & 0xE0) == 0xC0) { cp = *p & 0x1F; len = 2; }
			else if ((*p & 0xF0) == 0xE0) { cp = *p & 0x0F; len = 3; }
			else if ((*p & 0xF8) == 0xF0) { cp = *p & 0x07; len = 4; }
			else { ++p; continue; }

			int i = 1;
			for (; i < len && (p[i] & 0xC0) == 0x80; ++i) {
				cp = (cp << 6) | (p[i] & 0x3F);
			}
			if (i == len && isSpecialCodepoint(cp)) return true;
			p += i;
		}
		return false;
	}

	// Path of the bundled font, or empty if it could not be found.
	inline std::string const& fontPath() {
		static std::string path = [] {
			auto mod = Mod::get();
			auto dir = mod->getResourcesDir();
			std::error_code ec;
			// Depending on how the package lays resources out, the file is either
			// directly in the resources folder or in a folder named after the mod id.
			std::string id = std::string(mod->getID());
			for (auto const& p : {dir / "specialFont.fnt", dir / id / "specialFont.fnt"}) {
				if (std::filesystem::exists(p, ec)) return p.string();
			}
			log::warn("specialFont.fnt not found under {}", dir.string());
			return std::string();
		}();
		return path;
	}

} // namespace sc

// ---------------------------------------------------------------- typing ----

class $modify(SCTextInput, CCTextInputNode) {
	bool onTextFieldInsertText(CCTextFieldTTF* sender, char const* text, int len, enumKeyCodes key) {
		auto mod = Mod::get();
		if (!text || len <= 0 || !mod->getSettingValue<bool>("enable-codes")) {
			return CCTextInputNode::onTextFieldInsertText(sender, text, len, key);
		}
		std::string prefix = mod->getSettingValue<std::string>("prefix");
		if (prefix.empty()) {
			return CCTextInputNode::onTextFieldInsertText(sender, text, len, key);
		}

		// 1. This keystroke (or paste) completes a code: put the symbol in ourselves.
		if (m_textField && m_textField->m_pInputText) {
			std::string typed(text, static_cast<size_t>(len));
			std::string combined = std::string(m_textField->m_pInputText->c_str()) + typed;
			bool changed = false;
			std::string replaced = sc::replaceCodes(combined, prefix, changed);
			if (changed) {
				m_textField->setString(replaced.c_str());
				this->updateLabel(replaced);
				if (m_delegate) m_delegate->textChanged(this);
				return true; // handled: the field must not insert the raw characters as well
			}
		}

		// 2. Many text fields have an allow list that has no backslash. In fields that
		//    clearly take text (the list contains letters), let the prefix through.
		std::string allowed = m_allowedChars;
		if (!allowed.empty() && allowed.find('e') != std::string::npos) {
			std::string widened = allowed;
			for (char c : prefix) {
				if (widened.find(c) == std::string::npos) widened += c;
			}
			if (widened != allowed) {
				m_allowedChars = widened;
				bool result = CCTextInputNode::onTextFieldInsertText(sender, text, len, key);
				m_allowedChars = allowed;
				return result;
			}
		}

		return CCTextInputNode::onTextFieldInsertText(sender, text, len, key);
	}
};
