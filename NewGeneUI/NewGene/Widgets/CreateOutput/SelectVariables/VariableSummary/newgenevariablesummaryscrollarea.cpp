#include "newgenevariablesummaryscrollarea.h"
#include "ui_newgenevariablesummaryscrollarea.h"

#include "../Project/uiprojectmanager.h"
#include "../Project/uiinputproject.h"
#include "../Project/uioutputproject.h"

NewGeneVariableSummaryScrollArea::NewGeneVariableSummaryScrollArea(QWidget * parent) :
	QWidget(parent),
	NewGeneWidget(WidgetCreationInfo(this, parent, WIDGET_NATURE_OUTPUT_WIDGET, VARIABLE_GROUPS_SUMMARY_SCROLL_AREA,
									 true)),   // 'this' pointer is cast by compiler to proper Widget instance, which is already created due to order in which base classes appear in class definition
	ui(new Ui::NewGeneVariableSummaryScrollArea)
{

	ui->setupUi(this);

	PrepareOutputWidget();

}

NewGeneVariableSummaryScrollArea::~NewGeneVariableSummaryScrollArea()
{
	delete ui;
}

void NewGeneVariableSummaryScrollArea::changeEvent(QEvent * e)
{
	QWidget::changeEvent(e);

	switch (e->type())
	{
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;

		default:
			break;
	}
}

void NewGeneVariableSummaryScrollArea::UpdateOutputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project)
{
	NewGeneWidget::UpdateOutputConnections(connection_type, project);

	if (connection_type == NewGeneWidget::ESTABLISH_CONNECTIONS_OUTPUT_PROJECT)
	{
		connect(this, SIGNAL(RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA)), outp->getConnector(),
				SLOT(RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA)));

		// *** This is a parent (top-level) widget, so connect to refreshes here (... child widgets don't connect to refreshes) *** //
		connect(project->getConnector(), SIGNAL(WidgetDataRefresh(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA)), this,
				SLOT(WidgetDataRefreshReceive(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA)));

		// *** Has child widgets, so refer refresh signals directed at child to be received by us, the parent *** //
		connect(project->getConnector(), SIGNAL(WidgetDataRefresh(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_VARIABLE_GROUP_INSTANCE)), this,
				SLOT(WidgetDataRefreshReceive(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_VARIABLE_GROUP_INSTANCE)));
	}
	else if (connection_type == NewGeneWidget::RELEASE_CONNECTIONS_OUTPUT_PROJECT)
	{
		cached_active_vg = WidgetInstanceIdentifier{};
		Empty();
	}
}

void NewGeneVariableSummaryScrollArea::UpdateInputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIInputProject * project)
{
	NewGeneWidget::UpdateInputConnections(connection_type, project);

	if (connection_type == NewGeneWidget::ESTABLISH_CONNECTIONS_INPUT_PROJECT)
	{
		if (project)
		{
			project->RegisterInterestInChange(this, DATA_CHANGE_TYPE__INPUT_MODEL__VG_CHANGE, false, "");
		}
	}
	else if (connection_type == NewGeneWidget::RELEASE_CONNECTIONS_INPUT_PROJECT)
	{
		if (inp)
		{
			inp->UnregisterInterestInChanges(this);
		}
	}
}

void NewGeneVariableSummaryScrollArea::RefreshAllWidgets()
{
	if (outp == nullptr)
	{
		Empty();
		return;
	}

	WidgetDataItemRequest_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA request(WIDGET_DATA_ITEM_REQUEST_REASON__REFRESH_ALL_WIDGETS);
	emit RefreshWidget(request);
}

void NewGeneVariableSummaryScrollArea::WidgetDataRefreshReceive(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA widget_data)
{

	Empty();

	std::for_each(widget_data.identifiers.cbegin(), widget_data.identifiers.cend(), [&](WidgetInstanceIdentifier const & identifier)
	{
		if (identifier.uuid && identifier.code && identifier.longhand)
		{
			WidgetInstanceIdentifier new_identifier(identifier);
			NewGeneVariableSummaryGroup * tmpGrp = new NewGeneVariableSummaryGroup(this, new_identifier, outp);
			tmpGrp->setTitle(identifier.longhand->c_str());
			layout()->addWidget(tmpGrp);

			if (new_identifier.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID_PLUS_STRING_CODE, cached_active_vg))
			{
				DoTabChange(new_identifier); // pick up any metadata changes?
			}
		}
	});

}

