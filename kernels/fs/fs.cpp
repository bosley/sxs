#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <kernel_api.hpp>
#include <map>
#include <string>
#include <vector>

static const struct pkg::kernel::api_table_s *g_api = nullptr;
static std::map<int, FILE *> g_open_files;
static std::atomic<int> g_next_fd{1};

static int get_file_descriptor(FILE *fp) {
  if (!fp) {
    return -1;
  }
  int fd = g_next_fd.fetch_add(1);
  g_open_files[fd] = fp;
  return fd;
}

static FILE *get_file_pointer(int fd) {
  auto it = g_open_files.find(fd);
  if (it == g_open_files.end()) {
    return nullptr;
  }
  return it->second;
}

static void close_file_descriptor(int fd) {
  auto it = g_open_files.find(fd);
  if (it != g_open_files.end()) {
    g_open_files.erase(it);
  }
}

static slp::slp_object_c fs_open(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(-1);
  }

  auto mode_obj = g_api->eval(ctx, list.at(1));
  auto path_obj = g_api->eval(ctx, list.at(2));

  if (mode_obj.type() != slp::slp_type_e::DQ_LIST ||
      path_obj.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_int(-1);
  }

  std::string mode = mode_obj.as_string().to_string();
  std::string path = path_obj.as_string().to_string();

  FILE *fp = fopen(path.c_str(), mode.c_str());
  if (!fp) {
    return slp::slp_object_c::create_int(-1);
  }

  int fd = get_file_descriptor(fp);
  return slp::slp_object_c::create_int(fd);
}

static slp::slp_object_c fs_read(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_string("");
  }

  auto fd_obj = g_api->eval(ctx, list.at(1));
  if (fd_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_string("");
  }

  int fd = fd_obj.as_int();
  FILE *fp = get_file_pointer(fd);
  if (!fp) {
    return slp::slp_object_c::create_string("");
  }

  long current_pos = ftell(fp);
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (file_size < 0) {
    fseek(fp, current_pos, SEEK_SET);
    return slp::slp_object_c::create_string("");
  }

  std::vector<char> buffer(file_size + 1);
  size_t read_count = fread(buffer.data(), 1, file_size, fp);
  buffer[read_count] = '\0';

  return slp::slp_object_c::create_string(buffer.data());
}

static slp::slp_object_c fs_read_bytes(pkg::kernel::context_t ctx,
                                       const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_string("");
  }

  auto fd_obj = g_api->eval(ctx, list.at(1));
  auto count_obj = g_api->eval(ctx, list.at(2));

  if (fd_obj.type() != slp::slp_type_e::INTEGER ||
      count_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_string("");
  }

  int fd = fd_obj.as_int();
  long long count = count_obj.as_int();

  FILE *fp = get_file_pointer(fd);
  if (!fp || count < 0) {
    return slp::slp_object_c::create_string("");
  }

  std::vector<char> buffer(count + 1);
  size_t read_count = fread(buffer.data(), 1, count, fp);
  buffer[read_count] = '\0';

  return slp::slp_object_c::create_string(buffer.data());
}

static slp::slp_object_c fs_write(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(-1);
  }

  auto fd_obj = g_api->eval(ctx, list.at(1));
  auto data_obj = g_api->eval(ctx, list.at(2));

  if (fd_obj.type() != slp::slp_type_e::INTEGER ||
      data_obj.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_int(-1);
  }

  int fd = fd_obj.as_int();
  std::string data = data_obj.as_string().to_string();

  FILE *fp = get_file_pointer(fd);
  if (!fp) {
    return slp::slp_object_c::create_int(-1);
  }

  size_t written = fwrite(data.c_str(), 1, data.length(), fp);
  return slp::slp_object_c::create_int(static_cast<long long>(written));
}

static slp::slp_object_c fs_close(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto fd_obj = g_api->eval(ctx, list.at(1));
  if (fd_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_int(-1);
  }

  int fd = fd_obj.as_int();
  FILE *fp = get_file_pointer(fd);
  if (!fp) {
    return slp::slp_object_c::create_int(-1);
  }

  fclose(fp);
  close_file_descriptor(fd);
  return slp::slp_object_c::create_int(0);
}

