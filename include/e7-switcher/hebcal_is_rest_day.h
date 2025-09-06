#pragma once
#include <optional>

namespace e7_switcher {

/**
 * Checks if the current day is a rest day (Shabbat or holiday) in Jerusalem
 * 
 * @return std::optional<bool> true if it's a rest day, false if not, nullopt if the check failed
 */
std::optional<bool> hebcal_is_rest_day_in_jerusalem();

} // namespace e7_switcher
