#pragma once

#include <cstdint>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>

#include <openskp/export.hpp>
#include <openskp/observability.hpp>

namespace openskp {

class OPENSKP_EXPORT SkpParseError : public std::runtime_error {
 public:
  SkpParseError(std::string message, std::optional<ParseStage> stage = {},
                std::optional<std::uint64_t> record_index = {},
                std::optional<std::uint64_t> total_records = {},
                std::optional<std::string> tag = {}, std::optional<std::uint64_t> offset = {},
                std::optional<std::int64_t> definition_id = {}, std::exception_ptr cause = {});

  const std::optional<ParseStage>& stage() const noexcept { return stage_; }

  const std::optional<std::uint64_t>& record_index() const noexcept { return record_index_; }

  const std::optional<std::uint64_t>& total_records() const noexcept { return total_records_; }

  const std::optional<std::string>& tag() const noexcept { return tag_; }

  const std::optional<std::uint64_t>& offset() const noexcept { return offset_; }

  const std::optional<std::int64_t>& definition_id() const noexcept { return definition_id_; }

  std::exception_ptr cause() const noexcept { return cause_; }

 private:
  std::optional<ParseStage> stage_;
  std::optional<std::uint64_t> record_index_;
  std::optional<std::uint64_t> total_records_;
  std::optional<std::uint64_t> offset_;
  std::optional<std::string> tag_;
  std::optional<std::int64_t> definition_id_;
  std::exception_ptr cause_;
};
}  // namespace openskp
