#pragma once

#include "util/types.hpp"

#include <QDialog>

#include <atomic>
#include <memory>
#include <vector>

class QCloseEvent;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QThread;
class iso_archive;

class disc_compression_manager_dialog final : public QDialog
{
	Q_OBJECT

public:
	explicit disc_compression_manager_dialog(QWidget* parent = nullptr);
	~disc_compression_manager_dialog() override;

protected:
	void closeEvent(QCloseEvent* event) override;

private:
	struct queue_item
	{
		QString source;
		QString output;
		QString format;
		QString key_text;
		QString base_status;
		u64 input_size = 0;
		bool compressible = false;
		bool runnable = false;
	};

	void add_games();
	void add_source(const QString& source);
	void remove_selected();
	void clear_queue();
	void choose_output_directory();
	void update_output_paths();
	void refresh_table();
	void refresh_buttons();
	void start_compression();
	void set_running(bool running);
	QString output_path_for(const queue_item& item) const;
	QString format_description(const iso_archive& archive) const;

	QTableWidget* m_table = nullptr;
	QLineEdit* m_output_directory = nullptr;
	QLabel* m_current_label = nullptr;
	QProgressBar* m_progress = nullptr;
	QPushButton* m_add_button = nullptr;
	QPushButton* m_remove_button = nullptr;
	QPushButton* m_clear_button = nullptr;
	QPushButton* m_compress_button = nullptr;
	QPushButton* m_cancel_button = nullptr;
	QPushButton* m_close_button = nullptr;
	std::unique_ptr<QThread> m_thread;
	std::vector<queue_item> m_items;
	std::atomic_bool m_cancel{false};
	bool m_running = false;
};