static slp::slp_object_c fs_seek(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 4) {
    return slp::slp_object_c::create_int(-1);
  }

  auto fd_obj = g_api->eval(ctx, list.at(1));
  auto offset_obj = g_api->eval(ctx, list.at(2));
  auto whence_obj = g_api->eval(ctx, list.at(3));

  if (fd_obj.type() != slp::slp_type_e::INTEGER ||
      offset_obj.type() != slp::slp_type_e::INTEGER ||
      whence_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_int(-1);
  }

  int fd = fd_obj.as_int();
  long long offset = offset_obj.as_int();
  int whence = whence_obj.as_int();

  FILE *fp = get_file_pointer(fd);
  if (!fp || whence < 0 || whence > 2) {
    return slp::slp_object_c::create_int(-1);
  }

  int result = fseek(fp, offset, whence);
  if (result != 0) {
    return slp::slp_object_c::create_int(-1);
  }

  long pos = ftell(fp);
  return slp::slp_object_c::create_int(pos);
}

static slp::slp_object_c fs_tell(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto fd_obj = g_api->eval(ctx, list.at(1));
  if (fd_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_int(-1);
  }

  int fd = fd_obj.as_int();
  FILE *fp = get_file_pointer(fd);
  if (!fp) {
    return slp::slp_object_c::create_int(-1);
  }

  long pos = ftell(fp);
  if (pos < 0) {
    return slp::slp_object_c::create_int(-1);
  }

  return slp::slp_object_c::create_int(pos);
}

static slp::slp_object_c fs_size(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto fd_obj = g_api->eval(ctx, list.at(1));
  if (fd_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_int(-1);
  }

  int fd = fd_obj.as_int();
  FILE *fp = get_file_pointer(fd);
  if (!fp) {
    return slp::slp_object_c::create_int(-1);
  }

  long current_pos = ftell(fp);
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, current_pos, SEEK_SET);

  if (file_size < 0) {
    return slp::slp_object_c::create_int(-1);
  }

  return slp::slp_object_c::create_int(file_size);
}

static slp::slp_object_c fs_exists(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(0);
  }

  auto path_obj = g_api->eval(ctx, list.at(1));
  if (path_obj.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_int(0);
  }

  std::string path = path_obj.as_string().to_string();
  bool exists = std::filesystem::exists(path);

  return slp::slp_object_c::create_int(exists ? 1 : 0);
}

static slp::slp_object_c fs_remove(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto path_obj = g_api->eval(ctx, list.at(1));
  if (path_obj.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_int(-1);
  }

  std::string path = path_obj.as_string().to_string();
  bool success = std::filesystem::remove(path);

  return slp::slp_object_c::create_int(success ? 0 : -1);
}

static slp::slp_object_c fs_rename(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(-1);
  }

  auto old_path_obj = g_api->eval(ctx, list.at(1));
  auto new_path_obj = g_api->eval(ctx, list.at(2));

  if (old_path_obj.type() != slp::slp_type_e::DQ_LIST ||
      new_path_obj.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_int(-1);
  }

  std::string old_path = old_path_obj.as_string().to_string();
  std::string new_path = new_path_obj.as_string().to_string();

  try {
    std::filesystem::rename(old_path, new_path);
    return slp::slp_object_c::create_int(0);
  } catch (...) {
    return slp::slp_object_c::create_int(-1);
  }
}

static slp::slp_object_c fs_flush(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto fd_obj = g_api->eval(ctx, list.at(1));
  if (fd_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_int(-1);
  }

  int fd = fd_obj.as_int();
  FILE *fp = get_file_pointer(fd);
  if (!fp) {
    return slp::slp_object_c::create_int(-1);
  }

  int result = fflush(fp);
  return slp::slp_object_c::create_int(result == 0 ? 0 : -1);
}

static slp::slp_object_c fs_mkdir(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto path_obj = g_api->eval(ctx, list.at(1));
  if (path_obj.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_int(-1);
  }

  std::string path = path_obj.as_string().to_string();

  try {
    bool success = std::filesystem::create_directories(path);
    return slp::slp_object_c::create_int(success ? 0 : -1);
  } catch (...) {
    return slp::slp_object_c::create_int(-1);
  }
}

static slp::slp_object_c fs_rmdir(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto path_obj = g_api->eval(ctx, list.at(1));
  if (path_obj.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_int(-1);
  }

  std::string path = path_obj.as_string().to_string();

  try {
    if (!std::filesystem::is_directory(path)) {
      return slp::slp_object_c::create_int(-1);
    }
    bool success = std::filesystem::remove(path);
    return slp::slp_object_c::create_int(success ? 0 : -1);
  } catch (...) {
    return slp::slp_object_c::create_int(-1);
  }
}

