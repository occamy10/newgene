#include "uiinputproject.h"
#include "../../Widgets/newgenewidget.h"
#include <QListView>
#include <QTimer>
#include "newgenemainwindow.h"

void UIInputProject::SignalMessageBox(STD_STRING msg)
{
	QMessageBox msgBox;
	msgBox.setText(msg.c_str());
	msgBox.exec();
}

bool UIInputProject::QuestionMessageBox(STD_STRING msg_title, STD_STRING msg_text)
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(nullptr, QString(msg_title.c_str()), QString(msg_text.c_str()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

	if (reply == QMessageBox::Yes)
	{
		return true;
	}

	return false;
}

bool UIInputProject::is_model_equivalent(UIMessager & messager, UIInputModel * model_)
{
	if (!model_)
	{
		return false;
	}

	if (&model() == model_ && &model().backend() == &model_->backend())
	{
		boost::filesystem::path this_path = model().backend().getPathToDatabaseFile();
		boost::filesystem::path that_path = model_->backend().getPathToDatabaseFile();

		try
		{
			bool this_exists = boost::filesystem::exists(this_path);
			bool that_exists = boost::filesystem::exists(that_path);

			if (this_exists != that_exists)
			{
				return false;
			}

			if (!this_exists && !that_exists)
			{
				return true;
			}

			boost::filesystem::path this_path_canonical = boost::filesystem::canonical(this_path);
			boost::filesystem::path that_path_canonical = boost::filesystem::canonical(that_path);

			if (!boost::filesystem::equivalent(this_path_canonical, that_path_canonical))
			{
				return false;
			}
		}
		catch (boost::filesystem::filesystem_error & ex)
		{
			boost::format msg("Error during input model equivalence determination in UIInputProject evaluating input model database file pathnames (%1% vs. %2%): %3%");
			msg % this_path.string() % that_path.string() % ex.what();
			messager.AppendMessage(new MessagerWarningMessage(MESSAGER_MESSAGE__FILE_DOES_NOT_EXIST, msg.str()));
			return false;
		}
	}

	return true;
}

void UIInputProject::UpdateConnections()
{
	connect(getConnector(), SIGNAL(DataChangeMessageSignal(WidgetChangeMessages)), this, SLOT(DataChangeMessageSlot(WidgetChangeMessages)));
}

void UIInputProject::DoRefreshAllWidgets()
{
	emit RefreshAllWidgets();
}

// Called in UI thread
void UIInputProject::DataChangeMessageSlot(WidgetChangeMessages widget_change_messages)
{
	DisplayChanges(widget_change_messages);
}

void UIInputProject::PassChangeMessageToWidget(NewGeneWidget * widget, DataChangeMessage const & change_message)
{
	widget->HandleChanges(change_message);
}

void UIInputProject::PauseLists()
{
	NewGeneMainWindow * mainWindow = dynamic_cast<NewGeneMainWindow *>(mainWindowObject);
	if (mainWindow)
	{
		foreach (QListView * listPane, mainWindow->findChildren<QListView *>())
		{
			if (listPane->metaObject()->className() == QString("QListView"))
			{
				listPane->setUpdatesEnabled(false);
				QTimer::singleShot(2000, this, SLOT(UnpauseList(listPane)));
			}
		}
	}
}

void UIInputProject::UnpauseList(QListView * listPane)
{
	// In the worst case - a race condition in which the user clicks on a heavy-hitting activity
	// a few seconds AFTER a previous heavy-hitting activity paused the lists -
	// the "disable list updates" triggered on the second heavy-hitting activity
	// will be undone by the "enable list updates" single-shot that expires from the first heavy-hitting activity.
	// This isn't great because the list view might hang things refreshing itself over and over,
	// but it's not a crash and it's pretty rare that will happen, so stick with this approach for now.
	listPane->setUpdatesEnabled(true);
	listPane->update();
}
