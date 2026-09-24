#include "net.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <csignal>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>

using SteadyClock = std::chrono::steady_clock;

namespace {

const char *dashboard = R"HTML(<!doctype html>
<html lang="sv"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>IoT25 – lokal dashboard (C++)</title><style>
body{font:16px system-ui,sans-serif;margin:2rem;background:#f4f7f9;color:#17222b}h1{margin-bottom:.25rem}#status{color:#52616b}
main{display:grid;grid-template-columns:repeat(auto-fit,minmax(13rem,1fr));gap:1rem}article{background:white;border-left:.4rem solid #167d8d;border-radius:.3rem;padding:1rem;box-shadow:0 .1rem .5rem #0002}article strong{display:block;font-size:1.7rem;margin-top:.4rem}code{font-size:.85rem}
</style></head><body><h1>IoT-flödets mätetal</h1><p id="status">Hämtar…</p><main id="cards"></main><script>
const shown=[["http_requests_total","HTTP-anrop","st"],["readings_accepted_total","Accepterade mätningar","st"],["validation_errors_total","Valideringsfel","st"],["not_found_total","Okända resurser","st"],["request_duration_ms_avg","Genomsnittlig svarstid","ms"],["request_duration_ms_max","Maximal svarstid","ms"],["uptime_seconds","Upptid","s"]];
async function refresh(){try{const r=await fetch('/api/metrics',{cache:'no-store'}),d=await r.json();document.getElementById('cards').innerHTML=shown.map(([k,t,u])=>`<article><code>${k}</code><strong>${d[k]} ${u}</strong><span>${t}</span></article>`).join('');document.getElementById('status').textContent=`Senast uppdaterad ${new Date().toLocaleTimeString()} · sidan hämtar samma data varannan sekund`;}catch(e){document.getElementById('status').textContent=`Kunde inte hämta mätetal: ${e}`;}}refresh();setInterval(refresh,2000);
</script></body></html>)HTML";

std::atomic<bool> running{true};

std::string json_escape(const std::string &value)
{
    std::string result;
    for (const char character : value) {
        if (character == '\\' || character == '"') result += '\\';
        if (character == '\n') result += "\\n";
        else if (character != '\r') result += character;
    }
    return result;
}

std::string utc_timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S") << '.'
           << std::setfill('0') << std::setw(3) << milliseconds.count() << 'Z';
    return output.str();
}

void log_event(const std::string &level, const std::string &event,
    const std::string &request_id, const std::string &method,
    const std::string &path, int status, double duration_ms,
    const std::string &extra = "")
{
    std::cout << "{\"timestamp\":\"" << utc_timestamp()
              << "\",\"level\":\"" << level
              << "\",\"event\":\"" << event
              << "\",\"request_id\":\"" << json_escape(request_id)
              << "\",\"method\":\"" << method
              << "\",\"path\":\"" << path
              << "\",\"status\":" << status
              << ",\"duration_ms\":" << std::fixed << std::setprecision(3) << duration_ms
              << extra << "}" << std::endl;
}

class Metrics {
public:
    void complete(double duration_ms)
    {
        std::lock_guard<std::mutex> guard(lock_);
        ++http_requests_total;
        duration_sum += duration_ms;
        duration_max = (std::max)(duration_max, duration_ms);
    }

    void accepted() { std::lock_guard<std::mutex> guard(lock_); ++readings_accepted_total; }
    void invalid() { std::lock_guard<std::mutex> guard(lock_); ++validation_errors_total; }
    void not_found() { std::lock_guard<std::mutex> guard(lock_); ++not_found_total; }

    std::string json()
    {
        std::lock_guard<std::mutex> guard(lock_);
        std::ostringstream out;
        const double average = http_requests_total ? duration_sum / http_requests_total : 0.0;
        out << std::fixed << std::setprecision(3)
            << "{\"http_requests_total\":" << http_requests_total
            << ",\"readings_accepted_total\":" << readings_accepted_total
            << ",\"validation_errors_total\":" << validation_errors_total
            << ",\"not_found_total\":" << not_found_total
            << ",\"request_duration_ms_avg\":" << average
            << ",\"request_duration_ms_max\":" << duration_max
            << ",\"uptime_seconds\":" << uptime() << '}';
        return out.str();
    }

