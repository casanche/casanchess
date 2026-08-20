#include <filesystem>
#include <string>

void Convert(std::string ifilename, std::string ofilename);

int main([[maybe_unused]] int argc, char** argv) {
    std::filesystem::path filepath = argv[1];
    std::string inputFile = filepath.string();
    std::string outputFile = filepath.replace_extension(".nn").string();
    Convert(inputFile, outputFile);
}
