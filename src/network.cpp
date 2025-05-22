//
// Created by ilya on 22.05.2025.
//

#include "network.hpp"

#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>

using boost::asio::ip::tcp;

Network::Network(Peer &self, const std::string &address,
				 const unsigned short port) :
	self_(self), ctx_(),
	acceptor_(ctx_,
			  tcp::endpoint(boost::asio::ip::make_address(address), port)),
	pool_(4) {
	acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
	acceptor_.listen();
}

void Network::start() {
	accept();
	std::thread([this] { gossip(); }).detach();
	ctx_.run();
}

void Network::accept() {
	acceptor_.async_accept([this](const boost::system::error_code &ec,
								  tcp::socket socket) {
		if (!ec) {
			auto socket_ptr = std::make_shared<tcp::socket>(std::move(socket));
			pool_.enqueue([this, socket_ptr] { handle(socket_ptr); });
		}
		accept();
	});
}

void Network::handle(std::shared_ptr<tcp::socket> socket) {
	try {
		boost::asio::streambuf input;
		boost::asio::read_until(*socket, input, '\n');
		std::istream is(&input);
		std::string raw;
		std::getline(is, raw);
		auto message = Message::deserialize(raw);

		if (message->sender == self_.address()) {
			// std::lock_guard<std::mutex> lock(mutex_);
			// messages_.push_back(message->clone());
			return;
		}

		switch (message->type) {
			case MessageType::TEXT: {
				auto *t = static_cast<TextMessage *>(message.get());
				{
					std::lock_guard<std::mutex> lock(mutex_);
					messages_.push_back(message->clone());
				}
				std::cout << std::format("[{}] {}", t->sender, t->text)
						  << std::endl;
				self_.add_peer(t->sender);
				break;
			}
			case MessageType::FILE: {
				auto *f = static_cast<FileMessage *>(message.get());
				{
					std::lock_guard<std::mutex> lock(mutex_);
					messages_.push_back(message->clone());
				}
				std::ofstream out(f->filename, std::ios::binary);
				out.write(f->bytes.data(), f->bytes.size());
				std::cout << std::format("+ {} ({} bytes) (from {})",
										 f->filename, f->bytes.size(),
										 f->sender)
						  << std::endl;
				self_.add_peer(f->sender);
				break;
			}
			case MessageType::REQUEST: {
				auto *req = static_cast<RequestMessage *>(message.get());
				std::ifstream in(req->filename, std::ios::binary);
				if (in) {
					FileMessage resp;
					resp.id = Message::makeuid();
					resp.sender = self_.address();
					resp.timestamp = std::chrono::system_clock::now();
					resp.filename = req->filename;
					std::vector<char> buf(8192);
					while (in.read(buf.data(), buf.size()) || in.gcount() > 0) {
						resp.bytes.insert(resp.bytes.end(), buf.data(),
										  buf.data() + in.gcount());
					}
					send(req->sender, resp.serialize() + '\n');
				}
				break;
			}
		}
	} catch (std::exception &e) {
		std::cerr << "error: " << e.what() << std::endl;
	}
}

void Network::broadcast(const Message &message) {
	for (auto &peer: self_.get_peers()) {
		send(peer, message.serialize() + '\n');
	}
}

void Network::send(const std::string &address, const std::string &data) {
	const auto pos = address.find(':');
	const auto host = address.substr(0, pos);
	const auto port =
			static_cast<unsigned short>(std::stoi(address.substr(pos + 1)));

	auto socket = std::make_shared<tcp::socket>(ctx_);
	const tcp::endpoint ep(boost::asio::ip::make_address(host), port);
	socket->async_connect(ep, [socket, data](auto ec) {
		if (!ec) {
			boost::asio::async_write(*socket, boost::asio::buffer(data),
									 [socket](auto, auto) {});
		}
	});
}

void Network::gossip() {
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(2));
		std::vector<std::unique_ptr<Message> > copy;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto &message: messages_) {
				copy.push_back(message->clone());
			}
		}
		for (const auto &message: copy) {
			auto &hist = history_[message->id];
			for (auto &peer: self_.get_peers()) {
				if (hist.insert(peer).second) {
					send(peer, message->serialize() + '\n');
				}
			}
		}
	}
}
