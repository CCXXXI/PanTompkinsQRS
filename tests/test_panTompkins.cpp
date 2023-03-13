#include "../src/panTompkins.h"

int main() {
  init("assets/ecg_data/assets/lead II.txt", "out.txt");
  panTompkins();
}
