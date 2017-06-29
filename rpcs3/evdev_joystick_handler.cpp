#ifdef HAVE_LIBEVDEV

#include "evdev_joystick_handler.h"
#include "Utilities/Thread.h"
#include "Utilities/Log.h"

#include <functional>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>

evdev_joystick_config g_evdev_joystick_config;

namespace
{
    const u32 THREAD_SLEEP = 10;
    const u32 THREAD_SLEEP_INACTIVE = 100;
    const u32 READ_TIMEOUT = 10;
    const u32 THREAD_TIMEOUT = 1000;

    const std::string EVENT_JOYSTICK = "event-joystick";

    inline u32 milli2micro(u32 milli) { return milli * 1000; }
}

enum { DPAD_AXIS_X = -1, DPAD_AXIS_Y = -2 };

evdev_joystick_handler::evdev_joystick_handler() {}

evdev_joystick_handler::~evdev_joystick_handler() { Close(); }

void evdev_joystick_handler::Init(const u32 max_connect)
{
    std::memset(&m_info, 0, sizeof m_info);
    m_info.max_connect = std::min(max_connect, static_cast<u32>(1));

    g_evdev_joystick_config.load();

    fs::dir devdir{"/dev/input/by-id"};
    fs::dir_entry et;

    while (devdir.read(et)) {
        // Does the entry name end with event-joystick?
        if (et.name.size() > EVENT_JOYSTICK.size() &&
            et.name.compare(et.name.size() - EVENT_JOYSTICK.size(),
                            EVENT_JOYSTICK.size(), EVENT_JOYSTICK) == 0)
        {
            joy_paths.emplace_back(fmt::format("/dev/input/by-id/%s", et.name));
        }
    }

    for (u32 i = 0; i < m_info.max_connect; ++i)
    {
        joy_devs.push_back(nullptr);
        joy_axis_maps.emplace_back(ABS_RZ - ABS_X, -1);
        joy_button_maps.emplace_back(KEY_MAX - BTN_JOYSTICK, -1);
        joy_hat_ids.emplace_back(-1);
        m_pads.emplace_back(
            CELL_PAD_STATUS_DISCONNECTED,
            CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
            CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
            CELL_PAD_DEV_TYPE_STANDARD
        );
        auto& pad = m_pads.back();

        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_evdev_joystick_config.triangle, CELL_PAD_CTRL_TRIANGLE);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_evdev_joystick_config.circle, CELL_PAD_CTRL_CIRCLE);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_evdev_joystick_config.cross, CELL_PAD_CTRL_CROSS);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_evdev_joystick_config.square, CELL_PAD_CTRL_SQUARE);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_evdev_joystick_config.l2, CELL_PAD_CTRL_L2);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_evdev_joystick_config.r2, CELL_PAD_CTRL_R2);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_evdev_joystick_config.l1, CELL_PAD_CTRL_L1);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_evdev_joystick_config.r1, CELL_PAD_CTRL_R1);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_evdev_joystick_config.start, CELL_PAD_CTRL_START);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_evdev_joystick_config.select, CELL_PAD_CTRL_SELECT);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_evdev_joystick_config.l3, CELL_PAD_CTRL_L3);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_evdev_joystick_config.r3, CELL_PAD_CTRL_R3);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support

        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, DPAD_AXIS_Y, CELL_PAD_CTRL_UP);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, DPAD_AXIS_Y, CELL_PAD_CTRL_DOWN);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, DPAD_AXIS_X, CELL_PAD_CTRL_LEFT);
        pad.m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, DPAD_AXIS_X, CELL_PAD_CTRL_RIGHT);

        pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X+g_evdev_joystick_config.lxstick, 0, 0);
        pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X+g_evdev_joystick_config.lystick, 0, 0);
        pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X+g_evdev_joystick_config.rxstick, 0, 0);
        pad.m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X+g_evdev_joystick_config.rystick, 0, 0);
    }

    update_devs();
    active.store(true);
    joy_thread.reset(new std::thread(std::bind(&evdev_joystick_handler::thread_func, this)));
}

