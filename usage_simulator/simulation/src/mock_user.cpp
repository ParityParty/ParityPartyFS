#include "simulation/mock_user.hpp"

#include <array>
#include <chrono>
#include <vector>

void SingleDirMockUser::_createFile()
{
    std::stringstream ss;
    ss << _dir << "/" << _file_id++;
    auto fn = new FileNode(ss.str(), false, 0, std::vector<FileNode*>());
    auto ret = _fs.create(fn->name);
    if (!ret.has_value()) {
        if (ret.error() != FsError::InodeManager_NoMoreFreeInodes) {
            _logger->logError(toString(ret.error()));
        }
    } else {
        _root->children.push_back(fn);
        _logger->logMsg((std::stringstream()
            << "User " << static_cast<int>(id) << " Created file number: " << _file_id - 1)
                .str());
    }
}
void SingleDirMockUser::_writeToFile()
{
    if (_root->children.size() <= 0) {
        return;
    }
    std::uniform_int_distribution<int> file_distribution(0, _root->children.size() - 1);
    auto file = _root->children.at(file_distribution(_rng));

    std::uniform_int_distribution<int> write_size_distribution(1, _behaviour.max_write_size);
    auto write_size = write_size_distribution(_rng);

    auto open_ret = _fs.open(file->name, OpenMode::Append);
    if (!open_ret.has_value()) {
        _logger->logError(toString(open_ret.error()));
        _logger->logEvent(ReadEvent(
            0, std::chrono::duration<long, std::micro>::zero(), IoOperationResult::ExplicitError));
        return;
    }
    auto start = std::chrono::high_resolution_clock::now();
    std::array<uint8_t, 65536> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_size);
    std::fill(write_data.data(), write_data.data() + write_size, id);
    auto write_ret = _fs.write(open_ret.value(), write_data);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    if (!write_ret.has_value()) {
        if (write_ret.error() != FsError::BlockManager_NoMoreFreeBlocks) {
            _logger->logError(toString(write_ret.error()));
            _logger->logEvent(WriteEvent(write_size, duration, IoOperationResult::ExplicitError));
        }
    } else {
        file->size += write_size;

        _logger->logEvent(WriteEvent(write_size, duration, IoOperationResult::Success));
    }
    auto close_ret = _fs.close(open_ret.value());
    if (!close_ret.has_value()) {
        _logger->logError(toString(close_ret.error()));
    }
}
void SingleDirMockUser::_readFromFile()
{
    if (_root->children.size() <= 0) {
        return;
    }
    std::uniform_int_distribution<int> file_distribution(0, _root->children.size() - 1);
    auto file = _root->children.at(file_distribution(_rng));
    if (file->size == 0) {
        return;
    }

    std::uniform_int_distribution<int> read_size_distribution(1, _behaviour.max_read_size);
    auto read_size = std::min(file->size, static_cast<size_t>(read_size_distribution(_rng)));

    auto open_ret = _fs.open(file->name, OpenMode::Normal);
    if (!open_ret.has_value()) {
        _logger->logError(toString(open_ret.error()));
        _logger->logEvent(ReadEvent(
            0, std::chrono::duration<long, std::micro>::zero(), IoOperationResult::ExplicitError));
        return;
    }
    auto start = std::chrono::high_resolution_clock::now();
    std::array<uint8_t, 65536> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_ret = _fs.read(open_ret.value(), read_size, read_data);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    if (!read_ret.has_value()) {
        _logger->logError(toString(read_ret.error()));
        _logger->logEvent(ReadEvent(read_size, duration, IoOperationResult::ExplicitError));
    } else {
        bool has_error = false;
        for (auto b : read_data) {
            if (b != id) {
                _logger->logError("Data contains an error");
                has_error = true;
                break;
            }
        }
        _logger->logEvent(ReadEvent(read_data.size(), duration,
            has_error ? IoOperationResult::FalseSuccess : IoOperationResult::Success));
    }
    auto close_ret = _fs.close(open_ret.value());
    if (!close_ret.has_value()) {
        _logger->logError(toString(close_ret.error()));
    }
}

void SingleDirMockUser::_deleteFile()
{
    if (_root->children.size() <= 0) {
        return;
    }
    std::uniform_int_distribution<int> file_distribution(0, _root->children.size() - 1);
    auto index = file_distribution(_rng);
    auto file = _root->children.at(index);
    auto ret = _fs.remove(file->name);
    if (!ret.has_value()) {
        _logger->logError(toString(ret.error()));
    } else {
        _root->children.erase(_root->children.begin() + index);
        _logger->logMsg((std::stringstream()
            << "User " << static_cast<int>(index) << " deleted file: " << file->name)
                .str());
    }
}
SingleDirMockUser::SingleDirMockUser(IFilesystem& fs, std::shared_ptr<Logger> logger,
    UserBehaviour behaviour, std::uint8_t id, std::string_view dir, unsigned int seed)
    : _fs(fs)
    , _logger(logger)
    , _behaviour(behaviour)
    , id(id)
    , _dir(dir)
    , _root(new FileNode {
          .name = std::string(_dir),
          .is_dir = true,
          .children = std::vector<FileNode*>(),
      })
    , _rng(seed)
{
    auto ret = _fs.createDirectory(_dir);
    if (!ret.has_value()) {
        _logger->logError(toString(ret.error()));
    }
}

void SingleDirMockUser::step()
{
    if (_to_next_op > 0) {
        _to_next_op--;
        return;
    }

    std::uniform_int_distribution<int> next_op_dist(1, 2 * _behaviour.avg_steps_between_ops);
    _to_next_op = next_op_dist(_rng);

    std::discrete_distribution<int> op_dist({ _behaviour.create_weight, _behaviour.write_weight,
        _behaviour.read_weight, _behaviour.delete_weight });

    switch (op_dist(_rng)) {
    case 0: {
        _createFile();
        break;
    }
    case 1: {
        _writeToFile();
        break;
    }
    case 2: {
        _readFromFile();
        break;
    }
    case 3: {
        _deleteFile();
        break;
    }
    }
}
