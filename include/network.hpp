//
// Created by ilya on 22.05.2025.
//

#pragma once
#include "message.hpp"
#include "peer.hpp"

#include <boost/asio.hpp>
#include <thread>

class Network {
public:
	Network(Peer &self, const std::string &address, unsigned short port);

	void start();

	void broadcast(const Message &message);

private:
	void accept();

	void gossip();

	void handle(boost::asio::ip::tcp::socket socket);

	void send(const std::string &address, const string &data);

	Peer &self_;
	boost::asio::io_context ctx_;
	boost::asio::ip::tcp::acceptor acceptor_;
	std::vector<std::thread> threads_;
	std::vector<Message> messages_;
	std::mutex mutex_;
};
