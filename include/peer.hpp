//
// Created by ilya on 22.05.2025.
//

#pragma once
#include <mutex>
#include <set>
#include <string>

class Peer {
public:
	Peer(const std::string &address, unsigned short port);

	std::string address() const;

	unsigned short port() const;

	void add_peer(const std::string &peer);

	std::set<std::string> get_peers() const;

private:
	std::string address_;
	unsigned short port_;
	mutable std::mutex mutex_;
	std::set<std::string> peers_;
};
