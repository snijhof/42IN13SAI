#include "OutputWindow.h"
#include <iostream>

OutputWindow::OutputWindow(QWidget *parent) : QListView(parent)
{
	this->setFont(QFont("Consolas", 9));
#ifndef _WIN32
	// Set font to bigger size for readability on Mac OS X
	this->setFont(QFont("Consolas", 12));
#endif

	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionMode(QAbstractItemView::NoSelection);

	listModel = new QStringListModel(output, nullptr);
    
	setModel(listModel);
}

void OutputWindow::SetTheme(std::map<std::string, QString> colors)
{
    this->setStyleSheet("QTextEdit, QListWidget { color: white; background-color: rgb( "+ colors["background"] +"); border-style: solid; border-width: 1px; border-color: black; } QTabWidget::pane { background-color: rgb( "+ colors["background"] +") } QTabBar::tab { color: white; background-color: rgb( "+ colors["background"] +") border-style: solid; border-width: 1px; border-color: black; padding: 3px;} QTabBar::tab:selected { background-color: rgb( "+ colors["background"] +") }");
}

void OutputWindow::addOutput(std::string strOutput)
{
	bool is_number = true;
	try {
		std::stof(strOutput);
	}
	catch (const std::exception& e)
	{
		is_number = false;
	}

	QString str = QString::fromUtf8(strOutput.c_str());
	// Algoritm for output precision
	if (is_number)
	{
		str = setOutputPrecision(str);
	}
	output << str;

	listModel->setStringList(output);
}

QString OutputWindow::setOutputPrecision(QString str)
{
	std::vector<int> charsToRemove;
	for (int i = 0; i < str.count(); i++)
	{
		if (str[i] == '.')
		{
			int k = i + 1;
			for (int j = k; j < str.count(); j++)
			{
				if (str[j] == '0' || str[j] == '\n')
					charsToRemove.push_back(j);
				else
					charsToRemove.clear();
			}
		}
	}

	for (int s = charsToRemove.size() - 1; s >= 0; s--)
	{
		str = str.remove(charsToRemove[s], 1);
	}

	if (str[str.count() - 1] == '.')
		str = str.remove(str.count() - 1, 1);

	return str;
}

void OutputWindow::clearOutput()
{
	output.clear();
	listModel->setStringList(output);
}

OutputWindow::~OutputWindow()
{
	delete listModel;
}