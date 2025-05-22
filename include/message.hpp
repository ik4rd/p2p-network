//
// Created by ilya on 22.05.2025.
//

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

enum class MessageType : uint8_t { TEXT = 0, FILE = 1, REQUEST = 2 };

class Message {
public:
	MessageType type;
	std::string id;
	std::string sender;
	std::chrono::system_clock::time_point timestamp;

	explicit Message(const MessageType type) : type(type) {}

	virtual ~Message() = default;

	virtual std::string serialize() const = 0;

	virtual std::unique_ptr<Message> clone() const = 0;

	static std::unique_ptr<Message> deserialize(const std::string &data);

	static std::string makeuid();

protected:
	std::string serialize_header() const {
		const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
								timestamp.time_since_epoch())
								.count();
		std::ostringstream oss;
		oss << static_cast<int>(type) << '|' << id << '|' << sender << '|' << ms
			<< '|';
		return oss.str();
	}

	static void deserialize_header(std::istringstream &iss,
								   MessageType &out_type, std::string &out_id,
								   std::string &out_sender, long long &out_ms) {
		int t;
		iss >> t;
		iss.get();
		out_type = static_cast<MessageType>(t);
		std::getline(iss, out_id, '|');
		std::getline(iss, out_sender, '|');
		iss >> out_ms;
		iss.get();
	}
};

class TextMessage final : public Message {
public:
	std::string text;

	explicit TextMessage() : Message(MessageType::TEXT) {}

	std::string serialize() const override { return serialize_header() + text; }

	std::unique_ptr<Message> clone() const override {
		return std::make_unique<TextMessage>(*this);
	}

	static std::unique_ptr<TextMessage> from_stream(std::istringstream &iss,
													const std::string &id,
													const std::string &sender,
													const long long ms) {
		auto ptr = std::make_unique<TextMessage>();
		ptr->id = id;
		ptr->sender = sender;
		ptr->timestamp = std::chrono::system_clock::time_point(
				std::chrono::milliseconds(ms));
		std::getline(iss, ptr->text);
		return ptr;
	}
};

class FileMessage final : public Message {
public:
	std::string filename;
	std::vector<char> bytes;

	explicit FileMessage() : Message(MessageType::FILE) {}

	std::string serialize() const override {
		const auto header = serialize_header();
		std::ostringstream oss;
		oss << filename << '|' << bytes.size() << '|';
		std::string out = header + oss.str();
		out.append(bytes.data(), bytes.size());
		return out;
	}

	std::unique_ptr<Message> clone() const override {
		return std::make_unique<FileMessage>(*this);
	}

	static std::unique_ptr<FileMessage> from_stream(
			const std::string &data, const std::string &id,
			const std::string &sender, const long long ms,
			const std::streampos &payload) {
		std::istringstream iss(data.substr(0, payload));
		std::string file_name;
		size_t size;
		std::getline(iss, file_name, '|');
		iss >> size;
		iss.get();
		auto ptr = std::make_unique<FileMessage>();
		ptr->id = id;
		ptr->sender = sender;
		ptr->timestamp = std::chrono::system_clock::time_point(
				std::chrono::milliseconds(ms));
		ptr->filename = file_name;
		ptr->bytes.assign(data.begin() + payload,
						  data.begin() + payload + size);
		return ptr;
	}
};

class RequestMessage final : public Message {
public:
	std::string filename;

	RequestMessage() : Message(MessageType::REQUEST) {}

	std::string serialize() const override {
		return serialize_header() + filename;
	}

	std::unique_ptr<Message> clone() const override {
		return std::make_unique<RequestMessage>(*this);
	}

	static std::unique_ptr<RequestMessage> from_stream(
			std::istringstream &iss, const std::string &id,
			const std::string &sender, const long long ms) {
		auto ptr = std::make_unique<RequestMessage>();
		ptr->id = id;
		ptr->sender = sender;
		ptr->timestamp = std::chrono::system_clock::time_point(
				std::chrono::milliseconds(ms));
		std::getline(iss, ptr->filename);
		return ptr;
	}
};
