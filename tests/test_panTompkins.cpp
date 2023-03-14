#include <fstream>

#include "../src/panTompkins.h"

int main() {
  std::ifstream in("../assets/ecg_data/assets/lead II.txt");
  std::ofstream out("../assets/output.txt");

  init(125);

  for (float sample; in >> sample;) {
    out << std::boolalpha << panTompkins(sample) << std::endl;
  }
}
