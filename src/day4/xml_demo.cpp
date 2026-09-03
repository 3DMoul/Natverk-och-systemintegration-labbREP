#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <pugixml.hpp>

struct Reading {
    std::string sensor_id;
    double value;
    std::string unit;
};

Reading parse_reading(const std::string& text) {
    pugi::xml_document document;
    const pugi::xml_parse_result parse_result =
        document.load_string(text.c_str());

    if (!parse_result) {
        throw std::runtime_error(
            std::string("invalid XML: ") + parse_result.description()
        );
    }

    const pugi::xml_node root = document.child("reading");
    if (!root) {
        throw std::runtime_error("root element must be reading");
    }

    const pugi::xml_node sensor_id = root.child("sensorId");
    const pugi::xml_node value = root.child("value");
    const pugi::xml_node unit = root.child("unit");

    if (!sensor_id || !value || !unit) {
        throw std::runtime_error(
            "sensorId, value and unit are required"
        );
    }

    double parsed_value = 0.0;
    std::istringstream value_stream(value.child_value());

    if (!(value_stream >> parsed_value)) {
        throw std::runtime_error("value must be a number");
    }

    value_stream >> std::ws;
    if (value_stream.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("value must be a number");
    }

    Reading reading{
        sensor_id.child_value(),
        parsed_value,
        unit.child_value()
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
    pugi::xml_document document;

    pugi::xml_node declaration =
        document.append_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";

    pugi::xml_node root = document.append_child("reading");
    root.append_child("sensorId").text().set(
        reading.sensor_id.c_str()
    );

    std::ostringstream value_text;
    value_text << reading.value;
    root.append_child("value").text().set(
        value_text.str().c_str()
    );

    root.append_child("unit").text().set(reading.unit.c_str());

    std::ostringstream output;
    document.save(output, "  ");
    return output.str();
}

int main() {
    try {
        const std::string input =
            "<reading>"
            "<sensorId>temp-01</sensorId>"
            "<value>21.7</value>"
            "<unit>C</unit>"
            "</reading>";

        const Reading reading = parse_reading(input);
        std::cout << serialize_reading(reading);
    } catch (const std::exception& error) {
        std::cerr << "Ogiltig data: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
