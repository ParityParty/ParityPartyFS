#include "data_collection/data_colection.hpp"

#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>

ReadEvent::ReadEvent(
    size_t read_size, std::chrono::duration<long, std::micro> time, IoOperationResult result)
    : read_size { read_size }
    , time { time }
    , result(result)
{
}

std::string ReadEvent::prettyPrint() const
{
    std::stringstream ss;
    switch (result) {
    case IoOperationResult::Success:
        ss << "Read " << read_size << " bytes in " << time.count() << " microseconds";
        break;
    case IoOperationResult::ExplicitError:
        ss << "Detected uncorrectable bitflip during read operation in" << time.count()
           << " microseconds";
    case IoOperationResult::FalseSuccess:
        ss << "Read damaged data without reporting error";
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& lhs, IoOperationResult rhs)
{
    switch (rhs) {
    case IoOperationResult::Success:
        lhs << "success";
        break;
    case IoOperationResult::ExplicitError:
        lhs << "explicit_error";
        break;
    case IoOperationResult::FalseSuccess:
        lhs << "false_success";
        break;
    }
    return lhs;
}

std::string ReadEvent::toCsv() const
{
    std::stringstream ss;
    ss << read_size << "," << time.count() << "," << result;
    return ss.str();
}

std::string ReadEvent::fileName() const { return "read"; }
WriteEvent::WriteEvent(
    size_t write_size, std::chrono::duration<long, std::micro> time, IoOperationResult result)
    : write_size { write_size }
    , time { time }
    , result(result)
{
}

std::string WriteEvent::prettyPrint() const
{
    std::stringstream ss;
    switch (result) {
    case IoOperationResult::Success:
        ss << "Written " << write_size << " bytes in " << time.count() << " microseconds";
        break;
    case IoOperationResult::ExplicitError:
        ss << "Detected uncorrectable bitflip during write operation in " << time.count()
           << " microseconds";
    case IoOperationResult::FalseSuccess:
        ss << "Read damaged data without reporting error during write operation";
    }
    return ss.str();
}

std::string WriteEvent::toCsv() const
{
    std::stringstream ss;
    ss << write_size << "," << time.count() << "," << result;
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

Logger::Logger(const LogLevel log_level, const std::string& log_folder_path)
    : _log_level(log_level)
    , _log_folder_path(log_folder_path)
{
    (void)_mtx.init();
    _files["read"] = std::ofstream(_log_folder_path + "/read.csv", std::ios::out);
    _files["read"] << "step,size,time,result" << std::endl;
    _files["write"] = std::ofstream(_log_folder_path + "/write.csv");
    _files["write"] << "step,size,time,result" << std::endl;
    _files["flip"] = std::ofstream(_log_folder_path + "/flip.csv");
    _files["flip"] << "step,address" << std::endl;
    _files["correction"] = std::ofstream(_log_folder_path + "/correction.csv");
    _files["correction"] << "step,ecc_type,block" << std::endl;
    _files["detection"] = std::ofstream(_log_folder_path + "/detection.csv");
    _files["detection"] << "step,ecc_type,block" << std::endl;
    _files["error"] = std::ofstream(_log_folder_path + "/error.csv");
    _files["error"] << "step,message" << std::endl;
}

Logger::~Logger()
{
    for (auto& file : _files | std::views::values) {
        file.close();
    }
}

void Logger::step()
{
    (void)_mtx.lock();
    _step++;
    (void)_mtx.unlock();
}

void Logger::logEvent(const IEvent& event)
{
    (void)_mtx.lock();
    _files[event.fileName()] << _step << "," << event.toCsv() << std::endl;
    if (_log_level == LogLevel::None || _log_level == LogLevel::Error) {
        (void)_mtx.unlock();
        return;
    }
    std::cout << "[INFO ][" << std::setw(6) << std::setfill('0') << _step << "] "
              << event.prettyPrint() << std::endl;
    (void)_mtx.unlock();
}

void Logger::logError(std::string_view msg)
{
    (void)_mtx.lock();
    _files["error"] << _step << "," << msg << std::endl;

    if (_log_level == LogLevel::None) {
        (void)_mtx.unlock();
        return;
    }
    std::cerr << "[ERROR][" << std::setw(6) << std::setfill('0') << _step << "] " << msg
              << std::endl;
    (void)_mtx.unlock();
}

void Logger::logMsg(std::string_view msg)
{
    (void)_mtx.lock();
    if (_log_level == LogLevel::All) {
        std::cout << "[INFO ][" << std::setw(6) << std::setfill('0') << _step << "] " << msg
                  << std::endl;
    }
    (void)_mtx.unlock();
}
