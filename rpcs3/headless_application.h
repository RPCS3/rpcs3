#pragma once

#include <QCoreApplication>

#include "main_application.h"
#include "util/atomic.hpp"

#include <functional>

/** Headless RPCS3 Application Class
 * The main point of this class is to do application initialization and initialize callbacks.
*/

class headless_application : public QCoreApplication, public main_application
{
	Q_OBJECT
public:
	headless_application(int& argc, char** argv);

	/** Call this method before calling app.exec */
	bool Init() override;

private:
	void InitializeCallbacks();
	void InitializeConnects() const;

	QThread* get_thread() override
	{
		return thread();
	}

Q_SIGNALS:
	void RequestCallFromMainThread(std::function<void()> func, atomic_t<u32>* wake_up);

private Q_SLOTS:
	static void CallFromMainThread(const std::function<void()>& func, atomic_t<u32>* wake_up);
};
