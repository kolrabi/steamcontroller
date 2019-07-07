#include <steam_controller/steam_controller.hpp>
#include <chrono>
#include <cstring>
#include <hidapi/hidapi.h>
#include <stdexcept>
#include <thread>

// Message codes
#define USB_VID_VALVE 0x28de
#define USB_PID_STEAMCONTROLLER_WIRED 0x1102
#define USB_PID_STEAMCONTROLLER_WIRELESS 0x1142

#define STEAMCONTROLLER_CLEAR_MAPPINGS 0x81            // 1000 0001
#define STEAMCONTROLLER_GET_ATTRIBUTES 0x83            // 1000 0011
#define STEAMCONTROLLER_UNKNOWN_85 0x85                // 1000 0101
#define STEAMCONTROLLER_SET_SETTINGS 0x87              // 1000 0111 // seems to reset config
#define STEAMCONTROLLER_UNKNOWN_8E 0x8E                // 1000 1110
#define STEAMCONTROLLER_TRIGGER_HAPTIC_PULSE 0x8F      // 1000 1111
#define STEAMCONTROLLER_START_BOOTLOADER 0x90          // 1001 0000
#define STEAMCONTROLLER_SET_PRNG_ENTROPY 0x96          // 1001 0110
#define STEAMCONTROLLER_TURN_OFF_CONTROLLER 0x9F       // 1001 1111
#define STEAMCONTROLLER_DONGLE_GET_VERSION 0xA1        // 1010 0001
#define STEAMCONTROLLER_ENABLE_PAIRING 0xAD            // 1010 1101
#define STEAMCONTROLLER_GET_STRING_ATTRIBUTE 0xAE      // 1010 1110
#define STEAMCONTROLLER_UNKNOWN_B1 0xB1                // 1011 0001
#define STEAMCONTROLLER_DISCONNECT_DEVICE 0xB2         // 1011 0010
#define STEAMCONTROLLER_COMMIT_DEVICE 0xB3             // 1011 0011
#define STEAMCONTROLLER_DONGLE_GET_WIRELESS_STATE 0xB4 // 1011 0100
#define STEAMCONTROLLER_PLAY_MELODY 0xB6               // 1011 0110
#define STEAMCONTROLLER_GET_CHIPID 0xBA                // 1011 1010
#define STEAMCONTROLLER_WRITE_EEPROM 0xC1              // 1100 0001

