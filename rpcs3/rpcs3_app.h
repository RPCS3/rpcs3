#pragma once

#include "stdafx.h"

#include "main_application.h"

#include <QCoreApplication>

/** RPCS3 Application Class
 * The point of this class is to do application initialization and to hold onto the main window. The main thing I intend this class to do, for now, is to initialize callbacks and the main_window.
*/

class rpcs3_app : public QCoreApplication, public main_application
{
	Q_OBJECT
public:
	rpcs3_app(int& argc, char** argv);

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
