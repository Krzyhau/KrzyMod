#include "Event.hpp"

#include <map>

std::vector<PluginInitHandler *> PluginInitHandler::handlers;

PluginInitHandler::PluginInitHandler(std::function<void()> cb)
	: cb(cb) {
	handlers.push_back(this);
}

void PluginInitHandler::RunAll() {
	for (auto h : PluginInitHandler::handlers) {
		h->cb();
	}
}
