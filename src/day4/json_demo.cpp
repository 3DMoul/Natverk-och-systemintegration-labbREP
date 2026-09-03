#include <iostream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Reading {
    std::string sensor_id;
    double value;
    std::string unit;
};

Reading parse_reading(const std::string& text) {
    const json data = json::parse(text);

    if (!data.is_object()) {
        throw std::runtime_error("root must be an object");
    }

    if (!data.contains("sensorId") || !data["sensorId"].is_string()) {
        throw std::runtime_error("sensorId must be a string");
    }

    if (!data.contains("value") || !data["value"].is_number()) {
        throw std::runtime_error("value must be a number");
    }

    if (!data.contains("unit") || !data["unit"].is_string()) {
        throw std::runtime_error("unit must be a string");
    }

    Reading reading{
        data["sensorId"].get<std::string>(),
        data["value"].get<double>(),
        data["unit"].get<std::string>()
    };

    if (reading.sensor_id.empty()) {
        throw std::runtime_error("sensorId must not be empty");
    }

    if (reading.unit != "C") {
        throw std::runtime_error("unit must be C for temperature");
    }

    if (reading.value < -50.0 || reading.value > 100.0) {
        throw std::runtime_error(
            "temperature is outside the accepted range"
        );
    }

    return reading;
}

std::string serialize_reading(const Reading& reading) {
    const json data{
        {"sensorId", reading.sensor_id},
        {"value", reading.value},
        {"unit", reading.unit}
    };

    return data.dump(2);
}

int main() {
    try {
        const std::string input =
            R"({"sensorId":"temp-01","value":21.7,"unit":"C"})";

        const Reading reading = parse_reading(input);
        std::cout << serialize_reading(reading) << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Ogiltig data: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
