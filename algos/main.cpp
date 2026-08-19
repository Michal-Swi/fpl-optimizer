#include "optimizier.hpp"
#include <algorithm>
#include <utility>
#include <iostream>

bool comp(const std::pair<Player, bool> &p1, const std::pair<Player, bool> &p2) {
    return p1.first.element_type.at(0) > p2.first.element_type.at(0);
}

bool comp1(const Player &p1, const Player &p2) {
    return p1.element_type.at(0) > p2.element_type.at(0);
}

int main() {
    Optimizer o("../data/players_raw.csv");
    o.assign_ev();
    o.export_ev_to_csv();

    auto players = o.select_optimal_squad();
    auto players1 = o.select_optimal_starting_11();
    auto players2 = o.select_optimal_squad_cheap_backup_gk();

    std::sort(players.begin(), players.end(), comp1);
    std::sort(players1.begin(), players1.end(), comp);
    std::sort(players2.begin(), players2.end(), comp);

    std::cout << "Top 15" << std::endl;

    double used_budget = 0;
    for (const auto &p : players) {
        std::cout << "Name: " << p.first_name << ' ' << p.second_name << std::endl;
        std::cout << "Avg points per game: " << p.ev << std::endl;
        std::cout << "Position: " << p.element_type << std::endl;
        std::cout << "Cost: " << p.now_cost << std::endl << std::endl;

        used_budget += p.now_cost;
    }

    std::cout << "Used budget: " << used_budget << std::endl;

    std::cout << "=======================================" << std::endl;
    std::cout << "Top 11" << std::endl << std::endl;

    used_budget = 0;
    for (const auto &p : players1) {
        std::cout << "Name: " << p.first.first_name << ' ' << p.first.second_name << std::endl;
        std::cout << "Avg points per game: " << p.first.ev << std::endl;
        std::cout << "Position: " << p.first.element_type << std::endl;
        std::cout << "Cost: " << p.first.now_cost << std::endl;
        std::cout << "Starting: " << (p.second ? "Yes" : "No") << std::endl << std::endl;

        used_budget += p.first.now_cost;
    }

    std::cout << "Used budget: " << used_budget << std::endl;

    std::cout << "=======================================" << std::endl;
    std::cout << "Top 13 with cheap second goalkeeper" << std::endl << std::endl;

    used_budget = 0;
    for (const auto &p : players2) {
        std::cout << "Name: " << p.first.first_name << ' ' << p.first.second_name << std::endl;
        std::cout << "Avg points per game: " << p.first.ev << std::endl;
        std::cout << "Position: " << p.first.element_type << std::endl;
        std::cout << "Cost: " << p.first.now_cost << std::endl;
        std::cout << "Starting: " << (p.second ? "Yes" : "No") << std::endl << std::endl;

        used_budget += p.first.now_cost;
    }

    std::cout << "Used budget: " << used_budget << std::endl;

    return 0;
}

