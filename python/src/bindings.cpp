#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "e7-switcher/e7_switcher_client.h"
#include "e7-switcher/data_structures.h"

namespace py = pybind11;
using namespace e7_switcher;

// Helper function to convert Device to Python dict
py::dict device_to_dict(const Device& device) {
    py::dict result;
    result["name"] = device.name;
    result["ssid"] = device.ssid;
    result["mac"] = device.mac;
    result["type"] = device.type;
    result["firmware"] = device.firmware;
    result["online"] = device.online;
    result["line_no"] = device.line_no;
    result["line_type"] = device.line_type;
    result["did"] = device.did;
    return result;
}

// Helper function to convert SwitchStatus to Python dict
py::dict switch_status_to_dict(const SwitchStatus& status) {
    py::dict result;
    result["wifi_power"] = status.wifi_power;
    result["switch_state"] = status.switch_state;
    result["remaining_time"] = status.remaining_time;
    result["open_time"] = status.open_time;
    result["auto_closing_time"] = status.auto_closing_time;
    result["is_delay"] = status.is_delay;
    result["online_state"] = status.online_state;
    return result;
}

// Helper function to convert ACStatus to Python dict
py::dict ac_status_to_dict(const ACStatus& status) {
    py::dict result;
    result["wifi_power"] = status.wifi_power;
    result["temperature"] = status.temperature;
    result["power_status"] = static_cast<int>(status.power_status);
    result["mode"] = static_cast<int>(status.mode);
    result["ac_temperature"] = status.ac_temperature;
    result["fan_speed"] = static_cast<int>(status.fan_speed);
    result["swing"] = static_cast<int>(status.swing);
    result["temperature_unit"] = status.temperature_unit;
    result["device_type"] = status.device_type;
    result["code_id"] = status.code_id;
    result["last_time"] = status.last_time;
    result["open_time"] = status.open_time;
    result["auto_closing_time"] = status.auto_closing_time;
    result["is_delay"] = status.is_delay;
    result["online_state"] = status.online_state;
    return result;
}

PYBIND11_MODULE(_core, m) {
    m.doc() = "E7 Switcher Python bindings";
    
    // Enums
    py::enum_<ACMode>(m, "ACMode")
        .value("AUTO", ACMode::AUTO)
        .value("DRY", ACMode::DRY)
        .value("FAN", ACMode::FAN)
        .value("COOL", ACMode::COOL)
        .value("HEAT", ACMode::HEAT)
        .export_values();
    
    py::enum_<ACFanSpeed>(m, "ACFanSpeed")
        .value("FAN_LOW", ACFanSpeed::FAN_LOW)
        .value("FAN_MEDIUM", ACFanSpeed::FAN_MEDIUM)
        .value("FAN_HIGH", ACFanSpeed::FAN_HIGH)
        .value("FAN_AUTO", ACFanSpeed::FAN_AUTO)
        .export_values();
    
    py::enum_<ACSwing>(m, "ACSwing")
        .value("SWING_OFF", ACSwing::SWING_OFF)
        .value("SWING_ON", ACSwing::SWING_ON)
        .export_values();
    
    py::enum_<ACPower>(m, "ACPower")
        .value("POWER_OFF", ACPower::POWER_OFF)
        .value("POWER_ON", ACPower::POWER_ON)
        .export_values();
    
    // E7SwitcherClient class
    py::class_<E7SwitcherClient>(m, "E7SwitcherClient")
        .def(py::init<const std::string&, const std::string&>(),
             py::arg("account"), py::arg("password"))
        .def("list_devices", [](E7SwitcherClient& self) {
            const std::vector<Device>& devices = self.list_devices();
            py::list result;
            for (const auto& device : devices) {
                result.append(device_to_dict(device));
            }
            return result;
        })
        .def("control_switch", &E7SwitcherClient::control_switch,
             py::arg("device_name"), py::arg("action"))
        .def("control_ac", &E7SwitcherClient::control_ac,
             py::arg("device_name"), py::arg("action"), py::arg("mode"),
             py::arg("temperature"), py::arg("fan_speed"), py::arg("swing"),
             py::arg("operation_time") = 0)
        .def("get_switch_status", [](E7SwitcherClient& self, const std::string& device_name) {
            SwitchStatus status = self.get_switch_status(device_name);
            return switch_status_to_dict(status);
        }, py::arg("device_name"))
        .def("get_ac_status", [](E7SwitcherClient& self, const std::string& device_name) {
            ACStatus status = self.get_ac_status(device_name);
            return ac_status_to_dict(status);
        }, py::arg("device_name"));
}
