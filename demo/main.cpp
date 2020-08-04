#include <Windows.h>

#include <overlay_helper.h>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: OverlayDemo.exe [Dest Program Path]" << std::endl;
        return -1;
    }

    std::string dest_program = argv[1];

    STARTUPINFOA info = {sizeof(info)};
    PROCESS_INFORMATION process_info;

    // Try to create the dest process
    std::cout << "Starting dest process.." << std::endl;
    if (!CreateProcessA(dest_program.c_str(), "", NULL, NULL, TRUE, 0, NULL, NULL, &info,
                       &process_info)) {
        std::cerr << "Unable to start dest program!" << std::endl;
        return -1;
    }

    // Wait for process to load
    Sleep(500);

    // Try to inject the overlay to the process
    std::cout << "Injecting to dest process.." << std::endl;
    if (!OverlayInjectToProcess(process_info.dwProcessId)) {
        std::cerr << "Unable to inject overlay to process!" << std::endl;
        return -1;
    }

    std::cout << "Done!" << std::endl;

    return 0;
}