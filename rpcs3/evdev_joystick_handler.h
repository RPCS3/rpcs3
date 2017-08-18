#pragma once

#include "Utilities/types.h"
#include "Utilities/Config.h"
#include "Utilities/File.h"
#include "Emu/Io/PadHandler.h"
#include <libevdev/libevdev.h>
#include <vector>
#include <thread>

enum { EVDEV_DPAD_HAT_AXIS_X = -1, EVDEV_DPAD_HAT_AXIS_Y = -2 };

struct evdev_joystick_config final : cfg::node
{
    const std::string cfg_name = fs::get_config_dir() + "/config_linuxjoystick.yml";

    cfg::int32 select{this, "Select", 6};
    cfg::int32 start{this, "Start", 7};
    cfg::int32 cross{this, "Cross", 0};
    cfg::int32 circle{this, "Circle", 1};
    cfg::int32 square{this, "Square", 2};
    cfg::int32 triangle{this, "Triangle", 3};

    cfg::int32 r1{this, "R1", 5};
    cfg::int32 r2{this, "R2", 11};
    cfg::int32 r3{this, "R3", 10};
    cfg::int32 l1{this, "L1", 4};
    cfg::int32 l2{this, "L2", 12};
    cfg::int32 l3{this, "L3", 9};

    cfg::int32 up{this, "Up", EVDEV_DPAD_HAT_AXIS_Y};
    cfg::int32 down{this, "Down", EVDEV_DPAD_HAT_AXIS_Y};
    cfg::int32 left{this, "Left", EVDEV_DPAD_HAT_AXIS_X};
    cfg::int32 right{this, "Right", EVDEV_DPAD_HAT_AXIS_X};

    cfg::int32 rxstick{this, "Right stick X axis",  0};
    cfg::int32 rystick{this, "Right stick Y axis",  1};
    cfg::int32 lxstick{this, "Left stick X axis",  2};
    cfg::int32 lystick{this, "Left stick Y axis",  3};

    cfg::_bool rxreverse{this, "Reverse right stick X axis", false};
    cfg::_bool ryreverse{this, "Reverse right stick Y axis", false};
    cfg::_bool lxreverse{this, "Reverse left stick X axis", false};
    cfg::_bool lyreverse{this, "Reverse left stick Y axis", false};

    cfg::_bool axistrigger{this, "Z axis triggers", true};
    cfg::_bool squirclejoysticks{this, "Squircle Joysticks", true};
    cfg::int32 squirclefactor{this, "Squircle Factor", 5000};

    bool load()
    {
        if (fs::file cfg_file{ cfg_name, fs::read })
        {
            return from_string(cfg_file.to_string());
        }

        return false;
    }

    void save()
    {
        fs::file(cfg_name, fs::rewrite).write(to_string());
    }
};


class evdev_joystick_handler final : public PadHandlerBase
{
public:
    evdev_joystick_handler();
    ~evdev_joystick_handler();

    void Init(const u32 max_connect) override;
    void Close();

private:
    void update_devs();
    std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY);
    bool try_open_dev(u32 index);
    int scale_axis(int axis, int value);
    void thread_func();

    std::unique_ptr<std::thread> joy_thread;
    mutable atomic_t<bool> active{false}, dead{false};
    std::vector<std::string> joy_paths;
    std::vector<libevdev*> joy_devs;
    std::vector<std::vector<int>> joy_button_maps;
    std::vector<std::vector<int>> joy_axis_maps;
    // joy_axis is only used for squircling
    std::vector<std::vector<int>> joy_axis;
    std::vector<int> joy_hat_ids;
    bool axistrigger;
    std::map<int, std::pair<int, int>> axis_ranges;
    std::vector<bool> revaxis;
};
