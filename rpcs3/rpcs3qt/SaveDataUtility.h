#ifndef SAVEDATAUTILITY_H
#define SAVEDATAUTILITY_H

#include <QDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>
#include <QHeaderView>
#include <QMenu>

//TODO: Implement function calls related to Save Data List.
//Those function calls may be needed to use this GUI.
//Currently this is only a stub.

//A stub for the struct sent to SaveDataInfoDialog.
struct SaveDataInformation
{

};
//A stub for the sorting.
enum
{
	SAVE_DATA_LIST_SORT_BY_USERID
};
//A stub for a single entry of save data. used to make a save data list or do management.
struct SaveDataEntry
{

};

//Used to display the information of a savedata.
//Not sure about what information should be displayed.
class SaveDataInfoDialog :public QDialog
{
	Q_OBJECT

	QTableWidget* m_list;

	void UpdateData();
public:
	SaveDataInfoDialog(QWidget* parent, const SaveDataInformation& info);
};

//Simple way to show up the sort menu and other operations
//Like what you get when press Triangle on SaveData.
class SaveDataManageDialog :public QDialog
{
	Q_OBJECT

	QComboBox* m_sort_options;
	unsigned int* m_sort_type;

private slots:
	void OnInfo();
	void OnCopy();
	void OnDelete();
	void OnApplySort();
public:
	SaveDataManageDialog(QWidget* parent, unsigned int* sort_type, SaveDataEntry& save);
};

//Display a list of SaveData. Would need to be initialized.
//Can also be used as a Save Data Chooser.
class SaveDataListDialog : public QDialog
{
	Q_OBJECT

	QTableWidget* m_list;
	QMenu* m_sort_options;
	unsigned int m_sort_type;
	unsigned int m_sort_type_count;

	int m_sortColumn;
	bool m_sortAscending;

	void LoadEntries(void);
	void UpdateList(void);
public:
	SaveDataListDialog(QWidget* parent, bool enable_manage);
private:
	QAction* userIDAct;
	QAction* titleAct;
	QAction* subtitleAct;
	QAction* copyAct;
	QAction* removeAct;
	QAction* infoAct;

	void OnSort(int id);
private slots:
	void OnSelect();
	void OnManage();
	void OnEntryCopy();
	void OnEntryRemove();
	void OnEntryInfo();
	void ShowContextMenu(const QPoint &pos);
};

#endif // !SAVEDATAUTILITY_H
