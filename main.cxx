#include <fstream>
#include <iostream>

class PersistentCounter {
  public:
    PersistentCounter(const char *filename) : filename(filename) {
        std::ifstream file(filename);
        if (file.is_open()) {
            file >> counter;
        }
    }

    ~PersistentCounter() {
        std::ofstream file(filename);

        if (file.is_open()) {
            file << counter << std::endl;
        }
    }

    int postIncrement() { return counter++; }

  private:
    std::string filename;
    int counter = 0;
};

int main() {
    static PersistentCounter counter("played.txt");

    std::cout << "Played " << counter.postIncrement() << " times." << std::endl;

    return 0;
}