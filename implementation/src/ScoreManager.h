#pragma once
#include <gui/Application.h>

// Global persistent score manager for both games
class ScoreManager
{
public:
	struct GameStats
	{
		int wins = 0;
		int losses = 0;
	};

	static ScoreManager& getInstance()
	{
		static ScoreManager instance;
		return instance;
	}

	GameStats& getTTTStats() { return _tttStats; }
	GameStats& getC4Stats() { return _c4Stats; }

	void resetTTTStats() 
	{ 
		_tttStats = GameStats();
		saveTTTStats();
	}

	void resetC4Stats() 
	{ 
		_c4Stats = GameStats();
		saveC4Stats();
	}

	// Load scores from app properties
	void loadScores()
	{
		auto app = gui::getApplication();
		if (!app) return;
		auto props = app->getProperties();
		if (!props) return;

		_tttStats.wins = props->getValue("tttWins", 0);
		_tttStats.losses = props->getValue("tttLosses", 0);
		_c4Stats.wins = props->getValue("c4Wins", 0);
		_c4Stats.losses = props->getValue("c4Losses", 0);
	}

	// Save Tic-Tac-Toe scores to app properties
	void saveTTTStats()
	{
		auto app = gui::getApplication();
		if (!app) return;
		auto props = app->getProperties();
		if (!props) return;

		props->setValue("tttWins", _tttStats.wins);
		props->setValue("tttLosses", _tttStats.losses);
	}

	// Save Connect4 scores to app properties
	void saveC4Stats()
	{
		auto app = gui::getApplication();
		if (!app) return;
		auto props = app->getProperties();
		if (!props) return;

		props->setValue("c4Wins", _c4Stats.wins);
		props->setValue("c4Losses", _c4Stats.losses);
	}

private:
	ScoreManager() 
	{ 
		loadScores();  // Load from persistent storage on startup
	}
	GameStats _tttStats;
	GameStats _c4Stats;
};

