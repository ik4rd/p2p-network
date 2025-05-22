//
// Created by ilya on 22.05.2025.
//

#pragma once
#include <boost/asio.hpp>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

#include "message.hpp"
#include "peer.hpp"
#include "thread_pool.hpp"

class Network {
public:
	Network(Peer &self, const std::string &address, unsigned short port);

	void start();

	void broadcast(const Message &message);

private:
	void accept();

	void gossip();

	void handle(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

	void send(const std::string &address, const std::string &data);

	Peer &self_;
	boost::asio::io_context ctx_;
	boost::asio::ip::tcp::acceptor acceptor_;

	Pool pool_;

	std::vector<std::unique_ptr<Message> > messages_;
	std::unordered_map<std::string, std::set<std::string> > history_;
	std::mutex mutex_;
};
