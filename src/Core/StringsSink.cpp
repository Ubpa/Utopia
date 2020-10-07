#include <Utopia/Core/StringsSink.h>

#include <spdlog/details/log_msg_buffer.h>

using namespace Ubpa::Utopia;

std::vector<std::string> StringsSink::CopyLogs() const noexcept {
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(base_sink<std::mutex>::mutex_));
	std::vector<std::string> rst(logs.begin(), logs.end());
	return rst;
}

void StringsSink::Clear() {
	std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
	logs.clear();
}

void StringsSink::sink_it_(const spdlog::details::log_msg& msg) {
	spdlog::details::log_msg_buffer buffer{ msg };
	spdlog::memory_buf_t formatted;
	base_sink<std::mutex>::formatter_->format(buffer, formatted);
	logs.push_back(fmt::to_string(formatted));
}
