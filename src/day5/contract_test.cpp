#include "reading.hpp"
#include <iostream>
#include <limits>

int main() {
    int failed = 0;
    auto expect_rejection = [&](auto operation) {
        try {
            operation();
            ++failed;
        } catch (const std::exception&) {
            // An exception is the expected outcome for invalid input.
        }
    };

    const auto text = serialize_reading({"temp-01", 21.7, "C"});
    const auto reading = parse_reading(text);
    if (reading.sensor_id != "temp-01" || reading.value != 21.7 || reading.unit != "C") {
        ++failed;
    }

    expect_rejection([] {
        serialize_reading({"", 21.7, "C"});
    });
    expect_rejection([] {
        const double invalid_value = std::numeric_limits<double>::quiet_NaN();
        serialize_reading({"a", invalid_value, "C"});
    });
    expect_rejection([] {
        parse_reading("{");
    });
    expect_rejection([] {
        parse_reading(R"({"value":2,"unit":"C"})");
    });
    expect_rejection([] {
        parse_reading(R"({"sensorId":"a","value":"2","unit":"C"})");
    });
    expect_rejection([] {
        parse_reading(R"({"sensorId":"a","value":2,"unit":"kg"})");
    });
    expect_rejection([] {
        parse_reading(R"({"sensorId":"a","value":101,"unit":"C"})");
    });

    for (double boundary_value : {-50.0, 100.0}) {
        const auto serialized = serialize_reading({"a", boundary_value, "C"});
        const auto parsed = parse_reading(serialized);
        if (parsed.value != boundary_value) {
            ++failed;
        }
    }

    std::cout << "Contract failures: " << failed << '\n';
    return failed ? 1 : 0;
}
