import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Window 2.1
import GLViewer 1.0

ApplicationWindow {
	visible: true
	title: qsTr("RPCS3 Qt")
	width: 500
	height: 500
	menuBar: MenuBar {
		Menu {
			title: qsTr("&Boot")
			MenuItem { text: qsTr("&Boot Game...") }
			MenuItem { text: qsTr("&Install PKG") }
			MenuSeparator {}
			MenuItem { text: qsTr("Boot &ELF") }
			MenuItem { text: qsTr("Boot &SELF") }
			MenuSeparator {}
			MenuItem { text: qsTr("E\&xit"); onTriggered: Qt.quit() }
		}
		Menu {
			title: qsTr("&System")
			MenuItem { text: qsTr("&Pause") }
			MenuItem { text: qsTr("&Stop") }
			MenuSeparator {}
			MenuItem { text: qsTr("Send '&Open System Menu' command") }
			MenuItem { text: qsTr("Send 'E&xit' command") }
		}
		Menu {
			title: qsTr("&Config")
			MenuItem { text: qsTr("&Settings") }
			MenuSeparator {}
			MenuItem { text: qsTr("Virtual &File System Manager") }
			MenuItem { text: qsTr("Virtual &HDD Manager") }
		}
		Menu {
			title: qsTr("&Tools")
			MenuItem { text: qsTr("&ELF Compiler") }
			MenuItem { text: qsTr("&Memory Viewer") }
		}
		Menu {
			title: qsTr("&Help")
			MenuItem { text: qsTr("&About...") }
		}
	}
	GLViewer {}
	Rectangle {
		color: Qt.rgba(0, 0.5, 0.35);
		height: Math.round(parent.height / 2)
		width: height
		radius: width
		anchors.centerIn: parent
		Text {
			anchors.centerIn: parent
			font.pixelSize: parent.height / 2
			text: "Qt"
		}
	}

}
