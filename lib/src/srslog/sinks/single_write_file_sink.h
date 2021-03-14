/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsLTE.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef SRSLOG_SINGLE_WRITE_FILE_SINK_H
#define SRSLOG_SINGLE_WRITE_FILE_SINK_H

#include "file_utils.h"
#include "srslte/srslog/sink.h"

namespace srslog {

/// This class is a wrapper of a file handle that stores the input data into an
/// internal buffer and writes its contents to the file once the buffer is full
/// or in object destruction.
class single_write_file_sink : public sink
{
public:
  single_write_file_sink(std::string filename,
                         std::size_t capacity,
                         std::unique_ptr<log_formatter> f) :
    sink(std::move(f)), filename(std::move(filename))
  {
    buffer.reserve(capacity);
  }

  ~single_write_file_sink() override
  {
    if (!is_written) {
      write_contents();
    }
  }

  single_write_file_sink(const single_write_file_sink& other) = delete;
  single_write_file_sink&
  operator=(const single_write_file_sink& other) = delete;

  detail::error_string write(detail::memory_buffer input_buffer) override
  {
    // Nothing to do when the contents have been already written.
    if (is_written) {
      return {};
    }

    if (has_room_for(input_buffer.size())) {
      buffer.insert(buffer.end(), input_buffer.begin(), input_buffer.end());
      return {};
    }

    return write_contents();
  }

  detail::error_string flush() override { return handler.flush(); }

private:
  /// Returns true if the internal buffer has room for the specified input size,
  /// otherwise returns false.
  bool has_room_for(std::size_t s) const
  {
    return s + buffer.size() < buffer.capacity();
  }

  /// Writes the buffer contents into the file.
  detail::error_string write_contents()
  {
    is_written = true;
    if (auto err_str = handler.create(filename)) {
      return err_str;
    }
    return handler.write(detail::memory_buffer(buffer.data(), buffer.size()));
  }

private:
  const std::string filename;
  file_utils::file handler;
  std::vector<char> buffer;
  bool is_written = false;
};

} // namespace srslog

#endif // SRSLOG_SINGLE_WRITE_FILE_SINK_H