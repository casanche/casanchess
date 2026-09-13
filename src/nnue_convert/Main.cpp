#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

bool Convert(std::string ifilename, std::string ofilename);

struct CliArgs {
    std::string inputFile;
    bool showHelp = false;
};

void PrintUsage() {
    std::cout << "Usage: nnue-convert <weights-file.txt>\n"
              << "Converts the text weights file to a .nn binary with the same name.\n"
              << "Example: nnue-convert network.txt  # creates network.nn\n";
}

bool ParseArgs(int argc, char** argv, CliArgs& argsOut) {
    bool invalidArgs = false;

    for(int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if(arg == "-h" || arg == "--help" || arg == "help") argsOut.showHelp = true;
        else if(argsOut.inputFile.empty()) argsOut.inputFile = argv[i];
        else invalidArgs = true;
    }

    if(invalidArgs || (argsOut.inputFile.empty() && !argsOut.showHelp)) {
        std::cerr << "Error: expected one text weights file.\n";
        return false;
    }

    return true;
}

int main(int argc, char** argv) {
    CliArgs args;
    if(!ParseArgs(argc, argv, args) || args.showHelp) {
        PrintUsage();
        return args.showHelp ? 0 : 1;
    }

    std::filesystem::path filepath = args.inputFile;
    std::string inputFile = filepath.string();
    std::string outputFile = filepath.replace_extension(".nn").string();
    return Convert(inputFile, outputFile) ? 0 : 1;
}
