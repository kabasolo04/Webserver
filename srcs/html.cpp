#include "WebServer.hpp"

std::string	buildAutoindexHtml(std::string uri, DIR *dir)
{
	std::ostringstream html;

	html << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
	<< "<title>🔥 Index of " << uri << " 🔥</title>"

	// ====== STYLE ======
	<< "<style>"
	"body {"
		"margin:0;"
		"font-family: Arial, sans-serif;"
		"background: #000;"
		"color: #fff;"
		"text-shadow: 0 0 5px #f30;"
		"animation: pulse 3s infinite;"
	"}"
	"h1 {"
		"text-align:center;"
		"padding:20px;"
		"font-size:40px;"
		"color:#ff4500;"
		"text-shadow: 0 0 15px #ff0000, 0 0 5px #ff8c00;"
	"}"
	"pre {"
		"max-width:900px;"
		"margin:auto;"
		"padding:20px;"
		"background:rgba(0,0,0,0.6);"
		"border:solid 1px #f30;"
		"border-radius:10px;"
		"box-shadow:0 0 20px #f30;"
	"}"
	"a {"
		"color:#ffb380;"
		"font-size:20px;"
		"text-decoration:none;"
	"}"
	"a:hover {"
		"color:#fff;"
		"text-shadow: 0 0 10px #ff4500;"
	"}"
	"@keyframes pulse {"
		"0% { background-color: #000; }"
		"50% { background-color: #200; }"
		"100% { background-color: #000; }"
	"}"
	// Mini llamas arriba
	".firebar {"
		"text-align:center;"
		"font-size:25px;"
		"margin:0;"
		"padding:10px 0;"
		"animation: flame 1s infinite;"
	"}"
	"@keyframes flame {"
		"0% { text-shadow: 0 0 5px #f00; }"
		"50% { text-shadow: 0 0 15px #ff8000; }"
		"100% { text-shadow: 0 0 5px #f00; }"
	"}"
	"</style>"
	<< "</head><body>"

	// ====== FIRE BAR ======
	<< "<div class=\"firebar\">🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥</div>"

	<< "<h1>🔥 Index of " << uri << " 🔥</h1>"
	<< "<pre>";

	if (uri != "./")
		html << "<a href=\"..\">⟵ Back</a>\n";

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;

		if (name[0] == '.')
		continue;

		std::string fullPath = uri + "/" + name;
		struct stat st;
		if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
			name += "/";

		std::string baseUri = uri.substr(6);
		if (baseUri[baseUri.size() - 1] != '/')
			baseUri += '/';
		if (baseUri[0] != '/' )
			baseUri = '/' + baseUri;

		html << "<a href=\"" << baseUri << name << "\">" << name << "</a>\n";
	}

	html << "</pre></body></html>";
	return html.str();
}

