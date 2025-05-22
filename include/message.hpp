//
// Created by ilya on 22.05.2025.
//

#pragma once

#include <chrono>
#include <string>

struct Message {
	std::string id;
	std::string sender;
	std::string text;
	std::chrono::system_clock::time_point timestamp;

	std::string serialize() const;
	static Message deserialize(const std::string &data);
};
