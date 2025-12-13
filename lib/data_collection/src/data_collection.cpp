#include "data_collection/data_colection.hpp"

#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>

ReadEvent::ReadEvent(size_t read_size, std::chrono::duration<long, std::micro> time)
    : read_size { read_size }
    , time { time }
{
}
std::string ReadEvent::prettyPrint() const
{
    std::stringstream ss;
    ss << "Read " << read_size << " bytes in " << time.count() << " microseconds";
    return ss.str();
}
std::string ReadEvent::toCsv() const
{
    std::stringstream ss;
    ss << read_size << ", " << time.count();
    return ss.str();
}
std::string ReadEvent::fileName() const { return "read"; }
WriteEvent::WriteEvent(size_t write_size, std::chrono::duration<long, std::micro> time)
    : write_size { write_size }
    , time { time }
{
}
std::string WriteEvent::prettyPrint() const
{
    std::stringstream ss;
    ss << "Written " << write_size << " bytes in " << time.count() << " microseconds";
    return ss.str();
}
std::string WriteEvent::toCsv() const
{
    std::stringstream ss;
    ss << write_size << ", " << time.count();
    return ss.str();
}
std::string WriteEvent::fileName() const { return "write"; }
BitFlipEvent::BitFlipEvent(size_t byte_index)
    : byte_index(byte_index)
{
}
std::string BitFlipEvent::prettyPrint() const { return "Oh no! A bit has flipped!"; }
std::string BitFlipEvent::toCsv() const
{
    std::stringstream ss;
    ss << byte_index;
    return ss.str();
}
std::string BitFlipEvent::fileName() const { return "flip"; }
ErrorCorrectionEvent::ErrorCorrectionEvent(std::string ecc_type, block_index_t block_index)
    : ecc_type { ecc_type }
    , block_index { block_index }
{
}
std::string ErrorCorrectionEvent::prettyPrint() const
{
    std::stringstream ss;
    ss << "[" << ecc_type << "] Error corrected in block " << block_index;
    return ss.str();
}
std::string ErrorCorrectionEvent::toCsv() const
{
    std::stringstream ss;
    ss << ecc_type << "," << block_index;
    return ss.str();
}
std::string ErrorCorrectionEvent::fileName() const { return "correction"; }

ErrorDetectionEvent::ErrorDetectionEvent(std::string ecc_type, block_index_t block_index)
    : ecc_type { ecc_type }
    , block_index { block_index }
{
}
std::string ErrorDetectionEvent::prettyPrint() const
{
    std::stringstream ss;
    ss << "[" << ecc_type << "] Error detected in block " << block_index;
    return ss.str();
}
std::string ErrorDetectionEvent::toCsv() const
{
    std::stringstream ss;
    ss << ecc_type << ", " << block_index;
    return ss.str();
}
std::string ErrorDetectionEvent::fileName() const { return "detection"; }
Logger::Logger(const LogLevel log_level, const std::string& log_folder_path)
    : _log_level(log_level)
    , _log_folder_path(log_folder_path)
{
    _files["read"] = std::ofstream(_log_folder_path + "/read.csv", std::ios::out);
    _files["write"] = std::ofstream(_log_folder_path + "/write.csv");
    _files["flip"] = std::ofstream(_log_folder_path + "/flip.csv");
    _files["correction"] = std::ofstream(_log_folder_path + "/correction.csv");
    _files["detection"] = std::ofstream(_log_folder_path + "/detection.csv");
    _files["error"] = std::ofstream(_log_folder_path + "/error");
}
Logger::~Logger()
{
    for (auto& file : _files | std::views::values) {
        file.close();
    }
}

void Logger::step() { _step++; }
void Logger::logEvent(const IEvent& event)
{
    _files[event.fileName()] << _step << ", " << event.toCsv() << std::endl;
    if (_log_level == LogLevel::None || _log_level == LogLevel::Error) {
        return;
    }
    std::cout << "[INFO ][" << std::setw(6) << std::setfill('0') << _step << "] "
              << event.prettyPrint() << std::endl;
}
void Logger::logError(std::string_view msg)
{
    _files["error"] << _step << ", " << msg << std::endl;

    if (_log_level == LogLevel::None) {
        return;
    }
    std::cerr << "[ERROR][" << std::setw(6) << std::setfill('0') << _step << "] " << msg
              << std::endl;
}
void Logger::logMsg(std::string_view msg)
{
    if (_log_level == LogLevel::All) {
        std::cout << "[INFO ][" << std::setw(6) << std::setfill('0') << _step << "] " << msg
                  << std::endl;
    }
}
