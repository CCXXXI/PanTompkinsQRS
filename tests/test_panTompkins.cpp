#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <nlohmann/json.hpp>

#include "../src/panTompkins.h"

TEST_CASE("test_PanTompkinsQRS", "[PanTompkinsQRS]") {
  std::ifstream in("../assets/ecg_data/assets/data.json");
  auto points = nlohmann::json::parse(in);

  std::ifstream out("../assets/output.txt");

  init(125);

  for (auto point : points) {
    auto actual = panTompkins(point["leadII"]);

    bool expected;
    out >> std::boolalpha >> expected;

    REQUIRE(actual == expected);
  }

  // out should be empty now
  out.ignore(1, '\n');
  REQUIRE(out.get() == EOF);
}
