//
// Created by ilya on 22.05.2025.
//

#include "message.hpp"

std::unique_ptr<Message> Message::deserialize(const std::string &data) {
	std::istringstream iss(data);
	MessageType type;
	std::string id;
	std::string sender;
	long long ts;

	deserialize_header(iss, type, id, sender, ts);

	if (type == MessageType::TEXT) {
		return TextMessage::from_stream(iss, id, sender, ts);
	}

	if (type == MessageType::FILE) {
		std::streampos payload = iss.tellg();
		return FileMessage::from_stream(data, id, sender, ts, payload);
	}

	return nullptr;
}
