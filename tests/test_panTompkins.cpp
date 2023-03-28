#include <fstream>
#include <nlohmann/json.hpp>

#include "../src/panTompkins.h"

int main() {
  std::ifstream in("../assets/ecg_data/assets/data.json");
  nlohmann::json j;
  in >> j;

  std::ofstream out("../assets/output.txt");

  init(125);

  for (auto point : j) {
    out << std::boolalpha << panTompkins(point["leadII"]) << std::endl;
  }
}
