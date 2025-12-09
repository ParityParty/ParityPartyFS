#include "data_collection/data_colection.hpp"

#include <iostream>
#include <sstream>

ReadEvent::ReadEvent(size_t read_size, std::chrono::duration<long, std::milli> time_ms)
    : read_size { read_size }
    , time_ms { time_ms }
{
}
std::string ReadEvent::prettyPrint() const
{
    std::stringstream ss;
    ss << "Read " << read_size << " bytes in " << time_ms.count() << " ms";
    return ss.str();
}
std::string ReadEvent::toCsv() const
{
    std::stringstream ss;
    ss << read_size << "," << time_ms.count();
    return ss.str();
}
std::string ReadEvent::fileName() const { return "read"; }
WriteEvent::WriteEvent(size_t write_size, std::chrono::duration<long, std::milli> time_ms)
    : write_size { write_size }
    , time_ms { time_ms }
{
}
std::string WriteEvent::prettyPrint() const
{
    std::stringstream ss;
    ss << "Written " << write_size << " bytes in " << time_ms.count() << " ms";
    return ss.str();
}
std::string WriteEvent::toCsv() const
{
    std::stringstream ss;
    ss << write_size << "," << time_ms.count();
    return ss.str();
}
std::string WriteEvent::fileName() const { return "write"; }
std::string BitFlipEvent::prettyPrint() const { return "Oh no! A bit has flipped!"; }
std::string BitFlipEvent::toCsv() const { return ""; }
std::string BitFlipEvent::fileName() const { return "flip"; }
std::string ErrorCorrectionEvent::prettyPrint() const
{
    std::stringstream ss;
    ss << "Bit flip corrected";
    return ss.str();
}
std::string ErrorCorrectionEvent::toCsv() const { return ""; }
std::string ErrorCorrectionEvent::fileName() const { return "correction"; }

void Logger::step() { _step++; }
void Logger::logEvent(const IEvent& event)
{
    std::cout << "[INFO ][" << std::setw(6) << std::setfill('0') << _step << "] "
              << event.prettyPrint() << std::endl;
}
void Logger::logError(std::string_view msg)
{
    std::cerr << "[ERROR][" << std::setw(6) << std::setfill('0') << _step << "] " << msg
              << std::endl;
}
void Logger::logMsg(std::string_view msg)
{
    std::cout << "[INFO ][" << std::setw(6) << std::setfill('0') << _step << "] " << msg
              << std::endl;
}
