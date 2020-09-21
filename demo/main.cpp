#include <Windows.h>
#include <overlay/helper.h>

#include <cxxopts.hpp>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  cxxopts::Options options("Overlay Demo",
                           "This program is used to demonstrate the overlay");

  options.add_options()("p,pid", "The pid of the process to inject the overlay",
                        cxxopts::value<DWORD>())(
      "e,executable",
      "The name of the executable of the process to inject the overlay",
      cxxopts::value<std::string>())("c,connect",
                                     "Connect to the target process")(
      "stats-log", "Log target process's stats (FPS and Frame time)")(
      "h,help", "Show this help");

  auto args = options.parse(argc, argv);

  STARTUPINFOA info = {sizeof(info)};
  PROCESS_INFORMATION process_info = {0};

  std::shared_ptr<ovhp::Client> client = nullptr;

  // Show help
  if (args.count("help") || !(args.count("pid") || args.count("executable"))) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if (args.count("executable")) {
    // Try to create the dest process
    std::cout << "Starting dest process.." << std::endl;
    if (!CreateProcessA(args["executable"].as<std::string>().c_str(), "", NULL,
                        NULL, TRUE, 0, NULL, NULL, &info, &process_info)) {
      std::cerr << "Unable to start dest program!" << std::endl;
      return -1;
    }

    // Wait for process to load
    Sleep(300);
  } else {
    process_info.hProcess =
        OpenProcess(SYNCHRONIZE, false, args["pid"].as<DWORD>());
  }

  // Try to inject the overlay to the process
  std::cout << "Injecting to dest process.." << std::endl;
  try {
    ovhp::InjectCoreToProcess(process_info.dwProcessId != 0
                                  ? process_info.dwProcessId
                                  : args["pid"].as<DWORD>());
  } catch (std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    std::cerr << "Unable to inject overlay to process!" << std::endl;
    return -1;
  }

  // Wait for overlay to initiate
#ifdef _WIN64
  Sleep(1000);
#else
  Sleep(2500);
#endif

  if (args.count("connect")) {
    try {
      // Connect to the client
      std::cout << "Connecting to overlay.." << std::endl;
      client = ovhp::CreateClient(process_info.dwProcessId != 0
                                      ? process_info.dwProcessId
                                      : args["pid"].as<DWORD>());
      client->Connect();
      std::cout << "Connected to dest process' overlay!" << std::endl
                << std::endl;

      if (args["stats-log"].as<bool>()) {
        client->SubscribeToEvent(
            ovhp::EventType::ApplicationStats,
            [](std::shared_ptr<ovhp::Event> event) {
              printf(
                  "\rApplication stats: Frame time: %f, FPS: %f",
                  std::static_pointer_cast<ovhp::ApplicationStatsEvent>(event)
                      ->frame_time,
                  std::static_pointer_cast<ovhp::ApplicationStatsEvent>(event)
                      ->fps);
            });
      }

      // Wait for process to exit
      WaitForSingleObject(process_info.hProcess, INFINITE);
    } catch (std::exception& ex) {
      std::cerr << "\33[2K\rAn error occurred: " << ex.what() << std::endl;
      return -1;
    }
  }

  std::cout << "\33[2K\rDone!" << std::endl;

  return 0;
}