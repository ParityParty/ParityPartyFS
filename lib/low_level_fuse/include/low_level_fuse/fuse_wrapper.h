#pragma once

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 35
#endif

#include <fuse_lowlevel.h>

namespace low_level_fuse {
    typedef void (*t_init)(void *userdata, struct fuse_conn_info *conn);
    typedef void (*t_destroy) (void *userdata);
    typedef void (*t_lookup) (fuse_req_t req, fuse_ino_t parent, const char *name);
    typedef void (*t_forget) (fuse_req_t req, fuse_ino_t ino, uint64_t nlookup);
    typedef void (*t_getattr) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
	typedef void (*t_setattr) (fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi);
    typedef void (*t_readlink) (fuse_req_t req, fuse_ino_t ino);
	typedef void (*t_mknod) (fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, dev_t rdev);
    typedef void (*t_mkdir) (fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode);
	typedef void (*t_unlink) (fuse_req_t req, fuse_ino_t parent, const char *name);
	typedef void (*t_rmdir) (fuse_req_t req, fuse_ino_t parent, const char *name);
	typedef void (*t_symlink) (fuse_req_t req, const char *link, fuse_ino_t parent, const char *name);
	typedef void (*t_rename) (fuse_req_t req, fuse_ino_t parent, const char *name, fuse_ino_t newparent, const char *newname, unsigned int flags);
	typedef void (*t_link) (fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent, const char *newname);
	typedef void (*t_open) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
	typedef void (*t_read) (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);
	typedef void (*t_write) (fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi);
	typedef void (*t_flush) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
	typedef void (*t_release) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
	typedef void (*t_fsync) (fuse_req_t req, fuse_ino_t ino, int datasync, struct fuse_file_info *fi);
	typedef void (*t_opendir) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
	typedef void (*t_readdir) (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);
	typedef void (*t_releasedir) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
	typedef void (*t_fsyncdir) (fuse_req_t req, fuse_ino_t ino, int datasync, struct fuse_file_info *fi);
	typedef void (*t_statfs) (fuse_req_t req, fuse_ino_t ino);
	typedef void (*t_setxattr) (fuse_req_t req, fuse_ino_t ino, const char *name, const char *value, size_t size, int flags);
	typedef void (*t_getxattr) (fuse_req_t req, fuse_ino_t ino, const char *name, size_t size);
	typedef void (*t_listxattr) (fuse_req_t req, fuse_ino_t ino, size_t size);
	typedef void (*t_removexattr) (fuse_req_t req, fuse_ino_t ino, const char *name);
	typedef void (*t_access) (fuse_req_t req, fuse_ino_t ino, int mask);
	typedef void (*t_create) (fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, struct fuse_file_info *fi);
    typedef void (*t_getlk) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi, struct flock *lock);
	typedef void (*t_setlk) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi, struct flock *lock, int sleep);
	typedef void (*t_bmap) (fuse_req_t req, fuse_ino_t ino, size_t blocksize, uint64_t idx);
#if FUSE_USE_VERSION < 35
	typedef void (*t_ioctl) (fuse_req_t req, fuse_ino_t ino, int cmd, void *arg, struct fuse_file_info *fi, unsigned flags, const void *in_buf, size_t in_bufsz, size_t out_bufsz);
#else
	typedef void (*t_ioctl) (fuse_req_t req, fuse_ino_t ino, unsigned int cmd, void *arg, struct fuse_file_info *fi, unsigned flags, const void *in_buf, size_t in_bufsz, size_t out_bufsz);
