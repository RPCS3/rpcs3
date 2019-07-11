#pragma once

#include "stdafx.h"

#include <QCoreApplication>

#include "main_application.h"

/** RPCS3 Application Class
 * The main point of this class is to do application initialization and initialize callbacks.
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
