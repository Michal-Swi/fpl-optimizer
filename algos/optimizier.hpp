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
    std::string element_type;
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
            throw std::runtime_error("Error: Could not open the file.");
        }

        std::string line;
        std::getline(file, line); 

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::stringstream ss(line); 
            std::string token;
            Player p;

            std::getline(ss, p.first_name, ',');
            std::getline(ss, p.second_name, ',');
            
            std::getline(ss, token, ','); p.team = std::stoi(token);
            std::getline(ss, p.element_type, ',');
            std::getline(ss, token, ','); p.now_cost = std::stoi(token);
            std::getline(ss, token, ','); p.minutes = std::stoi(token);
            std::getline(ss, token, ','); p.starts = std::stoi(token);
            std::getline(ss, token, ','); p.total_points = std::stoi(token);
            std::getline(ss, token, ','); p.points_per_game = std::stod(token);
            std::getline(ss, token, ','); p.goals_scored = std::stoi(token);
            std::getline(ss, token, ','); p.assists = std::stoi(token);
            std::getline(ss, token, ','); p.clean_sheets = std::stoi(token);
            std::getline(ss, token, ','); p.goals_conceded = std::stoi(token);
            std::getline(ss, token, ','); p.defensive_contribution_per_90 = std::stod(token);
            std::getline(ss, token, ','); p.bonus = std::stoi(token);
            std::getline(ss, token, ','); p.yellow_cards = std::stoi(token);
            std::getline(ss, token, ','); p.red_cards = std::stoi(token);

            if (p.points_per_game > 0.0) {
                p.appearances = static_cast<int>((p.total_points / p.points_per_game) + 0.5);
            } else {
                p.appearances = 0;
            }
            
            if (p.appearances > 0) {
                p.avg_minutes_per_game = static_cast<double>(p.minutes) / p.appearances;
            } else {
                p.avg_minutes_per_game = 0.0;
            }

            p.ev = 0.0;
            players.push_back(p);
        }
        
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

        for (const auto& p : players) {
            if (p.minutes > 450) {
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
        else goal_multiplier = 6; 

        double goals_per_game = static_cast<double>(p.goals_scored) / p.appearances;
        double assists_per_game = static_cast<double>(p.assists) / p.appearances;

        return (goals_per_game * goal_multiplier) + (assists_per_game * 3.0);
    }

	private:
    double calculate_defensive_action_ev(const Player& p) const {
        double rate = p.defensive_contribution_per_90;
        if (rate <= 0.0) return 0.0;

        int threshold = (p.element_type == "DEF" || p.element_type == "GK") ? 10 : 12;
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

	private:
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

