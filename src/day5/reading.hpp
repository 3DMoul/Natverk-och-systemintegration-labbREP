#pragma once
#include <cmath>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>

struct Reading {
    std::string sensor_id;
    double value;
    std::string unit;
};

inline void validate(const Reading& reading) {
    if (reading.sensor_id.empty()) {
        throw std::runtime_error("sensorId must not be empty");
    }

    if (reading.unit != "C") {
        throw std::runtime_error("unit must be C");
    }

    if (!std::isfinite(reading.value) || reading.value < -50 || reading.value > 100) {
        throw std::runtime_error("value must be finite and between -50 and 100");
    }
}

inline Reading parse_reading(const std::string& text) {
    const auto data = nlohmann::json::parse(text);

    if (!data.is_object()
        || !data.contains("sensorId") || !data["sensorId"].is_string()
        || !data.contains("value") || !data["value"].is_number()
        || !data.contains("unit") || !data["unit"].is_string()) {
        throw std::runtime_error("expected sensorId:string, value:number, unit:string");
    }

    Reading reading{
        data["sensorId"].get<std::string>(),
        data["value"].get<double>(),
        data["unit"].get<std::string>()
    };
    validate(reading);

    // Unknown fields are deliberately ignored in this v1 demo.
    return reading;
}

inline std::string serialize_reading(const Reading& reading) {
    // Also validate locally produced data before transmission.
    validate(reading);

    const nlohmann::json data{
        {"sensorId", reading.sensor_id},
        {"value", reading.value},
        {"unit", reading.unit}
    };
    return data.dump();
}
