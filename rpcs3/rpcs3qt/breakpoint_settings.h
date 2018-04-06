#pragma once

#include "Utilities/types.h"

#include <QMap>
#include <QString>
#include <QSettings>


// Consider making a separate header just to hold this so that I don't have the enum I use and the data struct in different classes.
// TODO: Add minimum amount so that hardware breakpoints can be reconstructed when serialized.
// IF merging the two classes together (breakpoint handler, hardare breakpoints) doesn't happen, then use minimal amount possible.
struct breakpoint_data
{
	u32 flags;
	QString name;

	breakpoint_data(u32 flags, const QString& name) : flags(flags), name(name) {};
	breakpoint_data() : flags(0), name("breakpoint") {};

	bool operator ==(const breakpoint_data& data) { return flags == data.flags && name == data.name; };
};

/**
 * This class acts as a medium to write breakpoints to and from settings.  
 * I wanted to have it in a separate file for useability. Hence, separate file from guisettings. guiconfigs/breakpoints.ini
*/
class breakpoint_settings : public QObject
{
	Q_OBJECT
public:
	explicit breakpoint_settings(QObject* parent = nullptr);
	~breakpoint_settings();
	
	/*
	* Sets a breakpoint at the specified location.
	*/
	void SetBreakpoint(const QString& gameid, u32 addr, const breakpoint_data& bp_data);
	
	/*
	* Removes the breakpoint at specifiied location.
	*/
	void RemoveBreakpoint(const QString& gameid, u32 addr);

	/*
	* Reads all breakpoints from ini file. 
	*/
	QMap<QString, QMap<u32, breakpoint_data>> ReadBreakpoints();
private:
	QSettings m_bp_settings;
};
