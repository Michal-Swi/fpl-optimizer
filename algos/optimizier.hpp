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
#include <glpk.h>

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
    std::map<std::string, int> player_index_map;

public:
    Optimizer(const std::string& current_filepath, const std::string& historical_filepath) {
        load_current_data(current_filepath);
        load_historical_data(historical_filepath);
        prune_non_playing_players();
        assign_ev();
    }

    const std::vector<Player>& get_players() const {
        return players;
    }

    void export_ev_to_csv(const std::string& filename = "../data/player_ev_data.csv") const {
        std::ofstream file(filename);
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
    void load_current_data(const std::string& filepath) {
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
                if (it == header_map.end()) throw std::runtime_error("Missing column: " + key);
                size_t idx = static_cast<size_t>(it->second);
                if (idx >= row.size()) throw std::runtime_error("Too few fields for column: " + key);
                return row[idx];
            };

            Player p = {};
            try {
                p.first_name    = get_val("first_name");
                p.second_name   = get_val("second_name");
                p.team          = std::stoi(get_val("team"));
                p.element_type  = parse_element_type(get_val("element_type"));
                p.now_cost      = std::stoi(get_val("now_cost"));
                p.ev            = 0.0;
                
                std::string key = p.first_name + " " + p.second_name;
                player_index_map[key] = players.size();
                players.push_back(p);
            } catch (const std::exception& e) {
                ++rows_skipped;
            }
        }
        std::cout << "Loaded " << players.size() << " current players from " << filepath << "\n";
    }

    void load_historical_data(const std::string& filepath) {
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
                if (it == header_map.end()) throw std::runtime_error("Missing column: " + key);
                size_t idx = static_cast<size_t>(it->second);
                if (idx >= row.size()) throw std::runtime_error("Too few fields for column: " + key);
                return row[idx];
            };

            try {
                std::string key = get_val("first_name") + " " + get_val("second_name");
                auto it = player_index_map.find(key);
                if (it != player_index_map.end()) {
                    Player& p = players[it->second];
                    p.minutes       = std::stoi(get_val("minutes"));
                    p.starts        = std::stoi(get_val("starts"));
                    p.total_points  = std::stoi(get_val("total_points"));
                    p.points_per_game = std::stod(get_val("points_per_game"));
                    p.goals_scored  = std::stoi(get_val("goals_scored"));
                    p.assists       = std::stoi(get_val("assists"));
                    p.clean_sheets  = std::stoi(get_val("clean_sheets"));
                    p.goals_conceded = std::stoi(get_val("goals_conceded"));
                    
                    try {
                        p.defensive_contribution_per_90 = std::stod(get_val("defensive_contribution_per_90"));
                    } catch (...) {
                        p.defensive_contribution_per_90 = 0.0;
                    }

                    p.bonus         = std::stoi(get_val("bonus"));
                    p.yellow_cards  = std::stoi(get_val("yellow_cards"));
                    p.red_cards     = std::stoi(get_val("red_cards"));

                    if (p.points_per_game > 0.0) {
                        p.appearances = static_cast<int>((p.total_points / p.points_per_game) + 0.5);
                    } else {
                        p.appearances = 0;
                    }

                    p.avg_minutes_per_game = (p.appearances > 0)
                        ? static_cast<double>(p.minutes) / p.appearances
                        : 0.0;
                }
            } catch (...) {
                continue;
            }
        }
    }

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
            if (it != pos_map.end()) {
                return it->second;
            }
        } catch (...) {}

        if (token == "GK" || token == "DEF" || token == "MID" || token == "FWD") {
            return token;
        }
        return "UNKNOWN";
    }

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

        const int MEANINGFUL_MINUTES_THRESHOLD = 450;

        for (const auto& p : players) {
            if (p.minutes > MEANINGFUL_MINUTES_THRESHOLD) {
                active_players.push_back(p);
            } else {
                std::pair<int, std::string> key = {p.team, p.element_type};
                if (p.now_cost < min_costs[key] && p.now_cost > 0) {
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

    double calculate_appearance_ev(const Player& p) const {
        if (p.appearances == 0) return 0.0;
        double p_60_plus = static_cast<double>(p.starts) / p.appearances;
        double p_1_to_59 = static_cast<double>(p.appearances - p.starts) / p.appearances;
        return (p_60_plus * 2.0) + (p_1_to_59 * 1.0);
    }

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

    double calculate_defensive_action_ev(const Player& p) const {
        if (p.element_type == "GK") return 0.0;
        double rate = p.defensive_contribution_per_90;
        if (rate <= 0.0) return 0.0;
        int threshold = (p.element_type == "DEF") ? 10 : 12;
        double p_hit = 1.0 - poisson_cdf(threshold - 1, rate);
        return p_hit * 2.0;
    }

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
            if (p.appearances < 3) {
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

    std::vector<Player> select_optimal_squad(double budget = 1000.0) {
        glp_prob *mip = glp_create_prob();
        glp_set_prob_name(mip, "FPL_Optimizer_Full_15");
        glp_set_obj_dir(mip, GLP_MAX);

        int n = players.size();
        glp_add_cols(mip, n);

        for (int i = 0; i < n; ++i) {
            glp_set_col_bnds(mip, i + 1, GLP_DB, 0.0, 1.0);
            glp_set_col_kind(mip, i + 1, GLP_BV);
            glp_set_obj_coef(mip, i + 1, players[i].ev);
        }

        std::map<int, int> team_to_row;
        int current_row = 6;
        for (const auto& p : players) {
            if (team_to_row.find(p.team) == team_to_row.end()) {
                team_to_row[p.team] = current_row++;
            }
        }

        int total_rows = current_row - 1;
        glp_add_rows(mip, total_rows);

        glp_set_row_bnds(mip, 1, GLP_UP, 0.0, budget);
        glp_set_row_bnds(mip, 2, GLP_FX, 2.0, 2.0);
        glp_set_row_bnds(mip, 3, GLP_FX, 5.0, 5.0);
        glp_set_row_bnds(mip, 4, GLP_FX, 5.0, 5.0);
        glp_set_row_bnds(mip, 5, GLP_FX, 3.0, 3.0);

        for (int r = 6; r <= total_rows; ++r) {
            glp_set_row_bnds(mip, r, GLP_UP, 0.0, 3.0);
        }

        std::vector<int> ia(1, 0), ja(1, 0);
        std::vector<double> ar(1, 0.0);

        auto add_coef = [&](int row, int col, double val) {
            ia.push_back(row);
            ja.push_back(col);
            ar.push_back(val);
        };

        for (int i = 0; i < n; ++i) {
            int col = i + 1;
            
            add_coef(1, col, players[i].now_cost);

            if (players[i].element_type == "GK" || players[i].element_type == "Goalkeeper") add_coef(2, col, 1.0);
            else if (players[i].element_type == "DEF" || players[i].element_type == "Defender") add_coef(3, col, 1.0);
            else if (players[i].element_type == "MID" || players[i].element_type == "Midfielder") add_coef(4, col, 1.0);
            else if (players[i].element_type == "FWD" || players[i].element_type == "Forward") add_coef(5, col, 1.0);

            add_coef(team_to_row[players[i].team], col, 1.0);
        }

        glp_load_matrix(mip, ia.size() - 1, ia.data(), ja.data(), ar.data());

        glp_iocp parm;
        glp_init_iocp(&parm);
        parm.presolve = GLP_ON;
        parm.msg_lev = GLP_MSG_OFF;

        glp_intopt(mip, &parm);

        std::vector<Player> optimal_squad;
        if (glp_mip_status(mip) == GLP_OPT || glp_mip_status(mip) == GLP_FEAS) {
            for (int i = 0; i < n; ++i) {
                if (glp_mip_col_val(mip, i + 1) > 0.5) {
                    optimal_squad.push_back(players[i]);
                }
            }
        }

        glp_delete_prob(mip);
        return optimal_squad;
    }

    std::vector<std::pair<Player, bool>> select_optimal_starting_11(double budget = 1000.0) {
        glp_prob *mip = glp_create_prob();
        glp_set_prob_name(mip, "FPL_Optimizer_Starting_11");
        glp_set_obj_dir(mip, GLP_MAX);

        int n = players.size();
        glp_add_cols(mip, 2 * n);

        for (int i = 0; i < n; ++i) {
            glp_set_col_bnds(mip, i + 1, GLP_DB, 0.0, 1.0);
            glp_set_col_kind(mip, i + 1, GLP_BV);
            glp_set_obj_coef(mip, i + 1, 0.0);

            glp_set_col_bnds(mip, n + i + 1, GLP_DB, 0.0, 1.0);
            glp_set_col_kind(mip, n + i + 1, GLP_BV);
            glp_set_obj_coef(mip, n + i + 1, players[i].ev);
        }

        std::map<int, int> team_to_row;
        int current_row = 11;
        for (const auto& p : players) {
            if (team_to_row.find(p.team) == team_to_row.end()) {
                team_to_row[p.team] = current_row++;
            }
        }

        int start_of_link_rows = current_row;
        int total_rows = start_of_link_rows + n - 1;
        
        glp_add_rows(mip, total_rows);

        glp_set_row_bnds(mip, 1, GLP_UP, 0.0, budget);
        glp_set_row_bnds(mip, 2, GLP_FX, 2.0, 2.0);
        glp_set_row_bnds(mip, 3, GLP_FX, 5.0, 5.0);
        glp_set_row_bnds(mip, 4, GLP_FX, 5.0, 5.0);
        glp_set_row_bnds(mip, 5, GLP_FX, 3.0, 3.0);
        glp_set_row_bnds(mip, 6, GLP_FX, 1.0, 1.0);
        glp_set_row_bnds(mip, 7, GLP_DB, 3.0, 5.0);
        glp_set_row_bnds(mip, 8, GLP_DB, 2.0, 5.0);
        glp_set_row_bnds(mip, 9, GLP_DB, 1.0, 3.0);
        glp_set_row_bnds(mip, 10, GLP_FX, 11.0, 11.0);

        for (int r = 11; r < start_of_link_rows; ++r) {
            glp_set_row_bnds(mip, r, GLP_UP, 0.0, 3.0);
        }

        for (int i = 0; i < n; ++i) {
            glp_set_row_bnds(mip, start_of_link_rows + i, GLP_UP, 0.0, 0.0);
        }

        std::vector<int> ia(1, 0), ja(1, 0);
        std::vector<double> ar(1, 0.0);

        auto add_coef = [&](int row, int col, double val) {
            ia.push_back(row);
            ja.push_back(col);
            ar.push_back(val);
        };

        for (int i = 0; i < n; ++i) {
            int x_col = i + 1;
            int y_col = n + i + 1;
            
            add_coef(1, x_col, players[i].now_cost);

            bool is_gk = (players[i].element_type == "GK" || players[i].element_type == "Goalkeeper");
            bool is_def = (players[i].element_type == "DEF" || players[i].element_type == "Defender");
            bool is_mid = (players[i].element_type == "MID" || players[i].element_type == "Midfielder");
            bool is_fwd = (players[i].element_type == "FWD" || players[i].element_type == "Forward");

            if (is_gk) add_coef(2, x_col, 1.0);
            else if (is_def) add_coef(3, x_col, 1.0);
            else if (is_mid) add_coef(4, x_col, 1.0);
            else if (is_fwd) add_coef(5, x_col, 1.0);

            if (is_gk) add_coef(6, y_col, 1.0);
            else if (is_def) add_coef(7, y_col, 1.0);
            else if (is_mid) add_coef(8, y_col, 1.0);
            else if (is_fwd) add_coef(9, y_col, 1.0);

            add_coef(10, y_col, 1.0);
            add_coef(team_to_row[players[i].team], x_col, 1.0);
            add_coef(start_of_link_rows + i, y_col, 1.0);
            add_coef(start_of_link_rows + i, x_col, -1.0);
        }

        glp_load_matrix(mip, ia.size() - 1, ia.data(), ja.data(), ar.data());

        glp_iocp parm;
        glp_init_iocp(&parm);
        parm.presolve = GLP_ON;
        parm.msg_lev = GLP_MSG_OFF;

        glp_intopt(mip, &parm);

        std::vector<std::pair<Player, bool>> optimal_squad;
        if (glp_mip_status(mip) == GLP_OPT || glp_mip_status(mip) == GLP_FEAS) {
            for (int i = 0; i < n; ++i) {
                if (glp_mip_col_val(mip, i + 1) > 0.5) {
                    bool is_starting = glp_mip_col_val(mip, n + i + 1) > 0.5;
                    optimal_squad.push_back({players[i], is_starting});
                }
            }
        }

        glp_delete_prob(mip);
        return optimal_squad;
    }

    std::vector<std::pair<Player, bool>> select_optimal_squad_cheap_backup_gk(double budget = 1000.0) {
        glp_prob *mip = glp_create_prob();
        glp_set_prob_name(mip, "FPL_Optimizer_Cheap_Backup_GK");
        glp_set_obj_dir(mip, GLP_MAX);

        int n = players.size();
        glp_add_cols(mip, 2 * n);

        for (int i = 0; i < n; ++i) {
            bool is_gk = (players[i].element_type == "GK" || players[i].element_type == "Goalkeeper");
            
            glp_set_col_bnds(mip, i + 1, GLP_DB, 0.0, 1.0);
            glp_set_col_kind(mip, i + 1, GLP_BV);
            
            glp_set_col_bnds(mip, n + i + 1, GLP_DB, 0.0, 1.0);
            glp_set_col_kind(mip, n + i + 1, GLP_BV);
            
            if (is_gk) {
                glp_set_obj_coef(mip, i + 1, -0.00001 * players[i].now_cost);
                glp_set_obj_coef(mip, n + i + 1, players[i].ev);
            } else {
                glp_set_obj_coef(mip, i + 1, players[i].ev);
                glp_set_obj_coef(mip, n + i + 1, 0.0);
            }
        }

        std::map<int, int> team_to_row;
        int current_row = 7;
        for (const auto& p : players) {
            if (team_to_row.find(p.team) == team_to_row.end()) {
                team_to_row[p.team] = current_row++;
            }
        }

        int start_of_link_rows = current_row;
        int total_rows = start_of_link_rows + n - 1;
        
        glp_add_rows(mip, total_rows);

        glp_set_row_bnds(mip, 1, GLP_UP, 0.0, budget);
        glp_set_row_bnds(mip, 2, GLP_FX, 2.0, 2.0);
        glp_set_row_bnds(mip, 3, GLP_FX, 5.0, 5.0);
        glp_set_row_bnds(mip, 4, GLP_FX, 5.0, 5.0);
        glp_set_row_bnds(mip, 5, GLP_FX, 3.0, 3.0);
        glp_set_row_bnds(mip, 6, GLP_FX, 1.0, 1.0);

        for (int r = 7; r < start_of_link_rows; ++r) {
            glp_set_row_bnds(mip, r, GLP_UP, 0.0, 3.0);
        }

        for (int i = 0; i < n; ++i) {
            glp_set_row_bnds(mip, start_of_link_rows + i, GLP_UP, 0.0, 0.0);
        }

        std::vector<int> ia(1, 0), ja(1, 0);
        std::vector<double> ar(1, 0.0);

        auto add_coef = [&](int row, int col, double val) {
            ia.push_back(row);
            ja.push_back(col);
            ar.push_back(val);
        };

        for (int i = 0; i < n; ++i) {
            int x_col = i + 1;
            int y_col = n + i + 1;
            
            add_coef(1, x_col, players[i].now_cost);

            bool is_gk = (players[i].element_type == "GK" || players[i].element_type == "Goalkeeper");
            bool is_def = (players[i].element_type == "DEF" || players[i].element_type == "Defender");
            bool is_mid = (players[i].element_type == "MID" || players[i].element_type == "Midfielder");
            bool is_fwd = (players[i].element_type == "FWD" || players[i].element_type == "Forward");

            if (is_gk) {
                add_coef(2, x_col, 1.0);
                add_coef(6, y_col, 1.0);
            }
            else if (is_def) add_coef(3, x_col, 1.0);
            else if (is_mid) add_coef(4, x_col, 1.0);
            else if (is_fwd) add_coef(5, x_col, 1.0);

            add_coef(team_to_row[players[i].team], x_col, 1.0);

            add_coef(start_of_link_rows + i, y_col, 1.0);
            add_coef(start_of_link_rows + i, x_col, -1.0);
        }

        glp_load_matrix(mip, ia.size() - 1, ia.data(), ja.data(), ar.data());

        glp_iocp parm;
        glp_init_iocp(&parm);
        parm.presolve = GLP_ON;
        parm.msg_lev = GLP_MSG_OFF;

        glp_intopt(mip, &parm);

        std::vector<std::pair<Player, bool>> optimal_squad;
        if (glp_mip_status(mip) == GLP_OPT || glp_mip_status(mip) == GLP_FEAS) {
            for (int i = 0; i < n; ++i) {
                if (glp_mip_col_val(mip, i + 1) > 0.5) {
                    bool is_starting_or_outfielder = true;
                    if (players[i].element_type == "GK" || players[i].element_type == "Goalkeeper") {
                        is_starting_or_outfielder = glp_mip_col_val(mip, n + i + 1) > 0.5;
                    }
                    optimal_squad.push_back({players[i], is_starting_or_outfielder});
                }
            }
        }

        glp_delete_prob(mip);

        return optimal_squad;
    }

	public:
	void export_starting_11_to_csv(const std::vector<std::pair<Player, bool>> &squad, const std::string &filename) const {
        std::ofstream file(filename);
        file << "name,ev\n";
        
        for (const auto& p : squad) {
            if (p.second) {
                file << p.first.first_name << " " << p.first.second_name << "," << p.first.ev << "\n";
            }
        }
        
        file.close();
    }
};

