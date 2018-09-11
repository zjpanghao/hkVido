/*
  A trivial static http webserver using Libevent's evhttp.

  This is not the best code in the world, and it does some fairly stupid stuff
  that you would never want to do in a production webserver. Caveat hackor!

 */

/* Compatibility for possible missing IPv6 declarations */
//#include "../util-internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#endif
#include "event2/http_compat.h"
#include "login.h"
#include "dvr_control.h"
#include "channel.h"

#ifdef _WIN32
#ifndef stat
#define stat _stat
#endif
#ifndef fstat
#define fstat _fstat
#endif
#ifndef open
#define open _open
#endif
#ifndef close
#define close _close
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#endif

char uri_root[512];

static const struct table_entry {
	const char *extension;
	const char *content_type;
} content_type_table[] = {
	{ "txt", "text/plain" },
	{ "c", "text/plain" },
	{ "h", "text/plain" },
	{ "html", "text/html" },
	{ "htm", "text/htm" },
	{ "css", "text/css" },
	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postscript" },
	{ NULL, NULL },
};

/* Try to guess a good content-type for 'path' */
static const char *
guess_content_type(const char *path)
{
	const char *last_period, *extension;
	const struct table_entry *ent;
	last_period = strrchr(path, '.');
	if (!last_period || strchr(last_period, '/'))
		goto not_found; /* no exension */
	extension = last_period + 1;
	for (ent = &content_type_table[0]; ent->extension; ++ent) {
		if (!evutil_ascii_strcasecmp(ent->extension, extension))
			return ent->content_type;
	}

not_found:
	return "application/misc";
}