std::string buildErrorHtml(StatusCode code)
{
	std::stringstream temp;
	temp <<
		"<!doctype html>\n"
		"<html lang=\"en\">\n"
		"<head>\n"
		"  <meta charset=\"utf-8\" />\n"
		"  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\" />\n"
		"  <title>" << code << " " << getReasonPhrase(code) << "</title>\n"
		"  <style>\n"
		"    :root{\n"
		"      --bg1:#0f1724; --bg2:#132033; --card:#0b1220; --accent:#7c5cff; --muted:#9aa6b2;\n"
		"    }\n"
		"    *{box-sizing:border-box}\n"
		"    body{\n"
		"      margin:0; height:100vh; display:flex; align-items:center; justify-content:center;\n"
		"      background:linear-gradient(135deg,var(--bg1),var(--bg2));\n"
		"      color:#e6eef6; font-family:Inter,system-ui,Arial;\n"
		"      padding:32px; -webkit-font-smoothing:antialiased;\n"
		"    }\n"
		"    .card{\n"
		"      width:min(900px,95%);\n"
		"      display:grid; grid-template-columns:1fr 350px; gap:28px;\n"
		"      background:rgba(255,255,255,0.02); border-radius:20px;\n"
		"      padding:32px; box-shadow:0 10px 30px rgba(0,0,0,0.5);\n"
		"      border:1px solid rgba(255,255,255,0.05);\n"
		"    }\n"
		"    .code{ font-size:64px; font-weight:800; margin:0; color:var(--accent); }\n"
		"    .reason{ color:var(--muted); font-size:22px; margin-top:4px; }\n"
		"    .msg{ line-height:1.6; color:#dbe9f8; margin-top:16px; }\n"
		"    .btn{\n"
		"      display:inline-block; margin-top:20px; padding:10px 16px;\n"
		"      border-radius:10px; text-decoration:none; color:#e6eef6;\n"
		"      border:1px solid rgba(255,255,255,0.07);\n"
		"      transition:0.2s ease; font-weight:600;\n"
		"    }\n"
		"    .btn:hover{ transform:translateY(-3px); box-shadow:0 8px 24px rgba(0,0,0,0.4); }\n"
		"    @media(max-width:900px){ .card{ grid-template-columns:1fr; text-align:center; } }\n"
		"  </style>\n"
		"</head>\n"
		"<body>\n"
		"  <main class=\"card\">\n"
		"    <section>\n"
		"      <h1 class=\"code\">" << code << "</h1>\n"
		"      <div class=\"reason\">" << getReasonPhrase(code) << "</div>\n"
		"      <p class=\"msg\">\n"
		"        Something went wrong on the server. The response was <strong>" << code << " " << getReasonPhrase(code) << "</strong>.\n"
		"      </p>\n"
		"      <a class=\"btn\" href=\"/\">Return to homepage</a>\n"
		"    </section>\n"
		"    <aside>\n"
		"      <!-- You can replace this SVG with any graphic you want -->\n"
		"      <svg width=\"100%\" height=\"100%\" viewBox=\"0 0 600 600\">\n"
		"        <defs>\n"
		"          <linearGradient id=\"g\" x1=\"0\" x2=\"1\">\n"
		"            <stop offset=\"0\" stop-color=\"#7c5cff\"/>\n"
		"            <stop offset=\"1\" stop-color=\"#5be7c8\"/>\n"
		"          </linearGradient>\n"
		"        </defs>\n"
		"        <ellipse cx=\"300\" cy=\"520\" rx=\"130\" ry=\"18\" fill=\"rgba(0,0,0,0.45)\"/>\n"
		"        <rect x=\"160\" y=\"160\" width=\"280\" height=\"260\" rx=\"28\" fill=\"#071022\" stroke=\"url(#g)\" stroke-width=\"4\"/>\n"
		"        <circle cx=\"270\" cy=\"250\" r=\"14\" fill=\"#eaf6ff\" />\n"
		"        <circle cx=\"330\" cy=\"250\" r=\"14\" fill=\"#eaf6ff\" />\n"
		"        <rect x=\"270\" y=\"290\" width=\"60\" height=\"8\" rx=\"4\" fill=\"#8fb9ff\" />\n"
		"        <rect x=\"292\" y=\"110\" width=\"16\" height=\"60\" rx=\"8\" fill=\"url(#g)\"/>\n"
		"        <circle cx=\"300\" cy=\"100\" r=\"14\" fill=\"#ffd9f2\" />\n"
		"      </svg>\n"
		"    </aside>\n"
		"  </main>\n"
		"</body>\n"
		"</html>\n";

	return (temp.str());
}