void NewGeneVariableSummaryScrollArea::WidgetDataRefreshReceive(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_VARIABLE_GROUP_INSTANCE widget_data)
{
	if (widget_data.identifier && widget_data.identifier->uuid)
	{
		NewGeneWidget * child = outp->FindWidgetFromDataItem(uuid, *widget_data.identifier->uuid);

		if (child)
		{
			child->WidgetDataRefreshReceive(widget_data);
		}
	}
}

void NewGeneVariableSummaryScrollArea::Empty()
{
	//cached_active_vg = WidgetInstanceIdentifier{}; // No - this is sometimes called after the variable selection widget has loaded, but before this widget has begun loading - we need to save the cached value
	DoTabChange(cached_active_vg);
	QLayoutItem * child;

	while ((child = layout()->takeAt(0)) != 0)
	{
		delete child->widget();
		delete child;
	}
}


void NewGeneVariableSummaryScrollArea::HandleChanges(DataChangeMessage const & change_message)
{

	UIOutputProject * project = projectManagerUI().getActiveUIOutputProject();

	if (project == nullptr)
	{
		return;
	}

	UIMessager messager(project);

	std::for_each(change_message.changes.cbegin(), change_message.changes.cend(), [this](DataChange const & change)
	{
		switch (change.change_type)
		{
			case DATA_CHANGE_TYPE::DATA_CHANGE_TYPE__INPUT_MODEL__VG_CHANGE:
				{
					switch (change.change_intention)
					{

						case DATA_CHANGE_INTENTION__ADD:
							{

								RefreshAllWidgets(); // this triggers a resort by loading everything in the pane again

							}
							break;

						case DATA_CHANGE_INTENTION__REMOVE:
							{

								if (change.parent_identifier.code && change.parent_identifier.uuid)
								{

									WidgetInstanceIdentifier vg_to_remove(change.parent_identifier);

									int current_number = layout()->count();
									bool found = false;
									QWidget * widgetToRemove = nullptr;
									QLayoutItem * layoutItemToRemove = nullptr;
									int i = 0;

									for (i = 0; i < current_number; ++i)
									{
										QLayoutItem * testLayoutItem = layout()->itemAt(i);
										QWidget * testWidget(testLayoutItem->widget());

										try
										{
											NewGeneVariableSummaryGroup * testVG = dynamic_cast<NewGeneVariableSummaryGroup *>(testWidget);

											if (testVG && testVG->data_instance.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID_PLUS_STRING_CODE, vg_to_remove))
											{
												widgetToRemove = testVG;
												layoutItemToRemove = testLayoutItem;
												found = true;

												if (testVG->data_instance.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID_PLUS_STRING_CODE, cached_active_vg))
												{
													// No need to call DoTabChange as the entire block will be removed, but save the new empty value into the cache variable
													cached_active_vg = WidgetInstanceIdentifier{};
												}

												break;
											}
										}
										catch (std::bad_cast &)
										{
											// guess not
										}

									}

									if (found && widgetToRemove != nullptr)
									{
										layout()->takeAt(i);
										delete widgetToRemove;
										delete layoutItemToRemove;
										widgetToRemove = nullptr;
										layoutItemToRemove = nullptr;
									}

								}

							}
							break;

						case DATA_CHANGE_INTENTION__UPDATE:
							{
								// Should never receive this.
							}
							break;

						case DATA_CHANGE_INTENTION__RESET_ALL:
							{
								// Ditto above.
								RefreshAllWidgets();
							}
							break;

						default:
							{
							}
							break;

					}
				}
				break;

			default:
				{
				}
				break;
		}
	});

}

void NewGeneVariableSummaryScrollArea::DoTabChange(WidgetInstanceIdentifier data)
{
	cached_active_vg = data;

	int current_number = layout()->count();
	bool found = false;

	for (int i = 0; i < current_number; ++i)
	{
		QLayoutItem * layoutItem = layout()->itemAt(i);

		if (layoutItem)
		{
			auto vg = dynamic_cast<NewGeneVariableSummaryGroup *>(layoutItem->widget());

			if (vg)
			{
				if (vg->data_instance.IsEqual(WidgetInstanceIdentifier::EQUALITY_CHECK_TYPE__UUID_PLUS_STRING_CODE, data))
				{
					vg->setStyleSheet("QGroupBox {font-weight: bold; color: #101078;}");
				}
				else
				{
					vg->setStyleSheet("QGroupBox {font-weight: normal; color: black;}");
				}
			}
		}
	}
}
