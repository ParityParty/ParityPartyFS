#pragma once
#include <chrono>
#include <cstddef>
#include <string>

class IEvent {
public:
    virtual ~IEvent() = default;
    virtual std::string prettyPrint() const = 0;
    virtual std::string toCsv() const = 0;
    virtual std::string fileName() const = 0;
};

struct ReadEvent : public IEvent {
    ReadEvent(size_t read_size, std::chrono::duration<long, std::milli> time_ms);
    size_t read_size;
    std::chrono::duration<long, std::milli> time_ms;

    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

struct WriteEvent : public IEvent {
    WriteEvent(size_t write_size, std::chrono::duration<long, std::milli> time_ms);
    size_t write_size;
    std::chrono::duration<long, std::milli> time_ms;
    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

struct BitFlipEvent : public IEvent {
    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

struct ErrorCorrectionEvent : public IEvent {
    std::string prettyPrint() const override;
    std::string toCsv() const override;
    std::string fileName() const override;
};

class Logger {
    int _step = 0;

public:
    void step();
    void log(const IEvent& event);
};