static void heartBeat(struct evhttp_request *req, void *arg) {
  struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  const char *buffer = evhttp_request_uri(req);
  if (buffer == NULL) {
    evbuffer_add_printf(response, "buffer null");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  evhttp_parse_query(buffer, &keys);
  const char *taskId = evhttp_find_header(&keys, "taskId");
  if (taskId == NULL) {
    evbuffer_add_printf(response, "not contain taskId ");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  
  int task = atoi(taskId);
  if ((rc = getDVRControl().heartBeat(task)) != 0) {
    evbuffer_add_printf(response, "{\"error_code:\"%d}", rc);
  }  else {
    evbuffer_add_printf(response, "{\"task send heart\":%d}", task);
  }
	evhttp_send_reply(req, 200, "OK", response);
}

static void playcontrol(struct evhttp_request *req, void *arg) {
  struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  const char *buffer = evhttp_request_uri(req);
  if (buffer == NULL) {
    evbuffer_add_printf(response, "buffer null");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  evhttp_parse_query(buffer, &keys);
  const char *taskId = evhttp_find_header(&keys, "taskId");
  const char *flagId = evhttp_find_header(&keys, "flag");
  const char *params  = evhttp_find_header(&keys, "param");
  if (taskId == NULL || flagId == NULL) {
    evbuffer_add_printf(response, "not contain taskId or flag");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  
  int task = atoi(taskId);
  int flag = atoi(flagId);
  long param = -1;
  if (params != NULL) {
    param = atol(params);
  }
  if ((rc = getDVRControl().playControl(task, flag, param)) != 0) {
    evbuffer_add_printf(response, "{\"error_code\":%d}", rc);
  }  else {
    evbuffer_add_printf(response, "{\"play control set\":%d,%d, %ld}", task, flag, param);
  }
	evhttp_send_reply(req, 200, "OK", response);
}

static void stopplaytask(struct evhttp_request *req, void *arg) {
  struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  const char *buffer = evhttp_request_uri(req);
  if (buffer == NULL) {
    evbuffer_add_printf(response, "not contain taskId");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  evhttp_parse_query(buffer, &keys);
  const char *taskId = evhttp_find_header(&keys, "taskId");
  
  if (taskId == NULL) {
    evbuffer_add_printf(response, "not contain taskId");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  int task = atoi(taskId);
  if ((rc = getDVRControl().stopPlayTask(task)) != 0) {
    evbuffer_add_printf(response, "{\"error_code\":%d}", rc);
  }  else {
    evbuffer_add_printf(response, "{\"stopplayok\":%d}", task);
  }
	evhttp_send_reply(req, 200, "OK", response);
}

static void playback(struct evhttp_request *req, void *arg) {
  struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  const char *buffer = evhttp_request_uri(req);
  if (buffer == NULL) {
    evbuffer_add_printf(response, "not contain taskId");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  evhttp_parse_query(buffer, &keys);
  const char *taskId = evhttp_find_header(&keys, "taskId");
  const char *userId = evhttp_find_header(&keys, "userId");
  const char *topic = evhttp_find_header(&keys, "topic");
  if (taskId == NULL || userId == NULL || topic == NULL) {
    evbuffer_add_printf(response, "not contain taskId or userId or topic");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }

  const char *startTime = evhttp_find_header(&keys, "startTime");
  const char *endTime = evhttp_find_header(&keys, "endTime");
  const char *channelId = evhttp_find_header(&keys, "channel");
  long start = 0;
  long end = time(NULL);
  int channel = 1;
  int task = atoi(taskId);
  int user = atoi(userId);
  if (startTime != NULL) {
    start = atol(startTime);
  }

  if (endTime != NULL) {
    end = atol(endTime);
  }

  if (channelId != NULL) {
    channel = atol(channelId);
  }

  if (!getDVRControl().hasFilePlay(user, channel, start, end)) {
    evbuffer_add_printf(response, "{\"error_code\":%d}", -100);
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  PlayTask playTask(task, user, channel, start, end);
  if (topic) {
    playTask.setTopic(topic);
  }
  if (!getDVRControl().addTask(&playTask)) {
    evbuffer_add_printf(response, "task already exits or channel error");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  
  if ((rc = getDVRControl().play(task)) != 0) {
    evbuffer_add_printf(response, "{\"error_code\":%d}", rc);
  }  else {
    evbuffer_add_printf(response, "{\"playok\":%d}", task);
  }
	evhttp_send_reply(req, 200, "OK", response);

}

static void playReal(struct evhttp_request *req, void *arg) {
  struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  const char *buffer = evhttp_request_uri(req);
  if (buffer == NULL) {
    evbuffer_add_printf(response, "not contain taskId");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  evhttp_parse_query(buffer, &keys);
  const char *taskId = evhttp_find_header(&keys, "taskId");
  const char *userId = evhttp_find_header(&keys, "userId");
  const char *topic = evhttp_find_header(&keys, "topic");
  const char *streamType = evhttp_find_header(&keys, "stream");
  if (taskId == NULL || userId == NULL) {
    evbuffer_add_printf(response, "not contain taskId or userId");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }

  const char *channelId = evhttp_find_header(&keys, "channel");

  int channel = 1;
  StreamType stream = StreamType::CHILD_STREAM;
  int task = atoi(taskId);
  int user = atoi(userId);
  if (streamType) {
    stream = (StreamType)atoi(streamType);
  }
  
  if (channelId != NULL) {
    channel = atol(channelId);
  }
  PlayTask playTask(task, user, channel);
  if (topic) {
    playTask.setTopic(topic);
  }
  playTask.setStreamType(stream);
  if (!getDVRControl().addTask(&playTask)) {
    evbuffer_add_printf(response, "task already exits");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  
  if ((rc = getDVRControl().play(task)) != 0) {
    evbuffer_add_printf(response, "{\"error_code:\"%d}", rc);
  }  else {
    evbuffer_add_printf(response, "{\"playok\":%d}", task);
  }
	evhttp_send_reply(req, 200, "OK", response);

}

static void showtask(struct evhttp_request *req, void *arg) {
  struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  std::string result;
  result = getDVRControl().getTaskInfo();
  evbuffer_add_printf(response, "{\"info:\"%s}", result.c_str());
	evhttp_send_reply(req, 200, "OK", response);
}

static void showUser(struct evhttp_request *req, void *arg) {
  struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  std::string result;
  result = getLoginControl().showOnLineUser();
  evbuffer_add_printf(response, "{\"info:\"%s}", result.c_str());
	evhttp_send_reply(req, 200, "OK", response);
}
static void showChannel(struct evhttp_request *req, void *arg) {
  struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  std::string result;
  result = getChannelControl().showChannel();
  evbuffer_add_printf(response, "{\"info:\"%s}", result.c_str());
	evhttp_send_reply(req, 200, "OK", response);
}

static void logout(struct evhttp_request *req, void *arg) {
  struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  const char *buffer = evhttp_request_uri(req);
  if (buffer == NULL) {
    evbuffer_add_printf(response, "not contain userId");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  evhttp_parse_query(buffer, &keys);
  const char *userId = evhttp_find_header(&keys, "userId");
  if (userId == NULL) {
    evbuffer_add_printf(response, "not contain userId");
    evhttp_send_reply(req, 200, "OK", response);
    return;
  }
  
  if ((rc = getLoginControl().logout(atoi(userId))) != 0) {
    evbuffer_add_printf(response, "{\"error_code:\"%d}", rc);
  }  else {
    evbuffer_add_printf(response, "{\"logout\":%d}", atoi(userId));
  }
	evhttp_send_reply(req, 200, "OK", response);
}

/* Callback used for the /dump URI, and for every non-GET request:
 * dumps all information to stdout and gives back a trivial 200 ok */
static void
login_request_cb(struct evhttp_request *req, void *arg)
{
	struct evkeyvalq keys;
  int rc = 0;
  evbuffer *response = evbuffer_new();
  const char *buffer = evhttp_request_uri(req);
  const char *userName = NULL;
  const char *pass = NULL;
  const char *ip = NULL;
  const char *port = NULL;
  if (buffer != NULL) {
    evhttp_parse_query(buffer, &keys);
    userName = evhttp_find_header(&keys, "user");
    pass = evhttp_find_header(&keys, "password");
    ip = evhttp_find_header(&keys, "ip");
    port = evhttp_find_header(&keys, "port");
  }
  
  SDKUser hkUser = { userName ? userName : "admin", 
                    pass ? pass : "ky221data", 
                    ip ? ip : "192.168.2.3", 
                    port ? atoi(port) : 8000
                    };
  if ((rc = getLoginControl().login(&hkUser)) != 0) {
    evbuffer_add_printf(response, "{\"error_code:\"%d}", rc);
  }  else {
    evbuffer_add_printf(response, "{\"user_id\":%d}", hkUser.id);
  }
	evhttp_send_reply(req, 200, "OK", response);
}

/* This callback gets invoked when we get any http request that doesn't match
 * any other callback.  Like any evhttp server callback, it has a simple job:
 * it must eventually call evhttp_send_error() or evhttp_send_reply().
 */
static void
send_document_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = NULL;
	const char *docroot = (const char*)arg;
	const char *uri = evhttp_request_get_uri(req);
	struct evhttp_uri *decoded = NULL;
	const char *path;
	char *decoded_path;
	char *whole_path = NULL;
	size_t len;
	int fd = -1;
	struct stat st;

	if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
		login_request_cb(req, arg);
		return;
	}

	printf("Got a GET request for <%s>\n",  uri);

	/* Decode the URI */
	decoded = evhttp_uri_parse(uri);
	if (!decoded) {
		printf("It's not a good URI. Sending BADREQUEST\n");
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	/* Let's see what path the user asked for. */
	path = evhttp_uri_get_path(decoded);
	if (!path) path = "/";

	/* We need to decode it, to see what path the user really wanted. */
	decoded_path = evhttp_uridecode(path, 0, NULL);
	if (decoded_path == NULL)
		goto err;
	/* Don't allow any ".."s in the path, to avoid exposing stuff outside
	 * of the docroot.  This test is both overzealous and underzealous:
	 * it forbids aceptable paths like "/this/one..here", but it doesn't
	 * do anything to prevent symlink following." */
	if (strstr(decoded_path, ".."))
		goto err;

	len = strlen(decoded_path)+strlen(docroot)+2;
	if (!(whole_path = (char*)malloc(len))) {
		perror("malloc");
		goto err;
	}
	evutil_snprintf(whole_path, len, "%s/%s", docroot, decoded_path);

	if (stat(whole_path, &st)<0) {
		goto err;
	}

	/* This holds the content we're sending. */
	evb = evbuffer_new();

	if (S_ISDIR(st.st_mode)) {
		/* If it's a directory, read the comments and make a little
		 * index page */
#ifdef _WIN32
		HANDLE d;
		WIN32_FIND_DATAA ent;
		char *pattern;
		size_t dirlen;
#else
		DIR *d;
		struct dirent *ent;
#endif
		const char *trailing_slash = "";

		if (!strlen(path) || path[strlen(path)-1] != '/')
			trailing_slash = "/";

#ifdef _WIN32
		dirlen = strlen(whole_path);
		pattern = (char*)malloc(dirlen+3);
		memcpy(pattern, whole_path, dirlen);
		pattern[dirlen] = '\\';
		pattern[dirlen+1] = '*';
		pattern[dirlen+2] = '\0';
		d = FindFirstFileA(pattern, &ent);
		free(pattern);
		if (d == INVALID_HANDLE_VALUE)
			goto err;
#else
		if (!(d = opendir(whole_path)))
			goto err;
#endif

		evbuffer_add_printf(evb,
                    "<!DOCTYPE html>\n"
                    "<html>\n <head>\n"
                    "  <meta charset='utf-8'>\n"
		    "  <title>%s</title>\n"
		    "  <base href='%s%s'>\n"
		    " </head>\n"
		    " <body>\n"
		    "  <h1>%s</h1>\n"
		    "  <ul>\n",
		    decoded_path, /* XXX html-escape this. */
		    path, /* XXX html-escape this? */
		    trailing_slash,
		    decoded_path /* XXX html-escape this */);
#ifdef _WIN32
		do {
			const char *name = ent.cFileName;
#else
		while ((ent = readdir(d))) {
			const char *name = ent->d_name;
#endif
			evbuffer_add_printf(evb,
			    "    <li><a href=\"%s\">%s</a>\n",
			    name, name);/* XXX escape this */
#ifdef _WIN32
		} while (FindNextFileA(d, &ent));
#else
		}
#endif
		evbuffer_add_printf(evb, "</ul></body></html>\n");
#ifdef _WIN32
		FindClose(d);
#else
		closedir(d);
#endif
		evhttp_add_header(evhttp_request_get_output_headers(req),
		    "Content-Type", "text/html");
	} else {
		/* Otherwise it's a file; add it to the buffer to get
		 * sent via sendfile */
		const char *type = guess_content_type(decoded_path);
		if ((fd = open(whole_path, O_RDONLY)) < 0) {
			perror("open");
			goto err;
		}

		if (fstat(fd, &st)<0) {
			/* Make sure the length still matches, now that we
			 * opened the file :/ */
			perror("fstat");
			goto err;
		}
		evhttp_add_header(evhttp_request_get_output_headers(req),
		    "Content-Type", type);
		evbuffer_add_file(evb, fd, 0, st.st_size);
	}

	evhttp_send_reply(req, 200, "OK", evb);
	goto done;
err:
	evhttp_send_error(req, 404, "Document was not found");
	if (fd>=0)
		close(fd);
done:
	if (decoded)
		evhttp_uri_free(decoded);
	if (decoded_path)
		free(decoded_path);
	if (whole_path)
		free(whole_path);
	if (evb)
		evbuffer_free(evb);
}

static void
syntax(void)
{
	fprintf(stdout, "Syntax: http-server <docroot>\n");
}

int ev_server_start(int port)
{
	struct event_base *base;
	struct evhttp *http;
	struct evhttp_bound_socket *handle;

#ifdef _WIN32
	WSADATA WSAData;
	WSAStartup(0x101, &WSAData);
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return (1);
#endif

	base = event_base_new();
	if (!base) {
		fprintf(stderr, "Couldn't create an event_base: exiting\n");
		return 1;
	}

	/* Create a new evhttp object to handle requests. */
	http = evhttp_new(base);
	if (!http) {
		fprintf(stderr, "couldn't create evhttp. Exiting.\n");
		return 1;
	}

	/* The /dump URI will dump all requests to stdout and say 200 ok. */
	evhttp_set_cb(http, "/login", login_request_cb, NULL);
  evhttp_set_cb(http, "/logout", logout, NULL);
  evhttp_set_cb(http, "/playback", playback, NULL);
  evhttp_set_cb(http, "/playcontrol", playcontrol, NULL);
  evhttp_set_cb(http, "/stopplayback", stopplaytask, NULL);
  evhttp_set_cb(http, "/showtask", showtask, NULL);
  evhttp_set_cb(http, "/showchannel", showChannel, NULL);
   evhttp_set_cb(http, "/showuser", showUser, NULL);
  evhttp_set_cb(http, "/heart", heartBeat, NULL);

  evhttp_set_cb(http, "/playreal", playReal, NULL);


	/* We want to accept arbitrary requests, so we need to set a "generic"
	 * cb.  We can also add callbacks for specific paths. */
	//evhttp_set_gencb(http, send_document_cb, argv[1]);

	/* Now we tell the evhttp what port to listen on */
	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	if (!handle) {
		fprintf(stderr, "couldn't bind to port %d. Exiting.\n",
		    (int)port);
		return 1;
	}

	{
		/* Extract and display the address we're listening on. */
		struct sockaddr_storage ss;
		evutil_socket_t fd;
		ev_socklen_t socklen = sizeof(ss);
		char addrbuf[128];
		void *inaddr;
		const char *addr;
		int got_port = -1;
		fd = evhttp_bound_socket_get_fd(handle);
		memset(&ss, 0, sizeof(ss));
		if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
			perror("getsockname() failed");
			return 1;
		}
		if (ss.ss_family == AF_INET) {
			got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
			inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
		} else if (ss.ss_family == AF_INET6) {
			got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
			inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
		} else {
			fprintf(stderr, "Weird address family %d\n",
			    ss.ss_family);
			return 1;
		}
		addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
		    sizeof(addrbuf));
		if (addr) {
			printf("Listening on %s:%d\n", addr, got_port);
			evutil_snprintf(uri_root, sizeof(uri_root),
			    "http://%s:%d",addr,got_port);
		} else {
			fprintf(stderr, "evutil_inet_ntop failed\n");
			return 1;
		}
	}

	event_base_dispatch(base);

	return 0;
}
