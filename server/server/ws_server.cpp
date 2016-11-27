#include <fstream>

#include "ws_server.hpp"

ws_server::ws_server()
{
	// set up access channels to only log interesting things
	m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
	m_endpoint.set_access_channels(websocketpp::log::alevel::access_core); //access_core);
	m_endpoint.set_access_channels(websocketpp::log::alevel::app); //app);

	// Initialize the Asio transport policy
	m_endpoint.init_asio();

	// Bind the handlers we are using
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::bind;
	m_endpoint.set_open_handler(bind(&ws_server::on_open, this, _1));
	m_endpoint.set_close_handler(bind(&ws_server::on_close, this, _1));
	m_endpoint.set_http_handler(bind(&ws_server::on_http, this, _1));
}

void ws_server::run(std::string docroot, uint16_t port) {
	std::stringstream ss;
	ss << "Running telemetry server on port " << port << " using docroot=" << docroot;
	m_endpoint.get_alog().write(websocketpp::log::alevel::app, ss.str());

	m_docroot = docroot;

	// listen on specified port
	m_endpoint.listen(port);

	// Start the server accept loop
	m_endpoint.start_accept();

	// Set the initial timer to start telemetry
	//set_timer();

	// Start the ASIO io_service run loop
	try {
		m_endpoint.run();
	}
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
	}
}

void ws_server::on_http(connection_hdl hdl) {
	// Upgrade our connection handle to a full connection_ptr
	server::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);

	std::ifstream file;
	std::string filename = con->get_resource();
	std::string response;

	m_endpoint.get_alog().write(websocketpp::log::alevel::app,
		"http request1: " + filename);

	if (filename == "/") {
		filename = m_docroot + "index.html";
	}
	else {
		filename = m_docroot + filename.substr(1);
	}

	m_endpoint.get_alog().write(websocketpp::log::alevel::app,
		"http request2: " + filename);

	file.open(filename.c_str(), std::ios::in);
	if (!file) {
		// 404 error
		std::stringstream ss;

		ss << "<!doctype html><html><head>"
			<< "<title>Error 404 (Resource not found)</title><body>"
			<< "<h1>Error 404</h1>"
			<< "<p>The requested URL " << filename << " was not found on this server.</p>"
			<< "</body></head></html>";

		con->set_body(ss.str());
		con->set_status(websocketpp::http::status_code::not_found);
		return;
	}

	file.seekg(0, std::ios::end);
	response.reserve(file.tellg());
	file.seekg(0, std::ios::beg);

	response.assign((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	con->set_body(response);
	con->set_status(websocketpp::http::status_code::ok);
}

void ws_server::on_open(connection_hdl hdl) {
	m_conn_manager.m_connections.insert(hdl);
}

void ws_server::on_close(connection_hdl hdl) {
	m_conn_manager.m_connections.erase(hdl);
}