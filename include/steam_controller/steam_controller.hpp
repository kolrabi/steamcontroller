#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace steam_controller
{

enum class Button
{
    RT = (1U << 0U),                // Right trigger fully pressed.
    LT = (1U << 1U),                // Left trigger fully pressed.
    RS = (1U << 2U),                // Right shoulder button pressed.
    LS = (1U << 3U),                // Left shoulder button pressed.
    Y = (1U << 4U),                 // Y button.
    B = (1U << 5U),                 // B button.
    X = (1U << 6U),                 // X button.
    A = (1U << 7U),                 // A button.
    DPAD_UP = (0x01U << 8U),        // Left pad pressed with thumb in the upper quarter.
    DPAD_RIGHT = (0x02U << 8U),     // Left pad pressed with thumb in the right quarter.
    DPAD_LEFT = (0x04U << 8U),      // Left pad pressed with thumb in the left quarter.
    DPAD_DOWN = (0x08U << 8U),      // Left pad pressed with thumb in the bottom quarter.
    PREV = (0x10U << 8U),           // Left arrow button.
    HOME = (0x20U << 8U),           // Steam logo button.
    NEXT = (0x40U << 8U),           // Right arrow button.
    LG = (0x80U << 8U),             // Left grip button.
    RG = (0x01U << 16U),            // Right grip button.
    STICK = (0x02U << 16U),         // Stick or left pad is pressed down.
    RPAD = (0x04U << 16U),          // Right pad pressed.
    LFINGER = (0x08U << 16U),       // If set, a finger is touching left touch pad.
    RFINGER = (0x10U << 16U),       // If set, a finger is touching right touch pad.
    FLAG_PAD_STICK = (0x80U << 16U) // If set, LFINGER determines whether left_axis is pad- or stick-position.
};

enum class Config : std::uint32_t
{
    HORIZONTAL_ACCELERATION_AS_STICK = 1, /**< Replaces left horizontal axes with X acceleration. */
    VERTICAL_ACCELERATION_AS_STICK = 2,   /**< Replaces left vertical axes with Y acceleration. */
    SEND_ORIENTATION = 4,                 /**< Sends the controller orientation. */
    SEND_ACCELERATION = 8,                /**< Sends the controller acceleration vector. */
    SEND_GYRO = 16,                       /**< Sends the angular velocity of the controller. */
    SEND_BATTERY_STATUS = 32,             /**< Sends battery status updates. */
    RIGHT_PAD_HAPTIC_TOUCH = 64,          /**< Makes right pad tick on touch. */
    RIGHT_PAD_HAPTIC_TRACKBALL = 128,     /**< Makes right pad tick like in trackball mode. */
    STICK_HAPTIC = 256,                   /**< Makes stick tick on movement. */
};

struct vec2
{
    int16_t x, y;
};

struct vec3
{
    int16_t x, y, z;
};

enum class event_key : uint32_t
{
    UPDATE = (1),
    CONNECTION = (3),
    BATTERY = (4)
};

struct update_event
{
    event_key key;

    uint32_t time_stamp;
    uint32_t buttons;

    uint8_t left_trigger;
    uint8_t right_trigger;

    vec2 left_axis;
    vec2 right_axis;

    vec3 orientation;
    vec3 acceleration;
    vec3 angular_velocity;
};

struct connection_event
{
    enum message_type
    {
        DISCONNECTED = 1,
        CONNECTED = 2,
        PAIRING_REQUESTED = 3,
    };

    event_key key;
    uint8_t message;
};

struct battery_event
{
    event_key key;
    uint16_t voltage; /**< Battery voltage in millivolts */
};

union event {
    event_key key;
    update_event update;
    battery_event battery;
    connection_event connection;
};

struct connection_info
{
    connection_info(std::string path, bool wireless)
    : path(path)
    , wireless(wireless)
    {
    }
    std::string path;
    bool wireless = true;
};

inline bool operator==(connection_info const& lhs, connection_info const& rhs)
{
    return lhs.path == rhs.path && lhs.wireless == rhs.wireless;
}

inline bool operator!=(connection_info const& lhs, connection_info const& rhs)
{
    return !(lhs == rhs);
}

enum class hand
{
    right = 0,
    left = 1,
};

enum class connection_state
{
    connecting = 0,
    connected = 1,
    disconnected = 2,
};

class controller
{
public:
    virtual ~controller() = default;

    virtual bool poll(event& event) = 0;
    virtual void play_melody(std::uint32_t melody) = 0;
    virtual void
    feedback(hand left_or_right, std::uint16_t amplitude, std::chrono::microseconds period, std::uint16_t count) = 0;

    void feedback(std::uint16_t amplitude, std::chrono::microseconds period, std::uint16_t count)
    {
        feedback(hand::left, amplitude, period, count);
        feedback(hand::right, amplitude, period, count);
    }

    virtual connection_state state() const = 0;
};

class api;

class context
{
public:
    context();
    context(context const&) = default;
    context(context&&) = default;
    ~context();

    context& operator=(context const&) = default;
    context& operator=(context&&) = default;

    std::vector<connection_info> enumerate() const;
    std::unique_ptr<controller>
    connect(connection_info const& info, std::uint32_t flags, std::chrono::system_clock::duration timeout) const;

private:
    std::shared_ptr<api> m_api;
};

} // namespace steam_controller
