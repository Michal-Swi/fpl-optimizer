#include <fstream>
#include <sstream>
#include <vector>

struct Player {
    int id;
    std::string first_name;
    std::string second_name;
    int team;
    int element_type;
    int now_cost;
    double expected_goals_per_90;
    double expected_assists_per_90;
    double expected_goals_conceded_per_90;
    double starts_per_90;
    double ep_next;
    int total_points;
	int minutes; 

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
            std::stringstream ss(line); 
            std::string token;
            Player p;

            std::getline(ss, token, ';');
            p.id = std::stoi(token);

            std::getline(ss, p.first_name, ';');
            std::getline(ss, p.second_name, ';');

            std::getline(ss, token, ';');
            p.team = std::stoi(token);
            
            std::getline(ss, token, ';');
            p.element_type = std::stoi(token);

            std::getline(ss, token, ';');
            p.now_cost = std::stoi(token);

            std::getline(ss, token, ';');
            p.expected_goals_per_90 = std::stod(token);

            std::getline(ss, token, ';');
            p.expected_assists_per_90 = std::stod(token);

            std::getline(ss, token, ';');
            p.expected_goals_conceded_per_90 = std::stod(token);

            std::getline(ss, token, ';');
            p.starts_per_90 = std::stod(token);

            std::getline(ss, token, ';');
            p.ep_next = std::stod(token);

            std::getline(ss, token, ';');
            p.total_points = std::stoi(token);

            std::getline(ss, token, ';');
            p.= std::stoi(token);

            players.push_back(p);
        }
    }

	public:
    const std::vector<Player> get_players() const {
        return players;
    }

	private:
	double calculate_goalkeeper_ev(const Player &p) {
		double ev = 0; 
	}

	private:
	void assign_ev() {
		for (const auto player : players) {

		}
	}

	public:
	std::vector<Player> get_solution_players() {

	}
};

