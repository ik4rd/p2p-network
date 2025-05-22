//
// Created by ilya on 22.05.2025.
//

#include "message.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace boost::uuids;

std::unique_ptr<Message> Message::deserialize(const std::string &data) {
	std::istringstream iss(data);
	MessageType type;
	std::string id;
	std::string sender;
	long long ms;

	deserialize_header(iss, type, id, sender, ms);

	if (type == MessageType::TEXT) {
		return TextMessage::from_stream(iss, id, sender, ms);
	}

	if (type == MessageType::FILE) {
		const std::streampos payload = iss.tellg();
		return FileMessage::from_stream(data, id, sender, ms, payload);
	}

	if (type == MessageType::REQUEST) {
		return RequestMessage::from_stream(iss, id, sender, ms);
	}

	return nullptr;
}

std::string Message::makeuid() {
	static random_generator gen;
	const uuid u = gen();
	return to_string(u);
}
