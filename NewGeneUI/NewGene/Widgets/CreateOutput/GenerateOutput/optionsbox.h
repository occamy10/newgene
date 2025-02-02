#ifndef OPTIONSBOX_H
#define OPTIONSBOX_H

#include <QFrame>
#include "../../../newgenewidget.h"

namespace Ui
{
	class OptionsBox;
}

class OptionsBox : public QFrame, public NewGeneWidget // do not reorder base classes; QWidget instance must be instantiated first
{

		Q_OBJECT

	public:

		explicit OptionsBox(QWidget * parent = 0);
		~OptionsBox();

	protected:

		void changeEvent(QEvent * e);

	private:

		Ui::OptionsBox * ui;

	public:

	signals:
		void UpdateDoRandomSampling(WidgetActionItemRequest_ACTION_DO_RANDOM_SAMPLING_CHANGE);
		void UpdateRandomSamplingCount(WidgetActionItemRequest_ACTION_RANDOM_SAMPLING_COUNT_PER_STAGE_CHANGE);
		void UpdateConsolidateRows(WidgetActionItemRequest_ACTION_CONSOLIDATE_ROWS_CHANGE);
		void UpdateDisplayAbsoluteTimeColumns(WidgetActionItemRequest_ACTION_DISPLAY_ABSOLUTE_TIME_COLUMNS_CHANGE);
		void RefreshWidget(WidgetDataItemRequest_TIMERANGE_REGION_WIDGET);

	public slots:
		void UpdateOutputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIOutputProject * project);
		void UpdateInputConnections(NewGeneWidget::UPDATE_CONNECTIONS_TYPE connection_type, UIInputProject * project);
		void RefreshAllWidgets();
		void WidgetDataRefreshReceive(WidgetDataItem_TIMERANGE_REGION_WIDGET);
		void SelectAndSetKadOutputPath();
		void EditingFinishedKadOutputPath();

	private slots:
		void on_doRandomSampling_stateChanged(int arg1);
		void on_randomSamplingHowManyRows_textChanged(const QString & arg1);
		void on_mergeIdenticalRows_stateChanged(int arg1);
		void on_displayAbsoluteTimeColumns_stateChanged(int arg1);

	protected:

		void setFilenameInSelectionEditBox();

};

#endif // OPTIONSBOX_H
