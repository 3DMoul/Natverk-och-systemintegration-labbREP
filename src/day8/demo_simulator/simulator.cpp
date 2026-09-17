#include <deque>
#include <iomanip>
#include <iostream>
#include <string>

struct Event {
    int time_seconds;
    std::string sensor_id;
    double value;
    std::string unit;
};

int main() {
    std::deque<Event> queue;
    bool connected = true;
    int created = 0;
    int published = 0;
    int dropped = 0;
    int reconnects = 0;

    for (int second = 1; second <= 20; ++second) {
        if (second == 8) {
            connected = false;
            std::cout << "t=08 MQTT_DISCONNECTED\n";
        }
        if (second == 14) {
            connected = true;
            ++reconnects;
            std::cout << "t=14 MQTT_RECONNECTED\n";
        }

        if (second % 3 == 0) {
            queue.push_back({second, "temperature", 20.0 + second / 10.0, "C"});
            ++created;
        }
        if (second % 4 == 0) {
            queue.push_back({second, "humidity", 40.0 + second / 10.0, "%"});
            ++created;
        }

        while (!queue.empty()) {
            Event event = queue.front();
            queue.pop_front();
            if (!connected) {
                ++dropped;
                std::cout << "t=" << std::setw(2) << std::setfill('0') << second
                          << " DROP sensor=" << event.sensor_id << "\n";
                continue;
            }

            ++published;
            std::cout << "t=" << std::setw(2) << std::setfill('0') << second
                      << " PUBLISH {\"sensorId\":\"" << event.sensor_id
                      << "\",\"value\":" << std::fixed << std::setprecision(1)
                      << event.value << ",\"unit\":\"" << event.unit << "\"}\n";
        }
    }

    std::cout << "SUMMARY created=" << created
              << " published=" << published
              << " dropped=" << dropped
              << " reconnects=" << reconnects << '\n';

    return created == 11 && published == 7 && dropped == 4 && reconnects == 1 ? 0 : 1;
}