#endif
	typedef void (*t_poll) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi, struct fuse_pollhandle *ph);
	typedef void (*t_write_buf) (fuse_req_t req, fuse_ino_t ino, struct fuse_bufvec *bufv, off_t off, struct fuse_file_info *fi);
    typedef void (*t_retrieve_reply) (fuse_req_t req, void *cookie, fuse_ino_t ino, off_t offset, struct fuse_bufvec *bufv);
	typedef void (*t_forget_multi) (fuse_req_t req, size_t count, struct fuse_forget_data *forgets);
	typedef void (*t_flock) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi, int op);
	typedef void (*t_fallocate) (fuse_req_t req, fuse_ino_t ino, int mode, off_t offset, off_t length, struct fuse_file_info *fi);
	typedef void (*t_readdirplus) (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);
	typedef void (*t_copy_file_range) (fuse_req_t req, fuse_ino_t ino_in, off_t off_in, struct fuse_file_info *fi_in, fuse_ino_t ino_out, off_t off_out, struct fuse_file_info *fi_out, size_t len, int flags);
	typedef void (*_lseek) (fuse_req_t req, fuse_ino_t ino, off_t off, int whence, struct fuse_file_info *fi);
}
template <class T> class Fuse {
public:

    Fuse() {
        memset(&T::operations_, 0, sizeof(struct fuse_lowlevel_ops));
        _load_operations();
    }

    // no copy
    Fuse(const Fuse&) = delete;
    Fuse(Fuse&&) = delete;
    Fuse& operator=(const Fuse&) = delete;
    Fuse& operator=(Fuse&&) = delete;

    virual ~Fuse() = default;

    virtual int Run(int argc, char** argv) {
	
        struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	    struct fuse_session *se;
	    struct fuse_cmdline_opts opts;
	    struct fuse_loop_config config;
	    int ret = -1;

	    if (fuse_parse_cmdline(&args, &opts) != 0)
	    	return 1;
    	if (opts.show_help) {
	    	printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
		    fuse_cmdline_help();
    		fuse_lowlevel_help();
	    	ret = 0;
    		goto err_out1;
    	} else if (opts.show_version) {
    		printf("FUSE library version %s\n", fuse_pkgversion());
	    	fuse_lowlevel_version();
		    ret = 0;
    		goto err_out1;
	    }

    	if(opts.mountpoint == NULL) {
    		printf("usage: %s [options] <mountpoint>\n", argv[0]);
	    	printf("       %s --help\n", argv[0]);
		    ret = 1;
    		goto err_out1;
	    }

    	se = fuse_session_new(&args, &hello_ll_oper,
	    		      sizeof(hello_ll_oper), NULL);
	    if (se == NULL)
	        goto err_out1;

	    if (fuse_set_signal_handlers(se) != 0)
	        goto err_out2;

	    if (fuse_session_mount(se, opts.mountpoint) != 0)
	        goto err_out3;

    	fuse_daemonize(opts.foreground);

	    /* Block until ctrl+c or fusermount -u */
	    if (opts.singlethread)
		    ret = fuse_session_loop(se);
    	else {
	    	config.clone_fd = opts.clone_fd;
		    config.max_idle_threads = opts.max_idle_threads;
    		ret = fuse_session_loop_mt(se, &config);
	    }

    	fuse_session_unmount(se);
    err_out3:
	    fuse_remove_signal_handlers(se);
    err_out2:
	    fuse_session_destroy(se);
    err_out1:
	    free(opts.mountpoint);
	    fuse_opt_free_args(&args);

	    return ret ? 1 : 0;
    }

    static auto this_() { return static_cast<T*>(fuse_get_context()->private_data); }

private:
    static struct fuse_lovwlevel_ops _operations;

    static void _load_operations()
    {
        _operations.init = T::init;
        _operations.destroy = T::destroy;
        _operations.lookup = T::lookup;
        _operations.forget = T::forget;
        _operations.getattr = T::getattr;
        _operations.setattr = T::setattr;
        _operations.readlink = T::readlink;
        _operations.mknod = T::mknod;
        _operations.mkdir = T::mkdir;
        _operations.unlink = T::unlink;
        _operations.rmdir = T::rmdir;
        _operations.symlink = T::symlink;
        _operations.rename = T::rename;
        _operations.link = T::link;
        _operations.open = T::open;
        _operations.read = T::read;
        _operations.write = T::write;
        _operations.flush = T::flush;
        _operations.release = T::release;
        _operations.fsync = T::fsync;
        _operations.opendir = T::opendir;
        _operations.readdir = T::readdir;
        _operations.releasedir = T::releasedir;
        _operations.fsyncdir = T::fsyncdir;
        _operations.statfs = T::statfs;
        _operations.setxattr = T::setxattr;
        _operations.getxattr = T::getxattr;
        _operations.listxattr = T::listxattr;
        _operations.removexattr = T::removexattr;
        _operations.access = T::access;
        _operations.create = T::create;
        _operations.getlk = T::getlk;
        _operations.setlk = T::setlk;
        _operations.bmap = T::bmap;
        _operations.ioctl = T::ioctl;
        _operations.poll = T::poll;
        _operations.write_buf = T::write_buf;
        _operations.retrieve_reply = T::retrieve_reply;
        _operations.forget_multi = T::forget_multi;
        _operations.flock = T::flock;
        _operations.fallocate = T::fallocate;
        _operations.readdirplus = T::readdirplus;
        _operations.copy_file_range = T::copy_file_range;
        _operations.lseek = T::_lseek;
    }

    static t_init init;
    static t_destroy destroy;
    static t_lookup lookup;
    static t_forget forget;
    static t_getattr getattr;
    static t_setattr setattr;
    static t_readlink readlink;
    static t_mknod mknod;
    static t_mkdir mkdir;
    static t_unlink unlink;
    static t_rmdir rmdir;
    static t_symlink symlink;
    static t_rename rename;
    static t_link link;
    static t_open open;
    static t_read read;
    static t_write write;
    static t_flush flush;
    static t_release release;
    static t_fsync fsync;
    static t_opendir opendir;
    static t_readdir readdir;
    static t_releasedir releasedir;
    static t_fsyncdir fsyncdir;
    static t_statfs statfs;
    static t_setxattr setxattr;
    static t_getxattr getxattr;
    static t_listxattr listxattr;
    static t_removexattr removexattr;
    static t_access access;
    static t_create create;
    static t_getlk getlk;
    static t_setlk setlk;
    static t_bmap bmap;
    static t_ioctl ioctl;
    static t_poll poll;
    static t_write_buf write_buf;
    static t_retrieve_reply retrieve_reply;
    static t_forget_multi forget_multi;
    static t_flock flock;
    static t_fallocate fallocate;
    static t_readdirplus readdirplus;
    static t_copy_file_range copy_file_range;
    static _lseek _lseek;
};
