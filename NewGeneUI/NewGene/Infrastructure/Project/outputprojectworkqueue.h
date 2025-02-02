#ifndef OUTPUTPROJECTWORKQUEUE_H
#define OUTPUTPROJECTWORKQUEUE_H

#include <QListWidgetItem>

#include "Base/outputprojectworkqueue_base.h"

class UIOutputProject;

class OutputProjectWorkQueue : public WorkQueueManager<UI_OUTPUT_PROJECT>
{

	public:

		explicit OutputProjectWorkQueue(QObject * parent = NULL);

		void SetUIObject(void * ui_output_object_)
		{
			outp = ui_output_object_;
		}

		void SetConnections();

		UIOutputProject * get();

		// Called in context of Boost WORK POOL threads - NOT in context of this work queue manager's event loop thread
		void HandleChanges(DataChangeMessage & changes);

		void EmitMessage(std::string msg);

		void EmitOutputWidgetDataRefresh(WidgetDataItem_VARIABLE_GROUPS_SCROLL_AREA & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_VARIABLE_GROUPS_TOOLBOX & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_VARIABLE_GROUPS_SUMMARY_VARIABLE_GROUP_INSTANCE & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_KAD_SPIN_CONTROLS_AREA & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_KAD_SPIN_CONTROL_WIDGET & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_TIMERANGE_REGION_WIDGET & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_DATETIME_WIDGET & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_GENERATE_OUTPUT_TAB & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}
		void EmitOutputWidgetDataRefresh(WidgetDataItem_LIMIT_DMUS_TAB & widgetData)
		{
			emit WidgetDataRefresh(widgetData);
		}

	private:

		void * outp;

	protected:

		// ********************************* //
		// Slot Overrides
		// ********************************* //

		void TestSlot();

		// Data
		void RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUPS_SCROLL_AREA widget);
		void RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUPS_TOOLBOX widget);
		void RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUP_VARIABLE_GROUP_INSTANCE widget);
		void RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUPS_SUMMARY_SCROLL_AREA widget);
		void RefreshWidget(WidgetDataItemRequest_VARIABLE_GROUPS_SUMMARY_VARIABLE_GROUP_INSTANCE widget);
		void RefreshWidget(WidgetDataItemRequest_KAD_SPIN_CONTROLS_AREA widget);
		void RefreshWidget(WidgetDataItemRequest_KAD_SPIN_CONTROL_WIDGET widget);
		void RefreshWidget(WidgetDataItemRequest_DATETIME_WIDGET widget);
		void RefreshWidget(WidgetDataItemRequest_TIMERANGE_REGION_WIDGET widget);
		void RefreshWidget(WidgetDataItemRequest_GENERATE_OUTPUT_TAB widget);
		void RefreshWidget(WidgetDataItemRequest_LIMIT_DMUS_TAB widget);

		// Actions
		void ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_VARIABLE_GROUP_SET_MEMBER_SELECTION_CHANGED);
		void ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_KAD_COUNT_CHANGE);
		void ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_DO_RANDOM_SAMPLING_CHANGE);
		void ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_RANDOM_SAMPLING_COUNT_PER_STAGE_CHANGE);
		void ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_CONSOLIDATE_ROWS_CHANGE);
		void ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_DISPLAY_ABSOLUTE_TIME_COLUMNS_CHANGE);
		void ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_DATETIME_RANGE_CHANGE);
		void ReceiveVariableItemChanged(WidgetActionItemRequest_ACTION_GENERATE_OUTPUT);

		void DeleteDMU(WidgetActionItemRequest_ACTION_DELETE_DMU);
		void DeleteDMUMembers(WidgetActionItemRequest_ACTION_DELETE_DMU_MEMBERS);
		void DeleteUOA(WidgetActionItemRequest_ACTION_DELETE_UOA);
		void DeleteVG(WidgetActionItemRequest_ACTION_DELETE_VG);
		void SetVGDescriptions(WidgetActionItemRequest_ACTION_SET_VG_DESCRIPTIONS);
		void LimitDMUsChange(WidgetActionItemRequest_ACTION_LIMIT_DMU_MEMBERS_CHANGE);

};

#endif // OUTPUTPROJECTWORKQUEUE_H