std::string buildSuccesHtml(StatusCode code)
{
	std::stringstream html;
	std::string phrase = getReasonPhrase(code);

	html << "<!DOCTYPE html>\n"
		 << "<html lang=\"en\">\n"
		 << "<head>\n"
		 << "\t<meta charset=\"UTF-8\">\n"
		 << "\t<title>"
		 << code << " " << phrase
		 << "</title>\n"
		 << "\t<style>\n"
		 << "\t\tbody {\n"
		 << "\t\t\tmargin: 0;\n"
		 << "\t\t\tfont-family: Arial, sans-serif;\n"
		 << "\t\t\tbackground: linear-gradient(135deg, #ff9a9e, #fad0c4);\n"
		 << "\t\t\tdisplay: flex;\n"
		 << "\t\t\tjustify-content: center;\n"
		 << "\t\t\talign-items: center;\n"
		 << "\t\t\theight: 100vh;\n"
		 << "\t\t}\n"
		 << "\t\t.card {\n"
		 << "\t\t\tbackground: white;\n"
		 << "\t\t\tpadding: 50px;\n"
		 << "\t\t\tborder-radius: 20px;\n"
		 << "\t\t\tbox-shadow: 0 20px 40px rgba(0,0,0,0.2);\n"
		 << "\t\t\ttext-align: center;\n"
		 << "\t\t\tmax-width: 500px;\n"
		 << "\t\t}\n"
		 << "\t\th1 {\n"
		 << "\t\t\tmargin: 0;\n"
		 << "\t\t\tfont-size: 48px;\n"
		 << "\t\t\tcolor: #ff5f8f;\n"
		 << "\t\t}\n"
		 << "\t\tp {\n"
		 << "\t\t\tmargin-top: 20px;\n"
		 << "\t\t\tfont-size: 18px;\n"
		 << "\t\t\tcolor: #555;\n"
		 << "\t\t}\n"
		 << "\t\t.badge {\n"
		 << "\t\t\tdisplay: inline-block;\n"
		 << "\t\t\tmargin-top: 30px;\n"
		 << "\t\t\tpadding: 10px 20px;\n"
		 << "\t\t\tbackground: #ff5f8f;\n"
		 << "\t\t\tcolor: white;\n"
		 << "\t\t\tborder-radius: 999px;\n"
		 << "\t\t\tfont-weight: bold;\n"
		 << "\t\t\tletter-spacing: 1px;\n"
		 << "\t\t}\n"
		 << "\t\t.back-btn {\n"
		 << "\t\t\tdisplay: inline-block;\n"
		 << "\t\t\tmargin-top: 40px;\n"
		 << "\t\t\tpadding: 12px 30px;\n"
		 << "\t\t\tbackground: #ffd1dc;\n"
		 << "\t\t\tcolor: #ff5f8f;\n"
		 << "\t\t\ttext-decoration: none;\n"
		 << "\t\t\tfont-weight: bold;\n"
		 << "\t\t\tborder-radius: 999px;\n"
		 << "\t\t\ttransition: transform 0.1s ease, box-shadow 0.1s ease;\n"
		 << "\t\t}\n"
		 << "\t\t.back-btn:hover {\n"
		 << "\t\t\ttransform: translateY(-2px);\n"
		 << "\t\t\tbox-shadow: 0 5px 15px rgba(0,0,0,0.2);\n"
		 << "\t\t}\n"
		 << "\t</style>\n"
		 << "</head>\n"
		 << "<body>\n"
		 << "\t<div class=\"card\">\n"
		 << "\t\t<h1>"
		 << code << "</h1>\n"
		 << "\t\t<p><strong>"
		 << phrase
		 << "</strong></p>\n"
		 << "\t\t<p>Everything went amazing 💖<br>Like, literally iconic.</p>\n"
		 << "\t\t<div class=\"badge\">SUCCESS ✨</div>\n"
		 << "\t\t<br>\n"
		 << "\t\t<a class=\"back-btn\" href=\"/\">Go back</a>\n"
		 << "\t</div>\n"
		 << "</body>\n"
		 << "</html>\n";

	return html.str();
}

