#pragma once
#include "common/types.hpp"

#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

/**
 * Result status of an I/O operation.
 */
enum class IoOperationResult {
    /** Operation completed successfully */
    Success,
    /** Operation failed and returned an error */
    ExplicitError,
    /** Operation failed but no error was reported */
    FalseSuccess,
};

/**
 * Base interface for filesystem events.
 */
class IEvent {
public:
    virtual ~IEvent() = default;
    virtual std::string prettyPrint() const = 0;
    virtual std::string toCsv() const = 0;
    virtual std::string fileName() const = 0;
};

/**
 * Event representing a read operation.
 */
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

/**
 * Event representing a write operation.
 */
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

/**
 * Event representing a bit flip in disk storage.
 */
struct BitFlipEvent : IEvent {
    size_t byte_index;
    BitFlipEvent(size_t byte_index);
    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

/**
 * Event representing error detection and correction.
 */
struct ErrorCorrectionEvent : IEvent {
    ErrorCorrectionEvent(std::string ecc_type, block_index_t block_index);
    std::string ecc_type;
    block_index_t block_index;
    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

/**
 * Logger for recording filesystem events and errors.
 */
class Logger {
public:
    enum class LogLevel : std::uint8_t { None, Error, Medium, All };

    /**
     * Constructs a Logger instance.
     * @param log_level Minimum level of events to log.
     * @param log_folder_path Directory where log files will be written.
     */
    Logger(LogLevel log_level, const std::string& log_folder_path);
    ~Logger();
    /**
     * Advances to the next simulation step.
     */
    void step();
    /**
     * Records a filesystem event.
     * @param event The event to log.
     */
    void logEvent(const IEvent& event);
    /**
     * Logs an error message.
     * @param msg Error message to record.
     */
    void logError(std::string_view msg);
    /**
     * Logs an informational message.
     * @param msg Message to record.
     */
    void logMsg(std::string_view msg);

private:
    int _step = 0;
    LogLevel _log_level;
    std::string _log_folder_path;
    std::map<std::string, std::ofstream> _files;
};