static slp::slp_object_c fs_rmdir_recursive(pkg::kernel::context_t ctx,
                                            const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto path_obj = g_api->eval(ctx, list.at(1));
  if (path_obj.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_int(-1);
  }

  std::string path = path_obj.as_string().to_string();

  try {
    std::uintmax_t count = std::filesystem::remove_all(path);
    return slp::slp_object_c::create_int(static_cast<long long>(count));
  } catch (...) {
    return slp::slp_object_c::create_int(-1);
  }
}

static slp::slp_object_c fs_tmp(pkg::kernel::context_t ctx,
                                const slp::slp_object_c &args) {
  try {
    std::filesystem::path tmp_path = std::filesystem::temp_directory_path();
    std::string tmp_str = tmp_path.string();
    return slp::slp_object_c::create_string(tmp_str);
  } catch (...) {
    return slp::slp_object_c::create_string("");
  }
}

static slp::slp_object_c fs_join_path(pkg::kernel::context_t ctx,
                                      const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_string("");
  }

  try {
    std::filesystem::path result_path;

    for (size_t i = 1; i < list.size(); i++) {
      auto path_obj = g_api->eval(ctx, list.at(i));
      if (path_obj.type() != slp::slp_type_e::DQ_LIST) {
        return slp::slp_object_c::create_string("");
      }

      std::string path_str = path_obj.as_string().to_string();
      if (i == 1) {
        result_path = std::filesystem::path(path_str);
      } else {
        result_path /= path_str;
      }
    }

    std::string joined = result_path.string();
    return slp::slp_object_c::create_string(joined);
  } catch (...) {
    return slp::slp_object_c::create_string("");
  }
}

static slp::slp_object_c fs_ls(pkg::kernel::context_t ctx,
                               const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_bracket_list(nullptr, 0);
  }

  auto path_obj = g_api->eval(ctx, list.at(1));
  if (path_obj.type() != slp::slp_type_e::DQ_LIST) {
    return slp::slp_object_c::create_bracket_list(nullptr, 0);
  }

  std::string path = path_obj.as_string().to_string();

  try {
    if (!std::filesystem::is_directory(path)) {
      return slp::slp_object_c::create_bracket_list(nullptr, 0);
    }

    std::vector<slp::slp_object_c> entries;
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
      std::string filename = entry.path().filename().string();
      entries.push_back(slp::slp_object_c::create_string(filename));
    }

    return slp::slp_object_c::create_bracket_list(entries.data(),
                                                  entries.size());
  } catch (...) {
    return slp::slp_object_c::create_bracket_list(nullptr, 0);
  }
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const struct pkg::kernel::api_table_s *api) {
  g_api = api;
  api->register_function(registry, "open", fs_open, slp::slp_type_e::INTEGER,
                         0);
  api->register_function(registry, "read", fs_read, slp::slp_type_e::DQ_LIST,
                         0);
  api->register_function(registry, "read_bytes", fs_read_bytes,
                         slp::slp_type_e::DQ_LIST, 0);
  api->register_function(registry, "write", fs_write, slp::slp_type_e::INTEGER,
                         0);
  api->register_function(registry, "close", fs_close, slp::slp_type_e::INTEGER,
                         0);
  api->register_function(registry, "seek", fs_seek, slp::slp_type_e::INTEGER,
                         0);
  api->register_function(registry, "tell", fs_tell, slp::slp_type_e::INTEGER,
                         0);
  api->register_function(registry, "size", fs_size, slp::slp_type_e::INTEGER,
                         0);
  api->register_function(registry, "exists", fs_exists,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "remove", fs_remove,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "rename", fs_rename,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "flush", fs_flush, slp::slp_type_e::INTEGER,
                         0);
  api->register_function(registry, "mkdir", fs_mkdir, slp::slp_type_e::INTEGER,
                         0);
  api->register_function(registry, "rmdir", fs_rmdir, slp::slp_type_e::INTEGER,
                         0);
  api->register_function(registry, "rmdir_recursive", fs_rmdir_recursive,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "tmp", fs_tmp, slp::slp_type_e::DQ_LIST, 0);
  api->register_function(registry, "join_path", fs_join_path,
                         slp::slp_type_e::DQ_LIST, 1);
  api->register_function(registry, "ls", fs_ls, slp::slp_type_e::BRACKET_LIST,
                         0);
}

extern "C" void kernel_shutdown(const struct pkg::kernel::api_table_s *api) {
  for (auto &pair : g_open_files) {
    fclose(pair.second);
  }
  g_open_files.clear();
}
