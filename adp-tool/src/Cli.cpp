#include "Adp.h"
#include "Model/Log.h"
#include "Model/Device.h"
#include "Model/DeviceServer.h"
#include "Model/Firmware.h"

#include <thread>
#include <chrono>
#include <memory>
#include <fstream>
#include <exception>
#include "fmt/core.h"
#include "argparse/argparse.hpp"

using namespace std;
using namespace adp;

static const char* TOOL_NAME = "ADP Cli";

bool adp_connect()
{
    Device::Init();
    Device::Update();

    if(!Device::Pad()) {
        std::cout << "No adp device found" << std::endl;
        return false;
    }

    return true;
}

DeviceProfileGroups profile_groups_from_string(std::string g)
{
    DeviceProfileGroups groups = 0;

    size_t pos = 0;
    std::string token = g;
    do {
        if(token == "SENSITIVITY") {
            groups |= DPG_SENSITIVITY;
        }
        else if(token == "MAPPING") {
            groups |= DPG_MAPPING;
        }
        else if(token == "DEVICE") {
            groups |= DPG_DEVICE;
        }
        else if(token == "ALL") {
            groups |= DGP_ALL;
        }

        token = g.substr(0, pos);
        g.erase(0, pos + 1);
    } while ((pos = g.find(",")) != std::string::npos);

    return groups;
}

void adp_device_command()
{
    adp_connect();

    bool printing = false;
    for(int i=0; i<Log::NumMessages(); i++) {
        auto line = Log::Message(i);
        if(line.find("ConnectionManager") != std::string::npos) {
            printing = true;
        }

        if(printing) {
            cout << line << std::endl;
        }
    }
}

void adp_profile_save(argparse::ArgumentParser& args)
{
    if(!adp_connect()) {
        return;
    }

    DeviceProfileGroups groups = profile_groups_from_string(args.get<std::string>("-groups"));
    json j;
    Device::SaveProfile(j, groups);

    if(args["-print"] == true) {
        cout << j.dump(4);
    }

    if(args.is_used("-file")) {
        std::string file = args.get<std::string>("-file");

        std::ofstream output_stream(file);
        if (!output_stream)
            throw std::runtime_error("Could not open file");

        output_stream << j.dump(4);
		output_stream.close();
    }
}

void adp_profile_load(argparse::ArgumentParser& args)
{
    if(!adp_connect()) {
        return;
    }

    DeviceProfileGroups groups = profile_groups_from_string(args.get<std::string>("-groups"));

    if(args.is_used("-json")) {
        std::string jsonString = args.get<std::string>("-json");
        json j = json::parse(jsonString);

        Device::LoadProfile(j, groups);
    }
    else if(args.is_used("-file")) {
        ifstream fileStream;
        fileStream.open(args.get<std::string>("-file"));

        if (!fileStream.is_open())
            throw std::runtime_error("Could not open file");

        json j;

		fileStream >> j;
		fileStream.close();

        Device::LoadProfile(j, groups);
    }
    else {
        throw std::runtime_error("No profile source provided");
    }
}

void adp_firmware_flash(argparse::ArgumentParser& args)
{
    FirmwareUploader uploader;
    uploader.SetIgnoreBoardType(true);

    uploader.SetPort(args.get<std::string>("-port"));
    std::string type = args.get<std::string>("-type");
    std::string fileName = args.get<std::string>("-file");

    FlashResult result;

    if(type == "AVR") {
        result = uploader.WriteFirmwareAvr(fileName);
    }
    else if(type == "ESP") {
        result = uploader.WriteFirmwareEsp(fileName);
    }
    else {
        throw std::runtime_error("Invalid device type");
    }

}

void adp_firmware_update(argparse::ArgumentParser& args)
{

}

#ifdef DEVICE_SERVER_ENABLED
void adp_server_run(argparse::ArgumentParser& args)
{
    if(!adp_connect()) {
        return;
    }

    Device::ServerStart();
    while(true) {
        Device::Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
#endif

int main(int argc, char** argv)
{
    Log::Init();
    auto versionString = fmt::format("{} {}.{}", TOOL_NAME, ADP_VERSION_MAJOR, ADP_VERSION_MINOR);
    std::cout << versionString << std::endl;

    argparse::ArgumentParser program(TOOL_NAME);
    
    argparse::ArgumentParser device_command("device");
    device_command.add_description("Prints information about the connected device");
    program.add_subparser(device_command);

    argparse::ArgumentParser profile_save_command("profile:save");
    profile_save_command.add_description("Save device settings to a profile json");
    profile_save_command.add_argument("-file")
        .help(".json file location");
    profile_save_command.add_argument("-print")
        .help("print profile json to the console")
        .implicit_value(true)
        .default_value(false);
    profile_save_command.add_argument("-groups")
        .help("profile groups to save (split with comma's) (SENSITIVITY, MAPPING, DEVICE, LIGHTS, ALL)")
        .default_value(std::string{"ALL"});
    program.add_subparser(profile_save_command);

    argparse::ArgumentParser profile_load_command("profile:load");
    profile_load_command.add_description("Load device settings from a profile json");
    profile_load_command.add_argument("-file")
        .help(".json file location");
    profile_load_command.add_argument("-json")
        .help("profile json string");
    profile_load_command.add_argument("-groups")
        .help("profile groups to load (split with comma's) (SENSITIVITY, MAPPING, DEVICE, LIGHTS, ALL)")
        .default_value(std::string{"ALL"});
    program.add_subparser(profile_load_command);

    argparse::ArgumentParser firmware_flash_command("firmware:flash");
    firmware_flash_command.add_description("Flash firmware to a device");
    firmware_flash_command.add_argument("-file")
        .help(".hex/.bin file location");
    firmware_flash_command.add_argument("-type")
        .help("device type (AVR, ESP)");
    firmware_flash_command.add_argument("-port")
        .help("serial port to use");
    program.add_subparser(firmware_flash_command);

    argparse::ArgumentParser firmware_update_command("firmware:update");
    firmware_update_command.add_description("Update the firmware of the connected device");
    firmware_update_command.add_argument("-file")
        .help(".hex/.bin file location");
    program.add_subparser(firmware_update_command);

#ifdef DEVICE_SERVER_ENABLED
    argparse::ArgumentParser server_run_command("server:run");
    server_run_command.add_description("Run the adp device server");
    program.add_subparser(server_run_command);
#endif

     try {
        program.parse_args(argc, argv);

        if(program.is_subcommand_used("device")) {
            adp_device_command();
        } else if(program.is_subcommand_used("profile:save")) {
            adp_profile_save(profile_save_command);
        } else if(program.is_subcommand_used("profile:load")) {
            adp_profile_load(profile_load_command);
#ifdef DEVICE_SERVER_ENABLED
        } else if(program.is_subcommand_used("server:run")) {
            adp_server_run(server_run_command);
#endif
        } else if(program.is_subcommand_used("firmware:flash")) {
            adp_firmware_flash(firmware_flash_command);
        } else if(program.is_subcommand_used("firmware:update")) {
            adp_firmware_update(firmware_update_command);
        } else {
            throw std::runtime_error("Subcommand required");
        }
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    return 0;
}