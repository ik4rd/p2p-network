//
// Created by ilya on 22.05.2025.
//

#include <../include/message.hpp>
#include <sstream>

std::string Message::serialize() const {
	std::ostringstream oss;
	oss << id << '|' << sender << '|' << timestamp.time_since_epoch().count() << '|' << text;
	return oss.str();
}

Message Message::deserialize(const std::string &data) {
	std::istringstream iss(data);
	Message message;
	long long ts;
	std::getline(iss, message.id, '|');
	std::getline(iss, message.sender, '|');
	iss >> ts;
	iss.get();
	message.timestamp = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(ts));
	std::getline(iss, message.text);
	return message;
}