void evdev_joystick_handler::update_devs()
{
    int connected=0;

    for (u32 i = 0; i < m_info.max_connect; ++i)
        if (try_open_dev(i)) ++connected;

    m_info.now_connect = connected;
}

bool evdev_joystick_handler::try_open_dev(u32 index)
{
    libevdev*& dev = joy_devs[index];
    bool was_connected = dev != nullptr;

    if (index >= joy_paths.size()) return false;
    const auto& path = joy_paths[index];

    if (access(path.c_str(), R_OK) == -1)
    {
        if (was_connected)
        {
            // It was disconnected.
            m_pads[index].m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;

            int fd = libevdev_get_fd(dev);
            libevdev_free(dev);
            close(fd);
            dev = nullptr;
        }
        m_pads[index].m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
        LOG_ERROR(GENERAL, "Joystick %s is not present or accessible [previous status: %d]", path.c_str(),
                  was_connected ? 1 : 0);
        return false;
    }

    if (was_connected) return true;  // It's already been connected, and the js is still present.
    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);

    if (fd == -1)
    {
        int err = errno;
        LOG_ERROR(GENERAL, "Failed to open joystick #%d: %s [errno %d]", index, strerror(err), err);
        return false;
    }

    int ret = libevdev_new_from_fd(fd, &dev);
    if (ret < 0)
    {
        LOG_ERROR(GENERAL, "Failed to initialize libevdev for joystick #%d: %s [errno %d]", index, strerror(-ret), -ret);
        return false;
    }

    LOG_NOTICE(GENERAL, "Opened joystick #%d '%s' at %s (fd %d)", index, libevdev_get_name(dev), path, fd);

    if (!was_connected)
        // Connection status changed from disconnected to connected.
        m_pads[index].m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
    m_pads[index].m_port_status |= CELL_PAD_STATUS_CONNECTED;

    int buttons=0;
    for (int i=BTN_JOYSTICK; i<KEY_MAX; i++)
        if (libevdev_has_event_code(dev, EV_KEY, i))
        {
            LOG_NOTICE(GENERAL, "Joystick #%d has button %d as %d", index, i, buttons);
            joy_button_maps[index][i - BTN_MISC] = buttons++;
        }

    int axes=0;
    for (int i=ABS_X; i<=ABS_RZ; i++)
        if (libevdev_has_event_code(dev, EV_ABS, i))
        {
            LOG_NOTICE(GENERAL, "Joystick #%d has axis %d as %d", index, i, axes);
            joy_axis_maps[index][i - ABS_X] = axes++;
        }

    for (int i=ABS_HAT0X; i<=ABS_HAT3Y; i+=2)
        if (libevdev_has_event_code(dev, EV_ABS, i) ||
            libevdev_has_event_code(dev, EV_ABS, i+1))
        {
            LOG_NOTICE(GENERAL, "Joystick #%d has hat %d", index, i);
            joy_hat_ids[index] = i - ABS_HAT0X;
        }

    return true;
}

void evdev_joystick_handler::Close()
{
    if (active.load())
    {
        active.store(false);
        if (!dead.load())
        {
            usleep(milli2micro(THREAD_TIMEOUT));
            if (!dead.load())
                LOG_ERROR(GENERAL, "EvdevJoystick thread could not stop within %d milliseconds", THREAD_TIMEOUT);
        }
    }

    joy_thread->detach();

    for (auto& dev : joy_devs)
    {
        if (dev != nullptr)
        {
            int fd = libevdev_get_fd(dev);
            libevdev_free(dev);
            close(fd);
        }
    }
}

