#include "mock_user/mock_user.hpp"
MockUser::MockUser(IFilesystem& fs)
    : _fs(fs)
    , _root(new FileNode { .is_dir = true })
{
}