namespace
{

struct report_type
{
    uint8_t reportPage;
    uint8_t featureId;
    uint8_t dataLen;
    uint8_t data[62];
};

inline std::uint16_t operator"" _u(unsigned long long value)
{
    return static_cast<std::uint16_t>(value);
}

bool set_report(hid_device* device, report_type* report)
{
    return hid_send_feature_report(device, reinterpret_cast<unsigned char*>(report), sizeof(report_type)) != -1;
}

bool load_report_for(hid_device* device, report_type* report)
{
    auto const id = report->featureId;
    set_report(device, report);

    for (int i = 0; i < 50; ++i)
    {
        auto done = hid_get_feature_report(device, reinterpret_cast<unsigned char*>(report), sizeof(report_type)) != -1;
        if (done && report->featureId == id)
            return true;

        std::this_thread::sleep_for(std::chrono::nanoseconds(500));
    }

    return false;
}

/** Set up the controller to be usable. */
bool execute_initialization_sequence(hid_device* pDevice, bool wireless)
{
    report_type featureReport;

    if (wireless)
    {
#if 0
        std::memset(&featureReport, 0, sizeof(featureReport));
        featureReport.featureId = 0;

        load_report_for(pDevice, &featureReport);
#endif

        // Not really sure why the controller needs entropy. Maybe the it is used
        // to avoid collision of RF transmissions.
        std::memset(&featureReport, 0, sizeof(featureReport));
        featureReport.featureId = STEAMCONTROLLER_SET_PRNG_ENTROPY;
        featureReport.dataLen = 16;
        for (uint8_t i = 0; i < 16; i++)
            featureReport.data[i] = (uint8_t)rand(); // FIXME

        if (!set_report(pDevice, &featureReport))
        {
            return false;
        }

#if 0
        // TODO: Find out what this does.
        std::memset(&featureReport, 0, sizeof(featureReport));
        featureReport.featureId = STEAMCONTROLLER_UNKNOWN_B1;
        featureReport.dataLen = 2;
        set_report(pDevice, &featureReport);
#endif
        // TODO: Is this necessary? The results are not used.
        std::memset(&featureReport, 0, sizeof(featureReport));
        featureReport.featureId = STEAMCONTROLLER_DONGLE_GET_WIRELESS_STATE;
        set_report(pDevice, &featureReport);
    }

#if 0
    // TODO: Is this necessary? What could these attributes mean? The only
    // value ever observed for this is 0.
    std::memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId = STEAMCONTROLLER_GET_ATTRIBUTES;

    if (!load_report_for(pDevice, &featureReport))
    {
        // fprintf(stderr, "Failed to get GET_ATTRIBUTES response for controller %p\n", pDevice);
        return false;
    }

    if (featureReport.dataLen < 4)
    {
        // fprintf(stderr, "Bad GET_ATTRIBUTES response for controller %p\n", pDevice);
        // Don't fail, the controller still works without.
        // return false;
    }

    /*
      example data:

      0000   83 23 00 00 00 00 00 01 02 11 00 00 02 03 00 00
      0010   00 0a 6d 92 d2 55 04 01 8c c7 56 05 6c 4a 42 56
      0020   09 0a 00 00 00 00 00 00 00 00 00 00 00 00 00 00
      0030   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

      0x0011 or 0x0021?: board revision (=10)
      0x0012: uint32 = Bootloader revision (unix timestamp)
      0x0017: uint32 = Controller firmware revision (unix timestamp)
      0x001b: uint32 = Radio firmware revision (unix timestamp)
    */
#endif

    std::memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId = STEAMCONTROLLER_CLEAR_MAPPINGS;

    if (!set_report(pDevice, &featureReport))
    {
        // fprintf(stderr, "CLEAR_MAPPINGS failed for controller %p\n", pDevice);
        return false;
    }

#if 0
    // TODO: Find out more about the chip id.
    std::memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId = STEAMCONTROLLER_GET_CHIPID;

    if (!load_report_for(pDevice, &featureReport))
    {
        // fprintf(stderr, "GET_CHIPID failed for controller %p\n", pDevice);
        return false;
    }

    // TODO: Neccessary? Maybe remove like the other boot loaded stuff.
    std::memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId = STEAMCONTROLLER_DONGLE_GET_VERSION;

    if (!load_report_for(pDevice, &featureReport))
    {
        // fprintf(stderr, "DONGLE_GET_VERSION failed for controller %p\n", pDevice);
        return false;
    }
#endif

    return true;
}

static inline void store_word(std::uint8_t* data, uint16_t value)
{
    data[0] = static_cast<std::uint8_t>((value >> 0) & 0xff);
    data[1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
}

static inline void store_dword(uint8_t* data, uint32_t value)
{
    data[0] = static_cast<uint8_t>((value >> 0) & 0xff);
    data[1] = static_cast<uint8_t>((value >> 8) & 0xff);
    data[2] = static_cast<uint8_t>((value >> 16) & 0xff);
    data[3] = static_cast<uint8_t>((value >> 24) & 0xff);
}

/** Add a settings parameter and value to an SET_SETTINGS feature report. */
static inline void add_setting(report_type* report, std::uint8_t setting, std::uint16_t value)
{
    uint8_t offset = report->dataLen;
    report->data[offset] = setting;
    store_word(report->data + offset + 1, value);

    report->dataLen += 3;
}

bool execute_configuration_sequence(hid_device* device, unsigned configFlags)
{
    report_type report;

    // observed sequence when changing from desktop to steam:
    // 87 15 325802 180000 310200 080700 070700 300000 2e0000
    // 0000000000000000000000000000000000000000000000000000000000000000000000000000000000

    memset(&report, 0, sizeof(report));
    report.featureId = STEAMCONTROLLER_SET_SETTINGS;

    add_setting(&report, 0x32, 600); // 0x012c seconds to controller shutdown
    add_setting(&report, 0x05,
                (configFlags & static_cast<std::uint32_t>(steam_controller::Config::RIGHT_PAD_HAPTIC_TOUCH)) ? 1_u
                                                                                                             : 0_u);
    add_setting(&report, 0x07,
                (configFlags & static_cast<std::uint32_t>(steam_controller::Config::STICK_HAPTIC)) ? 0_u : 7_u);
    add_setting(&report, 0x08,
                (configFlags & static_cast<std::uint32_t>(steam_controller::Config::RIGHT_PAD_HAPTIC_TRACKBALL)) ? 0_u
                                                                                                                 : 7_u);
    add_setting(&report, 0x2d, 100);                                     // home button brightness, default to 100
    add_setting(&report, 0x30, static_cast<uint16_t>(configFlags & 31)); // Gyro settings
    // add_setting(&report, 0x31, (configFlags & STEAMCONTROLLER_CONFIG_SEND_BATTERY_STATUS) ? 2_u : 0_u);

    return set_report(device, &report);
}

void play_melody(hid_device* device, uint32_t melodyId)
{
    report_type featureReport;

    // 00 = Warm and Happy
    // 01 = Invader
    // 02 = Controller Confirmed
    // 03 = Victory
    // 04 = Rise and Shine
    // 05 = Shorty
    // 06 = Warm Boot
    // 07 = Next Level
    // 08 = Shake it off
    // 09 = Access Denied
    // 0a = Deactivate
    // 0b = Discovery
    // 0c = Triumph
    // 0d = The Mann

    memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId = STEAMCONTROLLER_PLAY_MELODY;
    featureReport.dataLen = 0x04;
    store_dword(featureReport.data, melodyId);

    // hid_write(device, reinterpret_cast<std::uint8_t const*>(&featureReport), sizeof(featureReport));
    set_report(device, &featureReport);
}

void add_feedback(
    hid_device* device, std::uint8_t side, std::uint16_t amplitude, std::uint16_t period, std::uint16_t count)
{
    report_type featureReport;
    memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId = STEAMCONTROLLER_TRIGGER_HAPTIC_PULSE;
    featureReport.dataLen = 7;
    featureReport.data[0] = side;
    store_word(featureReport.data + 1, amplitude);
    store_word(featureReport.data + 3, period);
    store_word(featureReport.data + 5, count);
    // hid_write(device, reinterpret_cast<std::uint8_t const*>(&featureReport), sizeof(featureReport));
    set_report(device, &featureReport);
}

uint8_t parse_event(steam_controller::event* pEvent, const uint8_t* eventData, int len)
{

    /*
      Layout of each message seems to be:

      0x0000 01       1 byte    Seems to always be 0x01. Could be an id if multiple controllers are paired with the same
      dongle. TODO: get second controller 0x0001 00       1 byte    Seems to always be 0x00. Could also be the high
      order byte of the other byte. 0x0002 xx       1 byte    Event type. So far the following values were observed:

                          0x01  Update. Contains axis and button data.
                          0x03  Connection update. Sent when a controller connects or disconnects (turned off/out of
      range). 0x04  Battery update. Sent occasionally to update battery status of controller. Needs to be enabled with
      STEAMCONTROLLER_CONFIG_FLAG_SEND_BATTERY_STATUS 0x05  ??? Seems mostly random. Needs some tinkering with settings,
      controller disconnects after a while.

      0x0003 xx       1 byte    Unknown. Seems to always be 0x3c (the device type) for event type 1,
                                0x01 for event type 3 and 0x0b for event type 4.

      0x0004                    Event type specific data.
    */

    using event_key = steam_controller::event_key;

    uint8_t eventType = eventData[0x02];
    pEvent->key = static_cast<steam_controller::event_key>(eventType);
    switch (eventType)
    {
    case static_cast<std::uint8_t>(event_key::UPDATE):
        /*
          0x0004 xx xx xx xx          1 ulong   Appears to be some kind of timestamp or message counter.
          0x0008 xx xx xx             3 bytes   Button data.
          0x000b xx                   1 byte    Left trigger value.
          0x000c xx                   1 byte    Right trigger value.
          0x000d 00 00 00             3 bytes   Padding?
          0x0010 xx xx yy yy          2 sshorts Left position data. If STEAMCONTROLLER_BUTTON_LFINGER is set
                                                this is the position of the thumb on the left touch pad.
                                                Otherwise it is the stick position.
          0x0014 xx xx yy yy          2 sshorts Right position data. This is the position of the thumb on the
                                                right touch pad.
          0x0018 00 00 00 00          4 bytes   Padding?
          0x001c xx xx yy yy zz zz    3 sshorts Acceleration along X,Y,Z axes.
          0x0022 xx xx yy yy zz zz    3 sshorts Angular velocity (gyro) along X,Y,Z axes.
          0x0028 xx xx yy yy zz zz    3 sshorts Orientation vector.
        */
        pEvent->update.time_stamp =
            eventData[0x04] | (eventData[0x05] << 8) | (eventData[0x06] << 16) | (eventData[0x07] << 24);
        pEvent->update.buttons = eventData[0x08] | (eventData[0x09] << 8) | (eventData[0x0a] << 16);

        pEvent->update.left_trigger = eventData[0x0b];
        pEvent->update.right_trigger = eventData[0x0c];

        pEvent->update.left_axis.x = eventData[0x10] | (eventData[0x11] << 8);
        pEvent->update.left_axis.y = eventData[0x12] | (eventData[0x13] << 8);

        pEvent->update.right_axis.x = eventData[0x14] | (eventData[0x15] << 8);
        pEvent->update.right_axis.y = eventData[0x16] | (eventData[0x17] << 8);

        pEvent->update.acceleration.x = eventData[0x1c] | (eventData[0x1d] << 8);
        pEvent->update.acceleration.y = eventData[0x1e] | (eventData[0x1f] << 8);
        pEvent->update.acceleration.z = eventData[0x20] | (eventData[0x21] << 8);

        pEvent->update.angular_velocity.x = eventData[0x22] | (eventData[0x23] << 8);
        pEvent->update.angular_velocity.y = eventData[0x24] | (eventData[0x25] << 8);
        pEvent->update.angular_velocity.z = eventData[0x26] | (eventData[0x27] << 8);

        pEvent->update.orientation.x = eventData[0x28] | (eventData[0x29] << 8);
        pEvent->update.orientation.y = eventData[0x2a] | (eventData[0x2b] << 8);
        pEvent->update.orientation.z = eventData[0x2c] | (eventData[0x2d] << 8);
        break;

    case static_cast<std::uint8_t>(event_key::BATTERY):
        /*
          0x0004 xx xx xx xx          1 ulong   Another timestamp or message counter, separate from update event.
          0x0008 00 00 00 00          4 bytes   Padding?
          0x000c xx xx                1 ushort  Battery voltage in millivolts (both cells).
          0x000e 64 00                1 ushort  Unknown. Seems to be stuck at 0x0064 (100 in decimal).
        */
        pEvent->battery.voltage = eventData[0x0c] | (eventData[0x0d] << 8);
        break;

    case static_cast<std::uint8_t>(event_key::CONNECTION):
        /*
          0x0004 xx                   1 byte    Connection detail. 0x01 for disconnect, 0x02 for connect, 0x03 for
          pairing request. 0x0005 xx xx xx             3 bytes   On disconnect: upper 3 bytes of timestamp of last
          received update.
        */
        pEvent->connection.message = eventData[4];
        break;

    default:
        eventType = 0;
        break;
    }
    return eventType;
}

uint8_t read_event(hid_device* device, steam_controller::event* pEvent)
{
    uint8_t buffer[65];
    auto bytes_read = hid_read_timeout(device, buffer, 65, 0);

    if (bytes_read <= 0)
    {
        return 0;
    }

    uint8_t* eventData = buffer;
    if (!*eventData)
    {
        eventData++;
        bytes_read--;
    }

    return parse_event(pEvent, eventData, bytes_read);
}

class hidapi_controller : public steam_controller::controller
{
public:
    using clock = std::chrono::high_resolution_clock;

    hidapi_controller(std::shared_ptr<steam_controller::api> api,
                      hid_device* device,
                      bool wireless,
                      std::uint32_t flags,
        clock::duration timeout_length)
    : m_api(std::move(api))
    , m_device(device)
    , m_state(steam_controller::connection_state::connecting)
    , m_flags(flags)
    {
        if (!execute_initialization_sequence(device, wireless))
        {
            m_state = steam_controller::connection_state::disconnected;
            return;
        }

        // Start the timeout for the connection process
        m_timeout = clock::now() + timeout_length;
    }

    bool poll(steam_controller::event& e) override
    {
        auto event_type = read_event(m_device, &e);

        if (m_state == steam_controller::connection_state::connected && e.key == steam_controller::event_key::UPDATE)
        {
            return true;
        }

        if (m_state == steam_controller::connection_state::connecting)
        {
            if (e.key == steam_controller::event_key::CONNECTION)
            {
                if (e.connection.message == steam_controller::connection_event::CONNECTED)
                {
                    execute_configuration_sequence(m_device, m_flags);
                    m_state = steam_controller::connection_state::connected;
                }
                else if (e.connection.message == steam_controller::connection_event::DISCONNECTED)
                {
                    m_state = steam_controller::connection_state::disconnected;
                }
            }
            // Time out during connection?
            else if (clock::now() > m_timeout)
            {
                m_state = steam_controller::connection_state::disconnected;
            }
        }

        return false;
    }

    void play_melody(std::uint32_t melody) override
    {
        ::play_melody(m_device, melody);
    }

    void feedback(steam_controller::hand left_or_right,
                  std::uint16_t amplitude,
                  std::chrono::microseconds period,
                  std::uint16_t count) override
    {
        std::uint8_t side = left_or_right == steam_controller::hand::left ? 1 : 0;
        if (period.count() > std::numeric_limits<std::uint16_t>::max())
            throw std::invalid_argument("Period too long");
        add_feedback(m_device, side, amplitude, static_cast<std::uint16_t>(period.count()), count);
    }

    steam_controller::connection_state state() const override
    {
        return m_state;
    }

    ~hidapi_controller() override
    {
        if (m_device)
            hid_close(m_device);
    }


private:
    std::shared_ptr<steam_controller::api> m_api;
    hid_device* m_device;
    steam_controller::connection_state m_state;
    std::chrono::high_resolution_clock::time_point m_timeout;
    std::uint32_t m_flags;

};

std::unique_ptr<steam_controller::controller> try_device(std::shared_ptr<steam_controller::api> const& api,
                                                         char const* path,
                                                         bool wireless,
                                                         std::uint32_t flags,
                                                         std::chrono::system_clock::duration timeout)
{
    auto device = hid_open_path(path);
    if (device == nullptr)
        return nullptr;

    return std::make_unique<hidapi_controller>(api, device, wireless, flags, timeout);
}

void add_list(std::vector<steam_controller::connection_info>& result, unsigned short product_id, bool wireless)
{
    auto list = hid_enumerate(USB_VID_VALVE, product_id);
    try
    {
        for (auto current = list; current != nullptr; current = current->next)
        {
            result.emplace_back(current->path, wireless);
        }
    }
    catch (...)
    {
        hid_free_enumeration(list);
        throw;
    }
    hid_free_enumeration(list);
}

} // namespace

class steam_controller::api
{
public:
    api()
    {
        hid_init();
    }

    ~api()
    {
        hid_exit();
    }

    api(api const&) = delete;
    api& operator=(api const&) = delete;
};

steam_controller::context::context()
: m_api(std::make_shared<api>())
{
}

steam_controller::context::~context() = default;

std::vector<steam_controller::connection_info> steam_controller::context::enumerate() const
{
    std::vector<steam_controller::connection_info> result;
    add_list(result, USB_PID_STEAMCONTROLLER_WIRELESS, true);
    add_list(result, USB_PID_STEAMCONTROLLER_WIRED, false);
    return result;
}

std::unique_ptr<steam_controller::controller> steam_controller::context::connect(
    connection_info const& info, std::uint32_t flags, std::chrono::system_clock::duration timeout) const
{
    return try_device(m_api, info.path.c_str(), info.wireless, flags, timeout);
}
