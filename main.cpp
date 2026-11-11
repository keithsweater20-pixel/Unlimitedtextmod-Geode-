// Song Select Guard v0.1.0 (GD 2.2081, Windows)
//
// Crash being prevented (from a Geode crash log):
//   GeometryDash.exe + ButtonSprite::updateBGImage reads a null pointer,
//   called from SongSelectNode::onSongMode(int) when the song select opens.
//
// This mod does not fix the cause. It only skips onSongMode(int) when one of the
// pointers it uses is null, which is the state that crashes. The song mode may then
// not switch, but the game should not crash.

#include <Geode/Geode.hpp>
#include <Geode/modify/SongSelectNode.hpp>

using namespace geode::prelude;

class $modify(SongSelectGuard, SongSelectNode) {
	void onSongMode(int songMode) {
		if (!m_songWidget || !m_normalButton || !m_customButton || !m_selectButton ||
			!m_changeButton || !m_audioLabel || !m_selectSongObjects ||
			!m_normalSongObjects || !m_changeSongObjects) {
			log::warn("Song Select Guard: skipped onSongMode({}) because a required pointer is null", songMode);
			return;
		}
		SongSelectNode::onSongMode(songMode);
	}
};