    std::string text()
    {
        std::lock_guard<std::mutex> guard(lock_);
        std::ostringstream out;
        const double average = http_requests_total ? duration_sum / http_requests_total : 0.0;
        out << std::fixed << std::setprecision(3)
            << "iot25_http_requests_total " << http_requests_total << '\n'
            << "iot25_readings_accepted_total " << readings_accepted_total << '\n'
            << "iot25_validation_errors_total " << validation_errors_total << '\n'
            << "iot25_not_found_total " << not_found_total << '\n'
            << "iot25_request_duration_ms_avg " << average << '\n'
            << "iot25_request_duration_ms_max " << duration_max << '\n'
            << "iot25_uptime_seconds " << uptime() << '\n';
        return out.str();
    }

private:
    double uptime() const
    {
        return std::chrono::duration<double>(SteadyClock::now() - started_).count();
    }
    std::mutex lock_;
    SteadyClock::time_point started_ = SteadyClock::now();
    unsigned long long http_requests_total = 0;
    unsigned long long readings_accepted_total = 0;
    unsigned long long validation_errors_total = 0;
    unsigned long long not_found_total = 0;
    double duration_sum = 0.0;
    double duration_max = 0.0;
};

Metrics metrics;
std::atomic<unsigned long long> generated_ids{0};

struct Request {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool receive_request(socket_t client, Request &request)
{
    std::string raw;
    char buffer[4096];
    while (raw.find("\r\n\r\n") == std::string::npos) {
        const auto count = recv(client, buffer, sizeof(buffer), 0);
        if (count <= 0) return false;
        raw.append(buffer, static_cast<std::size_t>(count));
        if (raw.size() > 32768) return false;
    }
    const auto header_end = raw.find("\r\n\r\n");
    std::istringstream headers(raw.substr(0, header_end));
    std::string line;
    if (!std::getline(headers, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream first(line);
    first >> request.method >> request.path;
    if (const auto query = request.path.find('?'); query != std::string::npos) request.path.resize(query);
    while (std::getline(headers, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto colon = line.find(':');
        if (colon != std::string::npos) {
            auto value = line.substr(colon + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            request.headers[lower(line.substr(0, colon))] = value;
        }
    }
    std::size_t length = 0;
    if (request.headers.count("content-length")) length = std::stoul(request.headers["content-length"]);
    request.body = raw.substr(header_end + 4);
    while (request.body.size() < length) {
        const auto count = recv(client, buffer, sizeof(buffer), 0);
        if (count <= 0) return false;
        request.body.append(buffer, static_cast<std::size_t>(count));
    }
    if (request.body.size() > length) request.body.resize(length);
    return true;
}

void respond(socket_t client, int status, const std::string &reason,
    const std::string &content_type, const std::string &body, const std::string &request_id)
{
    std::ostringstream response;
    response << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
             << "Content-Type: " << content_type << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Cache-Control: no-store\r\n"
             << "X-Request-ID: " << request_id << "\r\n"
             << "Connection: close\r\n\r\n" << body;
    send_all(client, response.str());
}

bool extract_string(const std::string &json, const std::string &key, std::string &value)
{
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) return false;
    value = match[1].str();
    return !value.empty();
}

bool extract_number(const std::string &json, const std::string &key, double &value)
{
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) return false;
    value = std::stod(match[1].str());
    return std::isfinite(value);
}

void handle(socket_t client)
{
    const auto started = SteadyClock::now();
    Request request;
    if (!receive_request(client, request)) return;
    const auto header_id = request.headers.find("x-request-id");
    const std::string request_id = header_id != request.headers.end()
        ? header_id->second : "server-" + std::to_string(++generated_ids);
    int status = 200;
    std::string event = "health_checked";
    std::string level = "INFO";
    std::string extra;

    if (request.method == "GET" && request.path == "/health") {
        respond(client, 200, "OK", "application/json; charset=utf-8", "{\"status\":\"ok\"}", request_id);
    } else if (request.method == "GET" && request.path == "/api/metrics") {
        event = "metrics_read";
        respond(client, 200, "OK", "application/json; charset=utf-8", metrics.json(), request_id);
    } else if (request.method == "GET" && request.path == "/metrics") {
        event = "metrics_read";
        respond(client, 200, "OK", "text/plain; charset=utf-8", metrics.text(), request_id);
    } else if (request.method == "GET" && (request.path == "/" || request.path == "/dashboard")) {
        event = "dashboard_read";
        respond(client, 200, "OK", "text/html; charset=utf-8", dashboard, request_id);
    } else if (request.method == "POST" && request.path == "/api/readings") {
        std::string sensor_id;
        std::string unit;
        double value = 0.0;
        std::string error;
        if (!extract_string(request.body, "sensor_id", sensor_id)) error = "sensor_id must be a non-empty string";
        else if (!extract_number(request.body, "value", value)) error = "value must be a finite number";
        else if (!extract_string(request.body, "unit", unit)) error = "unit must be a non-empty string";
        if (!error.empty()) {
            status = 400; event = "reading_rejected"; level = "WARNING";
            metrics.invalid();
            extra = ",\"reason\":\"" + json_escape(error) + "\"";
            respond(client, 400, "Bad Request", "application/json; charset=utf-8",
                "{\"error\":\"" + json_escape(error) + "\"}", request_id);
        } else {
            status = 202; event = "reading_accepted";
            metrics.accepted();
            extra = ",\"sensor_id\":\"" + json_escape(sensor_id) + "\",\"unit\":\"" + json_escape(unit) + "\"";
            respond(client, 202, "Accepted", "application/json; charset=utf-8",
                "{\"status\":\"accepted\",\"request_id\":\"" + json_escape(request_id) + "\"}", request_id);
        }
    } else {
        status = 404; event = "route_not_found"; level = "WARNING";
        metrics.not_found();
        respond(client, 404, "Not Found", "application/json; charset=utf-8", "{\"error\":\"not found\"}", request_id);
    }
    const double duration = std::chrono::duration<double, std::milli>(SteadyClock::now() - started).count();
    metrics.complete(duration);
    log_event(level, event, request_id, request.method, request.path, status, duration, extra);
}

void stop_server(int) { running = false; }

} // namespace

int main(int argc, char **argv)
{
    int port = 8090;
    for (int index = 1; index + 1 < argc; ++index) {
        if (std::string(argv[index]) == "--port") port = std::stoi(argv[++index]);
    }
    try {
        [[maybe_unused]] SocketRuntime runtime;
        const socket_t server = socket(AF_INET, SOCK_STREAM, 0);
        if (server == invalid_socket) throw std::runtime_error("could not create server socket");
        int reuse = 1;
        setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(static_cast<unsigned short>(port));
        if (bind(server, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 || listen(server, 16) != 0) {
            close_socket(server);
            throw std::runtime_error("could not bind/listen on 127.0.0.1:" + std::to_string(port));
        }
        std::signal(SIGINT, stop_server);
        std::cout << "{\"timestamp\":\"" << utc_timestamp()
                  << "\",\"level\":\"INFO\",\"event\":\"server_started\",\"host\":\"127.0.0.1\",\"port\":"
                  << port << "}" << std::endl;
        while (running) {
            // En kort select-timeout gör att Ctrl+C kan avsluta servern även
            // när ingen klient för tillfället ansluter.
            fd_set readable;
            FD_ZERO(&readable);
            FD_SET(server, &readable);
            timeval timeout{0, 250000};
            const int ready = select(static_cast<int>(server + 1), &readable, nullptr, nullptr, &timeout);
            if (!running) break;
            if (ready <= 0) continue;
            sockaddr_in peer{};
#ifdef _WIN32
            int peer_size = sizeof(peer);
#else
            socklen_t peer_size = sizeof(peer);
#endif
            const socket_t client = accept(server, reinterpret_cast<sockaddr *>(&peer), &peer_size);
            if (client == invalid_socket) continue;
            try { handle(client); }
            catch (const std::exception &error) { std::cerr << "request error: " << error.what() << '\n'; }
            close_socket(client);
        }
        close_socket(server);
    } catch (const std::exception &error) {
        std::cerr << "SERVER ERROR: " << error.what() << '\n';
        return 1;
    }
}