void evdev_joystick_handler::thread_func()
{
    while (active)
    {
        update_devs();

        for (int i=0; i<joy_devs.size(); i++)
        {
            auto& pad = m_pads[i];
            auto& dev = joy_devs[i];
            if (dev == nullptr) continue;

            // Try to query the latest event from the joystick.
            input_event evt;
            int ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &evt);

            // Grab any pending sync event.
            if (ret == LIBEVDEV_READ_STATUS_SYNC)
            {
                LOG_NOTICE(GENERAL, "Captured sync event");
                ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_SYNC, &evt);
            }

            if (ret < 0)
            {
                // -EAGAIN signifies no available events, not an actual *error*.
                if (ret != -EAGAIN)
                    LOG_ERROR(GENERAL, "Failed to read latest event from joystick #%d: %s [errno %d]", i, strerror(-ret), -ret);
                continue;
            }

            switch (evt.type)
            {
            case EV_SYN:
                LOG_NOTICE(GENERAL, "Joystick #%d sent EV_SYN", i);
                break;
            case EV_KEY:
            {
                LOG_NOTICE(GENERAL, "Joystick #%d EV_KEY: %d %d", i, evt.code, evt.value);
                if (evt.code < BTN_MISC)
                {
                    LOG_NOTICE(GENERAL, "Joystick #%d sent non-button key event %d", i, evt.code);
                    break;
                }

                int button_code = joy_button_maps[i][evt.code - BTN_MISC];
                if (button_code == -1)
                {
                    LOG_ERROR(GENERAL, "Joystick #%d sent invalid button code %d", i, evt.code);
                }

                auto which_button = std::find_if(
                    pad.m_buttons.begin(), pad.m_buttons.end(),
                    [&](const Button& bt) { return bt.m_keyCode == button_code; });
                if (which_button == pad.m_buttons.end())
                {
                    LOG_ERROR(GENERAL, "Joystick #%d sent button event for invalid button %d", i, evt.code);
                    break;
                }

                which_button->m_pressed = evt.value;
                which_button->m_value = evt.value ? 255 : 0;
                break;
            }
            case EV_ABS:
                LOG_NOTICE(GENERAL, "Joystick #%d EV_ABS: %d %d", i, evt.code, evt.value);

                if (evt.code >= ABS_HAT0X && evt.code <= ABS_HAT3Y) {
                    int hat = evt.code - ABS_HAT0X;
                    if (hat != joy_hat_ids[i] && hat-1 != joy_hat_ids[i])
                    {
                        LOG_ERROR(GENERAL, "Joystick #%d sent HAT event for invalid hat %d (expected %d)", i, hat, joy_hat_ids[i]);
                        break;
                    }

                    int source_axis = hat == joy_hat_ids[i] ? DPAD_AXIS_X : DPAD_AXIS_Y;

                    for (Button& bt : pad.m_buttons)
                    {
                        if (bt.m_keyCode != source_axis) continue;

                        if (evt.value == 0)
                        {
                            bt.m_pressed = false;
                            bt.m_value = 0;
                        }
                        else
                        {
                            int code = -1;
                            if (source_axis == DPAD_AXIS_X)
                            {
                                code = evt.value > 0 ? CELL_PAD_CTRL_RIGHT : CELL_PAD_CTRL_LEFT;
                            }
                            else
                            {
                                code = evt.value > 0 ? CELL_PAD_CTRL_DOWN : CELL_PAD_CTRL_UP;
                            }

                            if (bt.m_outKeyCode == code)
                            {
                                bt.m_pressed = true;
                                bt.m_value = 255;
                            }
                        }
                    }
                }
                else if (evt.code <= ABS_RZ)
                {
                    int axis = joy_axis_maps[i][evt.code - ABS_X];

                    if (axis > pad.m_sticks.size())
                    {
                        LOG_ERROR(GENERAL, "Joystick #%d sent axis event for invalid axis %d", i, axis);
                        break;
                    }

                    auto& stick = pad.m_sticks[axis];
                    stick.m_value = evt.value;
                }
                break;
            default:
                LOG_ERROR(GENERAL, "Unknown joystick #%d event %d", i, evt.type);
                break;
            }
        }

        int to_sleep = m_info.now_connect > 0 ? THREAD_SLEEP : THREAD_SLEEP_INACTIVE;
        usleep(milli2micro(to_sleep));
    }

    dead = true;
}

#endif
