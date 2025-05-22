//
// Created by ilya on 22.05.2025.
//

#include "peer.hpp"

Peer::Peer(const std::string &address, unsigned short port) : address_(address), port_(port) {
}

std::string Peer::address() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return address_ + ":" + std::to_string(port_);
}

unsigned short Peer::port() const { return port_; }

void Peer::add_peer(const std::string &peer) {
	std::lock_guard<std::mutex> lock(mutex_);
	peers_.insert(peer);
}

std::set<std::string> Peer::get_peers() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return peers_;
}
