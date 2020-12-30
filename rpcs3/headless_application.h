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
	void Init() override;

private:
	void InitializeCallbacks();
	void InitializeConnects();

	QThread* get_thread() override
	{
		return thread();
	};

Q_SIGNALS:
	void RequestCallAfter(const std::function<void()>& func);

private Q_SLOTS:
	void HandleCallAfter(const std::function<void()>& func);
};
