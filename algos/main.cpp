#include "optimizier.hpp"

int main() {
	Optimizer o("../data/cleaned_players.csv"); 
	auto v = o.get_non_playing_players();

	int i = 1; 
	for (const auto &p : v) {
		std::cout << i << ": " << p.first_name << ' ' << p.second_name << ' ' << p.minutes << std::endl; 
		++i;
	}

	return 0; 
}

