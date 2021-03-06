// Copyright (c) 2012 Cloudera, Inc. All rights reserved.
#include <stdio.h>
#include <signal.h>
#include <string>
#include <map>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>

#include "common/logging.h"
#include "util/webserver.h"
#include "util/logging.h"

using namespace std;
using namespace boost;
using namespace google;

DEFINE_int32(webserver_port, 25000, "Port to start debug webserver on");
DEFINE_string(webserver_interface, "",
    "Interface to start debug webserver on. If blank, webserver binds to 0.0.0.0");

namespace impala {

Webserver::Webserver()
  : port_(FLAGS_webserver_port),
    interface_(FLAGS_webserver_interface),
    context_(NULL) {

}

Webserver::Webserver(const string& interface, const int port)
  : port_(port),
    interface_(interface) {

}

Webserver::~Webserver() {
  Stop();
}

void Webserver::RootHandler(stringstream* output) {
  // path_handler_lock_ already held by MongooseCallback
  BOOST_FOREACH(const PathHandlerMap::value_type& handler, path_handlers_) {
    (*output) << "<a href=\"" << handler.first << "\">" << handler.first << "</a><br/>";
  }
}

Status Webserver::Start() {
  LOG(INFO) << "Starting webserver on " << interface_
              << (interface_.empty() ? "all interfaces, port " : ":") << port_;

  string port_as_string = boost::lexical_cast<string>(port_);
  stringstream listening_spec;
  listening_spec << interface_ << (interface_.empty() ? "" : ":") << port_as_string;
  string listening_str = listening_spec.str();
  const char* options[] = {"listening_ports", listening_str.c_str(), NULL};

  // mongoose ignores SIGCHLD and we need it to run kinit. This means that since
  // mongoose does not reap its own children CGI programs must be avoided.
  // Save the signal handler so we can restore it after mongoose sets it to be ignored.
  sighandler_t sig_chld = signal(SIGCHLD, SIG_DFL);

  // To work around not being able to pass member functions as C callbacks, we store a
  // pointer to this server in the per-server state, and register a static method as the
  // default callback. That method unpacks the pointer to this and calls the real
  // callback.
  context_ = mg_start(&Webserver::MongooseCallbackStatic, reinterpret_cast<void*>(this),
      options);

  // Restore the child signal handler so wait() works properly.
  signal(SIGCHLD, sig_chld);

  if (context_ == NULL) {
    stringstream error_msg;
    error_msg << "Could not start webserver on port: " << port_;
    return Status(error_msg.str());
  }

  PathHandlerCallback default_callback =
      bind<void>(mem_fn(&Webserver::RootHandler), this, _1);

  RegisterPathHandler("/", default_callback);

  LOG(INFO) << "Webserver started";
  return Status::OK;
}

void Webserver::Stop() {
  if (context_ != NULL) mg_stop(context_);
}

void* Webserver::MongooseCallbackStatic(enum mg_event event,
    struct mg_connection* connection, const struct mg_request_info* request_info) {
  Webserver* instance = reinterpret_cast<Webserver*>(request_info->user_data);
  return instance->MongooseCallback(event, connection, request_info);
}

// Mongoose requires a non-null return from the callback to signify successful processing
static void* PROCESSING_COMPLETE = reinterpret_cast<void*>(1);

void* Webserver::MongooseCallback(enum mg_event event, struct mg_connection* connection,
    const struct mg_request_info* request_info) {
  if (event == MG_NEW_REQUEST) {
    mutex::scoped_lock lock(path_handlers_lock_);
    PathHandlerMap::const_iterator it = path_handlers_.find(request_info->uri);
    if (it == path_handlers_.end()) {
      mg_printf(connection, "HTTP/1.1 404 Not Found\r\n"
                            "Content-Type: text/plain\r\n\r\n");
      mg_printf(connection, "No handler for URI %s\r\n\r\n", request_info->uri);
      return PROCESSING_COMPLETE;
    }

    // TODO: Consider adding simple HTML boilerplate, e.g. <html><body> ... </body></html>
    stringstream output;
    BOOST_FOREACH(const PathHandlerCallback& callback, it->second) {
      callback(&output);
    }

    string str = output.str();
    mg_printf(connection, "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "\r\n", (int)str.length());

    // Make sure to use mg_write for printing the body; mg_printf truncates at 8kb
    mg_write(connection, str.c_str(), str.length());
    return PROCESSING_COMPLETE;
  } else {
    return NULL;
  }
}

void Webserver::RegisterPathHandler(const std::string& path,
   const PathHandlerCallback& callback) {
  mutex::scoped_lock lock(path_handlers_lock_);
  // operator [] constructs an entry if one does not exist
  path_handlers_[path].push_back(callback);
}

}