std::string getReasonPhrase(StatusCode code)
{
	switch (code)
	{
		// 2xx – Success
		case OK:								return "OK";
		case CREATED:							return "Created";
		case ACCEPTED:							return "Accepted";
		case NON_AUTHORITATIVE_INFORMATION:		return "Non-Authoritative Information";
		case NO_CONTENT:						return "No Content";
		case RESET_CONTENT:						return "Reset Content";
		case PARTIAL_CONTENT:					return "Partial Content";
		case MULTI_STATUS:						return "Multi-Status";
		case ALREADY_REPORTED:					return "Already Reported";
		case IM_USED:							return "IM Used";

		// 3xx – Redirection
		case MULTIPLE_CHOICES:					return "Multiple Choices";
		case MOVED_PERMANENTLY:					return "Moved Permanently";
		case FOUND:								return "Found";
		case SEE_OTHER:							return "See Other";
		case NOT_MODIFIED:						return "Not Modified";
		case USE_PROXY:							return "Use Proxy";
		case TEMPORARY_REDIRECT:				return "Temporary Redirect";
		case PERMANENT_REDIRECT:				return "Permanent Redirect";

		// 4xx – Client Errors
		case BAD_REQUEST:						return "Bad Request";
		case UNAUTHORIZED:						return "Unauthorized";
		case PAYMENT_REQUIRED:					return "Payment Required";
		case FORBIDDEN:							return "Forbidden";
		case NOT_FOUND:							return "Not Found";
		case METHOD_NOT_ALLOWED:				return "Method Not Allowed";
		case NOT_ACCEPTABLE:					return "Not Acceptable";
		case PROXY_AUTHENTICATION_REQUIRED:		return "Proxy Authentication Required";
		case REQUEST_TIMEOUT:					return "Request Timeout";
		case CONFLICT:							return "Conflict";
		case GONE:								return "Gone";
		case LENGTH_REQUIRED:					return "Length Required";
		case PRECONDITION_FAILED:				return "Precondition Failed";
		case PAYLOAD_TOO_LARGE:					return "Payload Too Large";
		case URI_TOO_LONG:						return "URI Too Long";
		case UNSUPPORTED_MEDIA_TYPE:			return "Unsupported Media Type";
		case RANGE_NOT_SATISFIABLE:				return "Range Not Satisfiable";
		case EXPECTATION_FAILED:				return "Expectation Failed";
		case IM_A_TEAPOT:						return "I'm a teapot";
		case MISDIRECTED_REQUEST:				return "Misdirected Request";
		case UNPROCESSABLE_ENTITY:				return "Unprocessable Entity";
		case LOCKED:							return "Locked";
		case FAILED_DEPENDENCY:					return "Failed Dependency";
		case TOO_EARLY:							return "Too Early";
		case UPGRADE_REQUIRED:					return "Upgrade Required";
		case PRECONDITION_REQUIRED:				return "Precondition Required";
		case TOO_MANY_REQUESTS:					return "Too Many Requests";
		case REQUEST_HEADER_FIELDS_TOO_LARGE:	return "Request Header Fields Too Large";
		case UNAVAILABLE_FOR_LEGAL_REASONS:		return "Unavailable For Legal Reasons";

		// 5xx – Server Errors
		case INTERNAL_SERVER_ERROR:				return "Internal Server Error";
		case NOT_IMPLEMENTED:					return "Not Implemented";
		case BAD_GATEWAY:						return "Bad Gateway";
		case SERVICE_UNAVAILABLE:				return "Service Unavailable";
		case GATEWAY_TIMEOUT:					return "Gateway Timeout";
		case HTTP_VERSION_NOT_SUPPORTED:		return "HTTP Version Not Supported";
		case VARIANT_ALSO_NEGOTIATES:			return "Variant Also Negotiates";
		case INSUFFICIENT_STORAGE:				return "Insufficient Storage";
		case LOOP_DETECTED:						return "Loop Detected";
		case NOT_EXTENDED:						return "Not Extended";
		case NETWORK_AUTHENTICATION_REQUIRED:	return "Network Authentication Required";

		default:								return "Unknown";
	}
}
