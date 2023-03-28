#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>

#include "panTompkins.h"

int main() {
  std::ifstream in("../assets/ecg_data/assets/data.json");
  auto points = nlohmann::json::parse(in);

  std::ofstream out("../assets/output.txt");

  init(125);

  for (auto point : points) {
    out << std::boolalpha << panTompkins(point["leadII"]) << std::endl;
  }
}
