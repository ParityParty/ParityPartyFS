#pragma once
#include "common/types.hpp"

#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

enum class IoOperationResult {
    Success,
    // Read operation wasn't successful and method call returned an error
    ExplicitError,
    // Read operation wasn't successful but no error has been reported
    FalseSuccess,
};

class IEvent {
public:
    virtual ~IEvent() = default;
    virtual std::string prettyPrint() const = 0;
    virtual std::string toCsv() const = 0;
    virtual std::string fileName() const = 0;
};

struct ReadEvent : IEvent {
    size_t read_size;
    std::chrono::duration<long, std::micro> time;
    IoOperationResult result;

    ReadEvent(
        size_t read_size, std::chrono::duration<long, std::micro> time, IoOperationResult result);
    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

struct WriteEvent : IEvent {
    size_t write_size;
    std::chrono::duration<long, std::micro> time;
    IoOperationResult result;

    WriteEvent(size_t write_size, std::chrono::duration<long, std::micro> time_ms,
        IoOperationResult result);

    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

struct BitFlipEvent : IEvent {
    size_t byte_index;
    BitFlipEvent(size_t byte_index);
    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

struct ErrorCorrectionEvent : IEvent {
    ErrorCorrectionEvent(std::string ecc_type, block_index_t block_index);
    std::string ecc_type;
    block_index_t block_index;
    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

class Logger {
public:
    enum class LogLevel : std::uint8_t { None, Error, Medium, All };

    Logger(LogLevel log_level, const std::string& log_folder_path);
    ~Logger();
    void step();
    void logEvent(const IEvent& event);
    void logError(std::string_view msg);
    void logMsg(std::string_view msg);

private:
    int _step = 0;
    LogLevel _log_level;
    std::string _log_folder_path;
    std::map<std::string, std::ofstream> _files;
};
