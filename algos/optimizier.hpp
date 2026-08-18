#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <map>
#include <utility>
#include <cmath>

struct Player {
    std::string first_name;
    std::string second_name;
    int team;
    std::string element_type;   // normalized to "GK" / "DEF" / "MID" / "FWD"
    int now_cost;
    int minutes;
    int starts;
    int appearances;
    int total_points;
    double points_per_game;
    int goals_scored;
    int assists;
    int clean_sheets;
    int goals_conceded;
    double defensive_contribution_per_90;
    int bonus;
    int yellow_cards;
    int red_cards;
    double avg_minutes_per_game;
    double ev;
};

class Optimizer {
	private:
    std::vector<Player> players;

	public:
    Optimizer(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Error: Could not open the file: " + filepath);
        }

        std::string line;
        if (!std::getline(file, line)) return;

        std::map<std::string, int> header_map;
        std::stringstream header_ss(line);
        std::string col_name;
        int col_idx = 0;

        while (std::getline(header_ss, col_name, ',')) {
            strip_cr(col_name);
            header_map[col_name] = col_idx++;
        }

        int line_no = 1;
        int rows_skipped = 0;

        while (std::getline(file, line)) {
            ++line_no;
            if (line.empty()) continue;

            std::vector<std::string> row;
            std::stringstream ss(line);
            std::string token;
            while (std::getline(ss, token, ',')) {
                row.push_back(token);
            }
            if (!row.empty()) strip_cr(row.back());

            auto get_val = [&](const std::string& key) -> const std::string& {
                auto it = header_map.find(key);
                if (it == header_map.end()) {
                    throw std::runtime_error("CSV header is missing expected column '" + key + "'");
                }
                size_t idx = static_cast<size_t>(it->second);
                if (idx >= row.size()) {
                    throw std::runtime_error("Row " + std::to_string(line_no) +
                        " has too few fields for column '" + key + "'");
                }
                return row[idx];
            };

            Player p;
            try {
                p.first_name    = get_val("first_name");
                p.second_name   = get_val("second_name");
                p.team          = std::stoi(get_val("team"));
                p.element_type  = parse_element_type(get_val("element_type"));
                p.now_cost      = std::stoi(get_val("now_cost"));
                p.minutes       = std::stoi(get_val("minutes"));
                p.starts        = std::stoi(get_val("starts"));
                p.total_points  = std::stoi(get_val("total_points"));
                p.points_per_game = std::stod(get_val("points_per_game"));
                p.goals_scored  = std::stoi(get_val("goals_scored"));
                p.assists       = std::stoi(get_val("assists"));
                p.clean_sheets  = std::stoi(get_val("clean_sheets"));
                p.goals_conceded = std::stoi(get_val("goals_conceded"));
                p.defensive_contribution_per_90 = std::stod(get_val("defensive_contribution_per_90"));
                p.bonus         = std::stoi(get_val("bonus"));
                p.yellow_cards  = std::stoi(get_val("yellow_cards"));
                p.red_cards     = std::stoi(get_val("red_cards"));
            } catch (const std::exception& e) {
                std::cerr << "Skipping row " << line_no << " (" << e.what() << "): "
                          << line.substr(0, 80) << "\n";
                ++rows_skipped;
                continue;
            }

            if (p.points_per_game > 0.0) {
                p.appearances = static_cast<int>((p.total_points / p.points_per_game) + 0.5);
            } else {
                p.appearances = 0;
            }

            p.avg_minutes_per_game = (p.appearances > 0)
                ? static_cast<double>(p.minutes) / p.appearances
                : 0.0;

            p.ev = 0.0;
            players.push_back(p);
        }

        if (rows_skipped > 0) {
            std::cerr << rows_skipped << " row(s) skipped during load — see messages above.\n";
        }
        std::cerr << "Loaded " << players.size() << " players from " << filepath << "\n";

