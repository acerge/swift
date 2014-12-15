/*
 * Copyright (c) 2014 Kevin Smith and Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include <Swift/QtUI/Trellis/QtDynamicGridLayout.h>

#include <cassert>

#include <QLayoutItem>
#include <QGridLayout>
#include <QtDebug>

#include <Swift/QtUI/QtSwiftUtil.h>
#include <Swift/QtUI/QtTabbable.h>
#include <Swift/QtUI/QtTabWidget.h>
#include <Swift/QtUI/Trellis/QtDNDTabBar.h>

namespace Swift {

QtDynamicGridLayout::QtDynamicGridLayout(QWidget* parent, bool enableDND) : QWidget(parent), dndEnabled_(enableDND) {
	gridLayout_ = new QGridLayout(this);
	setContentsMargins(0,0,0,0);
	setDimensions(QSize(1,1));
}

QtDynamicGridLayout::~QtDynamicGridLayout() {
}

int QtDynamicGridLayout::addTab(QtTabbable* tab, const QString& title) {
	assert(gridLayout_->rowCount() > 0 && gridLayout_->columnCount() > 0);
	int index = -1;

	QPoint lastPos(0,0);
	if (tabPositions_.contains(P2QSTRING(tab->getID()))) {
		lastPos = tabPositions_[P2QSTRING(tab->getID())];
	}

	lastPos = QPoint(qMin(lastPos.x(), gridLayout_->columnCount() - 1), qMin(lastPos.y(), gridLayout_->rowCount() - 1));

	QLayoutItem* item = gridLayout_->itemAtPosition(lastPos.y(), lastPos.x());
	QtTabWidget* tabWidget = dynamic_cast<QtTabWidget*>(item ? item->widget() : 0);
	if (tabWidget) {
		index = tabWidget->addTab(tab, title);
	}
	return tabWidget ? indexOf(tab) : -1;
}

int QtDynamicGridLayout::count() const {
	int count = 0;
	for (int y = 0; y < gridLayout_->rowCount(); y++) {
		for (int x = 0; x < gridLayout_->columnCount(); x++) {
			QLayoutItem* layoutItem = gridLayout_->itemAtPosition(y, x);
			QtTabWidget* tabWidget = dynamic_cast<QtTabWidget*>(layoutItem->widget());
			if (tabWidget) {
				count += tabWidget->count();
			}
		}
	}
	return count;
}

QWidget* QtDynamicGridLayout::widget(int index) const {
	QWidget* widgetAtIndex = NULL;
	for (int y = 0; y < gridLayout_->rowCount(); y++) {
		for (int x = 0; x < gridLayout_->columnCount(); x++) {
			QLayoutItem* layoutItem = gridLayout_->itemAtPosition(y, x);
			QtTabWidget* tabWidget = dynamic_cast<QtTabWidget*>(layoutItem->widget());
			if (tabWidget) {
				if (index < tabWidget->count()) {
					widgetAtIndex = tabWidget->widget(index);
					return widgetAtIndex;
				}
				else {
					index -= tabWidget->count();
				}
			}
		}
	}
	return widgetAtIndex;
}

int QtDynamicGridLayout::indexOf(const QWidget* widget) const {
	int index = 0;
	if (widget) {
		for (int y = 0; y < gridLayout_->rowCount(); y++) {
			for (int x = 0; x < gridLayout_->columnCount(); x++) {
				QLayoutItem* layoutItem = gridLayout_->itemAtPosition(y, x);
				QtTabWidget* tabWidget = dynamic_cast<QtTabWidget*>(layoutItem->widget());
				if (tabWidget) {
					for (int n = 0; n < tabWidget->count(); n++) {
						QWidget* nthWidget = tabWidget->widget(n);
						if (nthWidget == widget) {
							return index;
						}
						index++;
					}
				}
			}
		}
	}
	return -1;
}

int QtDynamicGridLayout::currentIndex() const {
	return indexOf(currentWidget());
}

void QtDynamicGridLayout::setCurrentIndex(int index) {
	int tabIndex = -1;
	QtTabWidget* tabWidget = indexToTabWidget(index, tabIndex);
	if (tabIndex >= 0) {
		tabWidget->setCurrentIndex(tabIndex);
		if (!tabWidget->hasFocus()) {
			tabWidget->widget(tabIndex)->setFocus(Qt::TabFocusReason);
		}
	} else {
		assert(false);
	}
}

void QtDynamicGridLayout::removeTab(int index) {
	int tabIndex = -1;
	QtTabWidget* tabWidget = indexToTabWidget(index, tabIndex);
	if (tabWidget) {
		tabWidget->removeTab(tabIndex);
	}
}

QWidget* QtDynamicGridLayout::currentWidget() const {
	QWidget* current = NULL;
	current = focusWidget();
	while (current && !dynamic_cast<QtTabbable*>(current)) {
		if (current->parentWidget()) {
			current = current->parentWidget();
		} else {
			current = NULL;
			break;
		}
	}
	if (!current) {
		current = widget(0);
	}
	return current;
}

void QtDynamicGridLayout::setCurrentWidget(QWidget* widget) {
	for (int y = 0; y < gridLayout_->rowCount(); y++) {
		for (int x = 0; x < gridLayout_->columnCount(); x++) {
			QLayoutItem* layoutItem = gridLayout_->itemAtPosition(y, x);
			QtTabWidget* tabWidget = dynamic_cast<QtTabWidget*>(layoutItem->widget());
			if (tabWidget) {
				for (int n = 0; n < tabWidget->count(); n++) {
					if (tabWidget->widget(n) == widget) {
						tabWidget->setCurrentWidget(widget);
					}
				}
			}
		}
	}
}

QtTabWidget* QtDynamicGridLayout::indexToTabWidget(int index, int& tabIndex) {
	for (int y = 0; y < gridLayout_->rowCount(); y++) {
		for (int x = 0; x < gridLayout_->columnCount(); x++) {
			QLayoutItem* layoutItem = gridLayout_->itemAtPosition(y, x);
			QtTabWidget* tabWidget = dynamic_cast<QtTabWidget*>(layoutItem->widget());
			if (tabWidget) {
				if (index < tabWidget->count()) {
					tabIndex = index;
					return tabWidget;
				}
				else {
					index -= tabWidget->count();
					if (index < 0) {
						qWarning() << "Called QtDynamicGridLayout::setCurrentIndex with index out of bounds: index = " << index;
						tabIndex = -1;
						return NULL;
					}
				}
			}
		}
	}
	tabIndex = -1;
	return NULL;
}

bool QtDynamicGridLayout::isDNDEnabled() const {
	return dndEnabled_;
}

QHash<QString, QPoint> QtDynamicGridLayout::getTabPositions() const {
	return tabPositions_;
}

void QtDynamicGridLayout::setTabPositions(const QHash<QString, QPoint> positions) {
	tabPositions_ = positions;
}

QSize QtDynamicGridLayout::getDimension() const {
	return QSize(gridLayout_->columnCount(), gridLayout_->rowCount());
}

void QtDynamicGridLayout::setDimensions(const QSize& dim) {
	assert(dim.width() > 0 && dim.height() > 0);
	setUpdatesEnabled(false);

	QGridLayout* oldLayout = dynamic_cast<QGridLayout*>(layout());
	QGridLayout* newLayout = new QGridLayout;
	newLayout->setSpacing(4);
	newLayout->setContentsMargins(0,0,0,0);

	int oldWidth = oldLayout->columnCount();
	int oldHeight = oldLayout->rowCount();
	int maxCol = qMax(oldWidth, dim.width());
	int minCol = qMin(oldWidth, dim.width());
	int maxRow = qMax(oldHeight, dim.height());
	int minRow = qMin(oldHeight, dim.height());

	for (int row = 0; row < maxRow; row++) {
		for (int col = 0; col < maxCol; col++) {
			QLayoutItem* oldItem = oldLayout->itemAtPosition(row, col);
			QLayoutItem* newItem = newLayout->itemAtPosition(row, col);
			bool removeRow = !(row < dim.height());
			bool removeCol = !(col < dim.width());

			if (removeCol || removeRow) {
				if (oldItem) {
					int squeezeRow = removeRow ? (minRow - 1) : row;
					int squeezeCol = removeCol ? (minCol - 1) : col;
					newItem = newLayout->itemAtPosition(squeezeRow, squeezeCol);
					if (!newItem) {
						newLayout->addWidget(createDNDTabWidget(this), squeezeRow, squeezeCol);
						newItem = newLayout->itemAtPosition(squeezeRow, squeezeCol);
					}
					QtTabWidget* oldTabWidget = dynamic_cast<QtTabWidget*>(oldItem->widget());
					QtTabWidget* newTabWidget = dynamic_cast<QtTabWidget*>(newItem->widget());
					assert(oldTabWidget && newTabWidget);

					oldTabWidget->hide();
					while(oldTabWidget->count()) {
						QIcon icon = oldTabWidget->tabIcon(0);
						QString text = oldTabWidget->tabText(0);
						newTabWidget->addTab(oldTabWidget->widget(0), icon, text);
					}
					delete oldTabWidget;
				}
			} else {
				if (oldItem) {
					newLayout->addWidget(oldItem->widget(), row, col);
					newItem = newLayout->itemAtPosition(row, col);
				} else {
					newLayout->addWidget(createDNDTabWidget(this), row, col);
				}
			}
		}
	}

	for (int col = 0; col < dim.width(); col++) {
		newLayout->setColumnStretch(col, 1);
	}
	for (int row = 0; row < dim.height(); row++) {
		newLayout->setRowStretch(row, 1);
	}

	setUpdatesEnabled(true);
	delete layout();
	setLayout(newLayout);
	gridLayout_ = newLayout;
}

void QtDynamicGridLayout::moveCurrentTabRight() {
	int index = currentIndex();
	int tabIndex = -1;
	QtTabWidget* tabWidget = indexToTabWidget(index, tabIndex);
	assert(tabWidget);
	int newTabIndex = (tabIndex + 1) % tabWidget->count();
	moveTab(tabWidget, tabIndex, newTabIndex);
}

void QtDynamicGridLayout::moveCurrentTabLeft() {
	int index = currentIndex();
	int tabIndex = -1;
	QtTabWidget* tabWidget = indexToTabWidget(index, tabIndex);
	assert(tabWidget);
	int newTabIndex = (tabWidget->count() + tabIndex - 1) % tabWidget->count();
	moveTab(tabWidget, tabIndex, newTabIndex);
}

void QtDynamicGridLayout::moveCurrentTabToNextGroup() {
	int index = currentIndex();
	int tabIndex = -1;
	QtTabWidget* tabWidget = indexToTabWidget(index, tabIndex);

	int row = -1;
	int col = -1;
	int tmp;
	gridLayout_->getItemPosition(gridLayout_->indexOf(tabWidget), &row, &col, &tmp, &tmp);

	// calculate next cell
	col++;
	if (!(col < gridLayout_->columnCount())) {
		col = 0;
		row++;
		if (!(row < gridLayout_->rowCount())) {
			row = 0;
		}
	}

	QtTabWidget* targetTabWidget = dynamic_cast<QtTabWidget*>(gridLayout_->itemAtPosition(row, col)->widget());
	assert(tabWidget);
	assert(targetTabWidget);

	// fetch tab information
	QWidget* tab = tabWidget->widget(tabIndex);
	QString tabText = tabWidget->tabText(tabIndex);

	// move tab
	tab->blockSignals(true);
	targetTabWidget->addTab(tab, tabText);
	tab->blockSignals(false);
	tab->setFocus(Qt::TabFocusReason);

	updateTabPositions();
}

void QtDynamicGridLayout::moveCurrentTabToPreviousGroup() {
	int index = currentIndex();
	int tabIndex = -1;
	QtTabWidget* tabWidget = indexToTabWidget(index, tabIndex);

	int row = -1;
	int col = -1;
	int tmp;
	gridLayout_->getItemPosition(gridLayout_->indexOf(tabWidget), &row, &col, &tmp, &tmp);

	// calculate next cell
	col--;
	if (col < 0) {
		col = gridLayout_->columnCount() - 1;
		row--;
		if (row < 0) {
			row = gridLayout_->rowCount() - 1;
		}
	}

	QtTabWidget* targetTabWidget = dynamic_cast<QtTabWidget*>(gridLayout_->itemAtPosition(row, col)->widget());
	assert(tabWidget);
	assert(targetTabWidget);

	// fetch tab information
	QWidget* tab = tabWidget->widget(tabIndex);
	QString tabText = tabWidget->tabText(tabIndex);

	// move tab
	tab->blockSignals(true);
	targetTabWidget->addTab(tab, tabText);
	tab->blockSignals(false);
	tab->setFocus(Qt::TabFocusReason);

	updateTabPositions();
}

void QtDynamicGridLayout::handleTabCloseRequested(int index) {
	updateTabPositions();
	QtTabWidget* tabWidgetSender = dynamic_cast<QtTabWidget*>(sender());
	for (int y = 0; y < gridLayout_->rowCount(); y++) {
		for (int x = 0; x < gridLayout_->columnCount(); x++) {
			QLayoutItem* layoutItem = gridLayout_->itemAtPosition(y, x);
			QtTabWidget* tabWidget = dynamic_cast<QtTabWidget*>(layoutItem->widget());
			if (tabWidget == tabWidgetSender) {
				tabCloseRequested(index);
			}
			else {
				index += tabWidget->count();
			}
		}
	}
}

void QtDynamicGridLayout::updateTabPositions() {
	for (int y = 0; y < gridLayout_->rowCount(); y++) {
		for (int x = 0; x < gridLayout_->columnCount(); x++) {
			QLayoutItem* layoutItem = gridLayout_->itemAtPosition(y, x);
			QtTabWidget* tabWidget = dynamic_cast<QtTabWidget*>(layoutItem->widget());
			assert(tabWidget);
			for (int index = 0; index < tabWidget->count(); index++) {
				QtTabbable* tab = dynamic_cast<QtTabbable*>(tabWidget->widget(index));
				assert(tab);
				tabPositions_.insert(P2QSTRING(tab->getID()), QPoint(x, y));
			}
		}
	}
}

void QtDynamicGridLayout::moveTab(QtTabWidget* tabWidget, int oldIndex, int newIndex) {
	tabWidget->widget(oldIndex)->blockSignals(true);
	tabWidget->widget(newIndex)->blockSignals(true);
#if QT_VERSION >= 0x040500
	tabWidget->tabBar()->moveTab(oldIndex, newIndex);
#else
#warning Qt 4.5 or later is needed. Trying anyway, some things will be disabled.
	tabWidget->widget(oldIndex)->blockSignals(false);
	tabWidget->widget(newIndex)->blockSignals(false);
#endif
}

QtTabWidget* QtDynamicGridLayout::createDNDTabWidget(QWidget* parent) {
	QtTabWidget* tab = new QtTabWidget(parent);
	if (dndEnabled_) {
		QtDNDTabBar* tabBar = new QtDNDTabBar(tab);
		connect(tabBar, SIGNAL(onDropSucceeded()), this, SLOT(updateTabPositions()));
		tab->setTabBar(tabBar);
	}
	tab->setUsesScrollButtons(true);
	tab->setElideMode(Qt::ElideRight);
#if QT_VERSION >= 0x040500
	/*For Macs, change the tab rendering.*/
	tab->setDocumentMode(true);
	/*Closable tabs are only in Qt4.5 and later*/
	tab->setTabsClosable(true);
	tab->setMovable(true);
	connect(tab, SIGNAL(tabCloseRequested(int)), this, SLOT(handleTabCloseRequested(int)));
#else
#warning Qt 4.5 or later is needed. Trying anyway, some things will be disabled.
#endif
	return tab;
}

}