#include <fstream>
#include <map>
#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>

struct Player {
    std::string first_name;
    std::string second_name;
    int goals_scored;
    int assists;
    int total_points;
    int minutes;
    int goals_conceded;
    double creativity;
    double influence;
    double threat;
    int bonus;
    int bps;
    double ict_index;
    int clean_sheets;
    int red_cards;
    int yellow_cards;
    double selected_by_percent;
    int now_cost;
    std::string element_type;
    double value_per_m;
};

class Simulation {
	private:
    std::vector<Player> players;

	public:
    Simulation(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Error: Could not open file.");
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string token;
            Player p;

            std::getline(ss, p.first_name, ',');
            std::getline(ss, p.second_name, ',');

            std::getline(ss, token, ','); p.goals_scored = std::stoi(token);
            std::getline(ss, token, ','); p.assists = std::stoi(token);
            std::getline(ss, token, ','); p.total_points = std::stoi(token);
            std::getline(ss, token, ','); p.minutes = std::stoi(token);
            std::getline(ss, token, ','); p.goals_conceded = std::stoi(token);

            std::getline(ss, token, ','); p.creativity = std::stod(token);
            std::getline(ss, token, ','); p.influence = std::stod(token);
            std::getline(ss, token, ','); p.threat = std::stod(token);

            std::getline(ss, token, ','); p.bonus = std::stoi(token);
            std::getline(ss, token, ','); p.bps = std::stoi(token);

            std::getline(ss, token, ','); p.ict_index = std::stod(token);

            std::getline(ss, token, ','); p.clean_sheets = std::stoi(token);
            std::getline(ss, token, ','); p.red_cards = std::stoi(token);
            std::getline(ss, token, ','); p.yellow_cards = std::stoi(token);

            std::getline(ss, token, ','); p.selected_by_percent = std::stod(token);

            std::getline(ss, token, ','); p.now_cost = std::stoi(token);
            std::getline(ss, p.element_type, ',');
            std::getline(ss, token, ','); p.value_per_m = std::stod(token);

            players.push_back(p);
        }
    }

	public:
    const std::vector<Player>& get_players() const {
        return players;
    }

	private:
	int calculate_starts(double starts_per_90, int total_minutes) const {
		return static_cast<int>(starts_per_90 * (total_minutes / 90.0) + 0.5);
	}

	private:
	int calculate_sub_minutes(int total_minutes, int starts) const {
		int sub_minutes = total_minutes - (starts * 80);
		if (sub_minutes < 0) {
			return 0;
		}

		return sub_minutes;
	}

	private:
	int calculate_estimated_subs(int sub_minutes) const {
		return static_cast<int>((sub_minutes / 20.0) + 0.5);
	}

	private:
	double calculate_p_start(int starts, int team_matches) const {
		if (team_matches == 0) {
			return 0.0;
		}

		return static_cast<double>(starts) / team_matches;
	}

	private:
	double calculate_p_sub(int estimated_subs, int team_matches) const {
		if (team_matches == 0) {
			return 0.0;
		}

		return static_cast<double>(estimated_subs) / team_matches;
	}

	private:
	double calculate_p_unused(double p_start, double p_sub) const {
		double p_unused = 1.0 - p_start - p_sub;
		if (p_unused < 0.0) {
			return 0.0;
		}

		return p_unused;
	}	

	private:
	struct TierData {
		int total_minutes = 0;
		int count = 0;
	};

	public:
	std::map<std::string, std::map<int, double>> calculate_global_average_minutes_by_rank() const {
		std::map<std::string, std::vector<const Player*>> grouped_by_position;
		
		for (const auto& p : players) {
			grouped_by_position[p.element_type].push_back(&p);
		}

		std::map<std::string, std::map<int, TierData>> rank_stats;

		for (auto& pos_it : grouped_by_position) {
			std::string position = pos_it.first;
			auto& pos_players = pos_it.second;
			
			std::sort(pos_players.begin(), pos_players.end(), [](const Player* a, const Player* b) {
				return a->now_cost > b->now_cost;
			});

			int global_rank = 1;
			for (const Player* p : pos_players) {
				rank_stats[position][global_rank].total_minutes += p->minutes;
				rank_stats[position][global_rank].count++;
				global_rank++;
			}
		}

		std::map<std::string, std::map<int, double>> average_minutes;
		
		for (const auto& pos_it : rank_stats) {
			for (const auto& rank_it : pos_it.second) {
				average_minutes[pos_it.first][rank_it.first] = 
					static_cast<double>(rank_it.second.total_minutes) / rank_it.second.count;
			}
		}

		return average_minutes;
	}
};