        prune_non_playing_players();
        assign_ev();
    }

	public:
    const std::vector<Player>& get_players() const {
        return players;
    }

	public:
    std::vector<Player> get_solution_players() {
        return std::vector<Player>();
    }

	public:
    void export_ev_to_csv(const std::string& filename = "../data/player_ev_data.csv") const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Error: Could not open file for exporting EV: " + filename);
        }

        file << "first_name,second_name,element_type,team,now_cost,minutes,ev\n";
        for (const auto& p : players) {
            file << p.first_name << ","
                 << p.second_name << ","
                 << p.element_type << ","
                 << p.team << ","
                 << p.now_cost << ","
                 << p.minutes << ","
                 << p.ev << "\n";
        }
        file.close();
    }

	private:
    static void strip_cr(std::string& s) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
    }

    static std::string parse_element_type(const std::string& token) {
        static const std::map<int, std::string> pos_map = {
            {1, "GK"}, {2, "DEF"}, {3, "MID"}, {4, "FWD"}
        };
        try {
            int code = std::stoi(token);
            auto it = pos_map.find(code);
            if (it != pos_map.end()) return it->second;
        } catch (...) {
            // not numeric — fall through, try as a direct label below
        }
        if (token == "GK" || token == "DEF" || token == "MID" || token == "FWD") {
            return token;
        }
        throw std::runtime_error("Unrecognized element_type value: '" + token + "'");
    }

	private:
    void prune_non_playing_players() {
        std::vector<Player> active_players;
        std::map<std::pair<int, std::string>, Player> cheapest_fodder;
        std::map<std::pair<int, std::string>, int> min_costs;

        for (int t = 1; t <= 20; ++t) {
            min_costs[{t, "GK"}] = 999;
            min_costs[{t, "DEF"}] = 999;
            min_costs[{t, "MID"}] = 999;
            min_costs[{t, "FWD"}] = 999;
        }

        // ~5 full matches. Below this, per-game rate stats are computed off too
        const int MEANINGFUL_MINUTES_THRESHOLD = 450;

        for (const auto& p : players) {
            if (p.minutes > MEANINGFUL_MINUTES_THRESHOLD) {
                active_players.push_back(p);
            } else {
                std::pair<int, std::string> key = {p.team, p.element_type};
                if (p.now_cost < min_costs[key]) {
                    min_costs[key] = p.now_cost;
                    cheapest_fodder[key] = p;
                }
            }
        }

        for (const auto& pair : cheapest_fodder) {
            if (pair.second.now_cost > 0) {
                active_players.push_back(pair.second);
            }
        }

        players = active_players;
    }

	private:
    double poisson_cdf(int k, double lambda) const {
        if (lambda <= 0.0) return 1.0;
        double sum = 0.0;
        double term = std::exp(-lambda);
        for (int i = 0; i <= k; ++i) {
            sum += term;
            term *= lambda / (i + 1.0);
        }
        return sum;
    }

	private:
    double calculate_appearance_ev(const Player& p) const {
        if (p.appearances == 0) return 0.0;

        double p_60_plus = static_cast<double>(p.starts) / p.appearances;
        double p_1_to_59 = static_cast<double>(p.appearances - p.starts) / p.appearances;

        return (p_60_plus * 2.0) + (p_1_to_59 * 1.0);
    }

	private:
    double calculate_attack_ev(const Player& p) const {
        if (p.appearances == 0) return 0.0;

        int goal_multiplier = 0;
        if (p.element_type == "FWD") goal_multiplier = 4;
        else if (p.element_type == "MID") goal_multiplier = 5;
        else goal_multiplier = 6; // GK/DEF

        double goals_per_game = static_cast<double>(p.goals_scored) / p.appearances;
        double assists_per_game = static_cast<double>(p.assists) / p.appearances;

        return (goals_per_game * goal_multiplier) + (assists_per_game * 3.0);
    }

	private:
    double calculate_defensive_action_ev(const Player& p) const {
        if (p.element_type == "GK") return 0.0;

        double rate = p.defensive_contribution_per_90;
        if (rate <= 0.0) return 0.0;

        int threshold = (p.element_type == "DEF") ? 10 : 12;
        double p_hit = 1.0 - poisson_cdf(threshold - 1, rate);

        return p_hit * 2.0;
    }

	private:
    double calculate_defense_ev(const Player& p) const {
        if (p.appearances == 0) return 0.0;

        double cs_per_game = static_cast<double>(p.clean_sheets) / p.appearances;
        double gc_per_game = static_cast<double>(p.goals_conceded) / p.appearances;

        double defensive_action_ev = calculate_defensive_action_ev(p);

        if (p.element_type == "GK" || p.element_type == "DEF") {
            double gc_penalty_per_game = (gc_per_game / 2.0);
            return (cs_per_game * 4.0) - gc_penalty_per_game + defensive_action_ev;
        } else if (p.element_type == "MID") {
            return (cs_per_game * 1.0) + defensive_action_ev;
        }

        return defensive_action_ev; 
    }

	private:
    double calculate_bonus_penalty_ev(const Player& p) const {
        if (p.appearances == 0) return 0.0;

        double bonus_per_game = static_cast<double>(p.bonus) / p.appearances;
        double yc_per_game = static_cast<double>(p.yellow_cards) / p.appearances;
        double rc_per_game = static_cast<double>(p.red_cards) / p.appearances;

        return bonus_per_game - (yc_per_game * 1.0) - (rc_per_game * 3.0);
    }

	public:
    void assign_ev() {
        for (auto& p : players) {
            if (p.appearances == 0) {
                p.ev = 0.0;
                continue;
            }

            double app_ev = calculate_appearance_ev(p);
            double attack_ev = calculate_attack_ev(p);
            double defense_ev = calculate_defense_ev(p);
            double bp_ev = calculate_bonus_penalty_ev(p);

            p.ev = app_ev + attack_ev + defense_ev + bp_ev;
        }
    }
};

