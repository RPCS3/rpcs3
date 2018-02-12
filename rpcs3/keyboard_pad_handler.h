#pragma once

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"

#include <QWindow>
#include <QKeyEvent>

class keyboard_pad_handler final : public QObject, public PadHandlerBase
{
	// Unique button names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> mouse_list =
	{
		{ Qt::NoButton       , ""             },
		{ Qt::LeftButton     , "Mouse Left"   },
		{ Qt::RightButton    , "Mouse Right"  },
		{ Qt::MiddleButton   , "Mouse Middle" },
		{ Qt::BackButton     , "Mouse Back"   },
		{ Qt::ForwardButton  , "Mouse Fwd"    },
		{ Qt::TaskButton     , "Mouse Task"   },
		{ Qt::ExtraButton4   , "Mouse 4"      },
		{ Qt::ExtraButton5   , "Mouse 5"      },
		{ Qt::ExtraButton6   , "Mouse 6"      },
		{ Qt::ExtraButton7   , "Mouse 7"      },
		{ Qt::ExtraButton8   , "Mouse 8"      },
		{ Qt::ExtraButton9   , "Mouse 9"      },
		{ Qt::ExtraButton10  , "Mouse 10"     },
		{ Qt::ExtraButton11  , "Mouse 11"     },
		{ Qt::ExtraButton12  , "Mouse 12"     },
		{ Qt::ExtraButton13  , "Mouse 13"     },
		{ Qt::ExtraButton14  , "Mouse 14"     },
		{ Qt::ExtraButton15  , "Mouse 15"     },
		{ Qt::ExtraButton16  , "Mouse 16"     },
		{ Qt::ExtraButton17  , "Mouse 17"     },
		{ Qt::ExtraButton18  , "Mouse 18"     },
		{ Qt::ExtraButton19  , "Mouse 19"     },
		{ Qt::ExtraButton20  , "Mouse 20"     },
		{ Qt::ExtraButton21  , "Mouse 21"     },
		{ Qt::ExtraButton22  , "Mouse 22"     },
		{ Qt::ExtraButton23  , "Mouse 23"     },
		{ Qt::ExtraButton24  , "Mouse 24"     },
	};

public:
	bool Init() override;

	keyboard_pad_handler();

	void SetTargetWindow(QWindow* target);
	void processKeyEvent(QKeyEvent* event, bool pressed);
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);

	bool eventFilter(QObject* obj, QEvent* ev) override;

	void init_config(pad_config* cfg, const std::string& name) override;
	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;

	std::string GetMouseName(const QMouseEvent* event);
	std::string GetMouseName(u32 button);
	QStringList GetKeyNames(const QKeyEvent* keyEvent);
	std::string GetKeyName(const QKeyEvent* keyEvent);
	std::string GetKeyName(const u32& keyCode);
	u32 GetKeyCode(const std::string& keyName);
	u32 GetKeyCode(const QString& keyName);

protected:
	void Key(const u32 code, bool pressed, u16 value = 255);
	int GetModifierCode(QKeyEvent* e);

private:
	QWindow* m_target = nullptr;
	std::vector<std::shared_ptr<Pad>> bindings;
	u8 m_stick_min[4] = { 0, 0, 0, 0 };
	u8 m_stick_max[4] = { 128, 128, 128, 128 };
};
