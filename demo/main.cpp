#include <Windows.h>
#include <overlay/helper.h>
#include <tlhelp32.h>

#include <cxxopts.hpp>
#include <iostream>
#include <string>

DWORD FindProcessByName(std::string name) {
  DWORD pid = 0;

  HANDLE hProcSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if (INVALID_HANDLE_VALUE != hProcSnapshot) {
    PROCESSENTRY32 procEntry = {0};
    procEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hProcSnapshot, &procEntry)) {
      do {
        HANDLE hModSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,
                                                       procEntry.th32ProcessID);

        if (INVALID_HANDLE_VALUE != hModSnapshot) {
          MODULEENTRY32 modEntry = {0};
          modEntry.dwSize = sizeof(MODULEENTRY32);

          if (Module32First(hModSnapshot, &modEntry)) {
            std::string path((const char*)modEntry.szExePath);

            if (name == path.substr(path.rfind("\\") + 1)) {
              pid = procEntry.th32ProcessID;

              CloseHandle(hModSnapshot);
              break;
            }
          }

          CloseHandle(hModSnapshot);
        }
      } while (Process32Next(hProcSnapshot, &procEntry));
    }
    CloseHandle(hProcSnapshot);
  }

  return pid;
}

int main(int argc, char** argv) {
  cxxopts::Options options("Overlay Demo",
                           "This program is used to demonstrate the overlay");

  options.add_options()("p,pid", "The pid of the process to inject the overlay",
                        cxxopts::value<DWORD>())  // -p
      ("e,executable",
       "The name of the executable of the process to inject the overlay",
       cxxopts::value<std::string>())  // -e
      ("f,find-executable-instance",
       "Use an open instance of the executable if exists")  // -f
      ("c,connect", "Connect to the target process")        // -c
      ("r,retry-connect", "Retry to connect if connection fails",
       cxxopts::value<bool>()->default_value("true"))  // -r
      ("max-connect-retries", "Maximum connection retries",
       cxxopts::value<unsigned int>()->default_value(
           "5"))  // --max-connect-retries 5
      ("stats-log",
       "Log target process's stats (FPS and Frame time)")  // --stats-log
      ("h,help", "Show this help")                         // -h
      ;

  auto args = options.parse(argc, argv);

  STARTUPINFOA info = {sizeof(info)};
  PROCESS_INFORMATION process_info = {0};

  std::shared_ptr<ovhp::Client> client = nullptr;
  unsigned int retries = 0;

  // Show help
  if (args.count("help") || !(args.count("pid") || args.count("executable"))) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if (args.count("executable")) {
    if (args["find-executable-instance"].as<bool>() &&
        (process_info.dwProcessId =
             FindProcessByName(args["executable"].as<std::string>())) != 0) {
      process_info.hProcess =
          OpenProcess(SYNCHRONIZE, false, process_info.dwProcessId);
    }

    if (process_info.hProcess == 0) {
      // Try to create the dest process
      std::cout << "Starting dest process.." << std::endl;
      if (!CreateProcessA(args["executable"].as<std::string>().c_str(), "",
                          NULL, NULL, TRUE, 0, NULL, NULL, &info,
                          &process_info)) {
        std::cerr << "Unable to start dest program!" << std::endl;
        return -1;
      }

      // Wait for process to load
      Sleep(300);
    }
  } else {
    process_info.dwProcessId = args["pid"].as<DWORD>();
    process_info.hProcess =
        OpenProcess(SYNCHRONIZE, false, args["pid"].as<DWORD>());
    if (!process_info.hProcess) {
      std::cerr << "The wanted process wasn't found!" << std::endl;
      return -1;
    }
  }

  // Try to inject the overlay to the process
  std::cout << "Injecting to dest process.." << std::endl;
  try {
    ovhp::InjectCoreToProcess(process_info.dwProcessId);
  } catch (std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    std::cerr << "Unable to inject overlay to process!" << std::endl;
    return -1;
  }

  // Wait for overlay to initiate
  Sleep(1000);

  if (args["connect"].as<bool>()) {
    try {
      // Connect to the client
      std::cout << "Connecting to overlay.." << std::endl;
      client = ovhp::CreateClient(process_info.dwProcessId);

      do {
        if (retries > 0) {
          std::cout << "Connecting to overlay (Retry " << retries << "/"
                    << args["max-connect-retries"].as<unsigned int>() << ").."
                    << std::endl;
        }

        try {
          client->Connect();
          break;
        } catch (...) {
          if (!(args["retry-connect"].as<bool>() &&
                retries++ < args["max-connect-retries"].as<unsigned int>())) {
            throw;
          }

          Sleep(1000);
        }
      } while (true);

      std::cout << "Connected to dest process' overlay!" << std::endl
                << std::endl;

      if (args["stats-log"].as<bool>()) {
        client->SubscribeToEvent(
            ovhp::EventType::ApplicationStats,
            [](std::shared_ptr<ovhp::Event> event) {
              printf(
                  "\rApplication stats: Window Size: %zdx%zd, Fullscreen: %s, "
                  "Frame time: %f, FPS: %f",
                  std::static_pointer_cast<ovhp::ApplicationStatsEvent>(event)
                      ->width,
                  std::static_pointer_cast<ovhp::ApplicationStatsEvent>(event)
                      ->height,
                  std::static_pointer_cast<ovhp::ApplicationStatsEvent>(event)
                          ->fullscreen
                      ? "True"
                      : "False",
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