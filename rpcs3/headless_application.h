#pragma once

#include <QCoreApplication>

#include "main_application.h"

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
	void RequestCallAfter(std::function<void()> func);

private Q_SLOTS:
	static void HandleCallAfter(const std::function<void()>& func);
};
