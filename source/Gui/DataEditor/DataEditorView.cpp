﻿#include <QTabWidget>
#include <QGridLayout>

#include "Gui/Settings.h"
#include "ClusterEditTab.h"
#include "CellEditTab.h"
#include "MetadataEditTab.h"
#include "CellComputerEditTab.h"
#include "ParticleEditTab.h"
#include "SelectionEditTab.h"
#include "SymbolEditTab.h"
#include "TokenEditTabWidget.h"
#include "TabWidgetHelper.h"

#include "DataEditModel.h"
#include "DataEditorView.h"


DataEditorView::DataEditorView(QWidget * parent)
	: QObject(parent)
{
	//main tabs
	_mainTabWidget = new QTabWidget(parent);
	TabWidgetHelper::setupTabWidget(_mainTabWidget, QSize(385, 260));

	_clusterTab = new ClusterEditTab(parent);
	TabWidgetHelper::setupTextEdit(_clusterTab);
	_clusterTab->setVisible(false);

	_cellTab = new CellEditTab(parent);
	TabWidgetHelper::setupTextEdit(_cellTab);
	_cellTab->setVisible(false);

	_metadataTab = new MetadataEditTab(parent);
	_metadataTab->setPalette(GuiSettings::getPaletteForTab());
	_metadataTab->setVisible(false);

	_particleTab = new ParticleEditTab(parent);
	TabWidgetHelper::setupTextEdit(_particleTab);
	_particleTab->setVisible(false);

	_selectionTab = new SelectionEditTab(parent);
	TabWidgetHelper::setupTextEdit(_selectionTab);
	_selectionTab->setVisible(false);

	//computer tabs
	_computerTabWidget = new QTabWidget(parent);
	TabWidgetHelper::setupTabWidget(_computerTabWidget, QSize(385, 341));

	_computerTab = new CellComputerEditTab(parent);
	_computerTab->setPalette(GuiSettings::getPaletteForTab());
	_computerTab->setVisible(false);

	//symbol tabs
	_symbolTabWidget = new QTabWidget(parent);
	TabWidgetHelper::setupTabWidget(_symbolTabWidget, QSize(385, 260));

	_symbolTab = new SymbolEditTab(parent);
	_symbolTab->setVisible(false);

	//token tabs
	_tokenTabWidget = new TokenEditTabWidget(parent);
	TabWidgetHelper::setupTabWidget(_tokenTabWidget, QSize(385, 260));

	updateDisplay();
}

void DataEditorView::init(IntVector2D const & upperLeftPosition, DataEditModel* model, DataEditController* controller, CellComputerCompiler* compiler)
{
	_model = model;
	_upperLeftPosition = upperLeftPosition;
	_mainTabWidget->setGeometry(upperLeftPosition.x, upperLeftPosition.y, _mainTabWidget->width(), _mainTabWidget->height());
	_computerTabWidget->setGeometry(upperLeftPosition.x, upperLeftPosition.y + _mainTabWidget->height() + 10, _computerTabWidget->width(), _computerTabWidget->height());
	_symbolTabWidget->setGeometry(upperLeftPosition.x + _mainTabWidget->width() + 10, upperLeftPosition.y, _symbolTabWidget->width(), _symbolTabWidget->height());
	_tokenTabWidget->setGeometry(_upperLeftPosition.x + _mainTabWidget->width() + 10 + _symbolTabWidget->width() + 10, _upperLeftPosition.y, _tokenTabWidget->width(), _tokenTabWidget->height());

	_clusterTab->init(_model, controller);
	_cellTab->init(_model, controller);
	_metadataTab->init(_model, controller);
	_computerTab->init(_model, controller, compiler);
	_particleTab->init(_model, controller);
	_selectionTab->init(_model, controller);
	_symbolTab->init(_model, controller);
	_tokenTabWidget->init(_model, controller);
}

void DataEditorView::updateDisplay() const
{
	if (!_visible || _editorSelector == EditorSelector::No) {
		_mainTabWidget->setVisible(false);
		_computerTabWidget->setVisible(false);
		_symbolTabWidget->setVisible(false);
		_tokenTabWidget->setVisible(false);
		return;
	}

	if (_editorSelector == EditorSelector::Selection) {
		_mainTabWidget->setVisible(true);
		_computerTabWidget->setVisible(false);
		_symbolTabWidget->setVisible(false);
		_tokenTabWidget->setVisible(false);

		_selectionTab->updateDisplay();
	}

	if (_editorSelector == EditorSelector::CellWithComputerWithToken) {
		_mainTabWidget->setVisible(true);
		_computerTabWidget->setVisible(true);
		_symbolTabWidget->setVisible(true);
		_tokenTabWidget->setVisible(true);
		_tokenTabWidget->setGeometry(_upperLeftPosition.x + _computerTabWidget->width() + 10 + _symbolTabWidget->width() + 10, _upperLeftPosition.y, _tokenTabWidget->width(), _tokenTabWidget->height());

		_clusterTab->updateDisplay();
		_cellTab->updateDisplay();
		_metadataTab->updateDisplay();
		_computerTab->updateDisplay();
		_symbolTab->updateDisplay();
		_tokenTabWidget->updateDisplay();
	}

	if (_editorSelector == EditorSelector::CellWithoutComputerWithToken) {
		_mainTabWidget->setVisible(true);
		_computerTabWidget->setVisible(false);
		_symbolTabWidget->setVisible(false);
		_tokenTabWidget->setVisible(true);
		_tokenTabWidget->setGeometry(_upperLeftPosition.x + _computerTabWidget->width() + 10, _upperLeftPosition.y, _tokenTabWidget->width(), _tokenTabWidget->height());

		_clusterTab->updateDisplay();
		_cellTab->updateDisplay();
		_metadataTab->updateDisplay();
		_tokenTabWidget->updateDisplay();
	}

	if (_editorSelector == EditorSelector::CellWithComputerWithoutToken) {
		_mainTabWidget->setVisible(true);
		_computerTabWidget->setVisible(true);
		_symbolTabWidget->setVisible(true);
		_tokenTabWidget->setVisible(false);

		_clusterTab->updateDisplay();
		_cellTab->updateDisplay();
		_metadataTab->updateDisplay();
		_computerTab->updateDisplay();
		_symbolTab->updateDisplay();
	}

	if (_editorSelector == EditorSelector::CellWithoutComputerWithoutToken) {
		_mainTabWidget->setVisible(true);
		_computerTabWidget->setVisible(false);
		_symbolTabWidget->setVisible(false);
		_tokenTabWidget->setVisible(false);

		_clusterTab->updateDisplay();
		_cellTab->updateDisplay();
		_metadataTab->updateDisplay();
	}

	if (_editorSelector == EditorSelector::Particle) {
		_mainTabWidget->setVisible(true);
		_computerTabWidget->setVisible(false);
		_symbolTabWidget->setVisible(false);
		_tokenTabWidget->setVisible(false);

		_particleTab->updateDisplay();
	}
}

void DataEditorView::saveTabPositionForCellEditor()
{
	if (_editorSelector == EditorSelector::CellWithComputerWithToken
		|| _editorSelector == EditorSelector::CellWithComputerWithoutToken
		|| _editorSelector == EditorSelector::CellWithoutComputerWithToken
		|| _editorSelector == EditorSelector::CellWithoutComputerWithoutToken) {
		_savedTabPosition = _mainTabWidget->currentIndex();
	}
}

int DataEditorView::getTabPositionForCellEditor()
{
	return _savedTabPosition;
}

void DataEditorView::switchToNoEditor()
{
	saveTabPositionForCellEditor();
	_editorSelector = EditorSelector::No;
	updateDisplay();
}

void DataEditorView::switchToCellEditorWithComputerWithToken()
{
	saveTabPositionForCellEditor();

	if (_editorSelector != EditorSelector::CellWithComputerWithToken) {
		_mainTabWidget->clear();
		_mainTabWidget->addTab(_clusterTab, "cluster");
		_mainTabWidget->addTab(_cellTab, "cell");
		_mainTabWidget->addTab(_metadataTab, "metadata");
		_mainTabWidget->setCurrentIndex(getTabPositionForCellEditor());
		_computerTabWidget->clear();
		_computerTabWidget->addTab(_computerTab, "cell computer");
		_symbolTabWidget->clear();
		_symbolTabWidget->addTab(_symbolTab, "symbols");
		_editorSelector = EditorSelector::CellWithComputerWithToken;
	}
	updateDisplay();
}

void DataEditorView::switchToCellEditorWithoutComputerWithToken()
{
	saveTabPositionForCellEditor();

	if (_editorSelector != EditorSelector::CellWithoutComputerWithToken) {
		_mainTabWidget->clear();
		_mainTabWidget->addTab(_clusterTab, "cluster");
		_mainTabWidget->addTab(_cellTab, "cell");
		_mainTabWidget->addTab(_metadataTab, "metadata");
		_mainTabWidget->setCurrentIndex(getTabPositionForCellEditor());
		_editorSelector = EditorSelector::CellWithoutComputerWithToken;
	}
	updateDisplay();
}

void DataEditorView::switchToCellEditorWithComputerWithoutToken()
{
	saveTabPositionForCellEditor();

	if (_editorSelector != EditorSelector::CellWithComputerWithoutToken) {
		_mainTabWidget->clear();
		_mainTabWidget->addTab(_clusterTab, "cluster");
		_mainTabWidget->addTab(_cellTab, "cell");
		_mainTabWidget->addTab(_metadataTab, "metadata");
		_mainTabWidget->setCurrentIndex(getTabPositionForCellEditor());
		_computerTabWidget->clear();
		_computerTabWidget->addTab(_computerTab, "cell computer");
		_symbolTabWidget->clear();
		_symbolTabWidget->addTab(_symbolTab, "symbols");
		_editorSelector = EditorSelector::CellWithComputerWithoutToken;
	}
	updateDisplay();
}

void DataEditorView::switchToCellEditorWithoutComputerWithoutToken()
{
	saveTabPositionForCellEditor();
	if (_editorSelector != EditorSelector::CellWithoutComputerWithoutToken) {
		_mainTabWidget->clear();
		_mainTabWidget->addTab(_clusterTab, "cluster");
		_mainTabWidget->addTab(_cellTab, "cell");
		_mainTabWidget->addTab(_metadataTab, "metadata");
		_mainTabWidget->setCurrentIndex(getTabPositionForCellEditor());
		_editorSelector = EditorSelector::CellWithoutComputerWithoutToken;
	}
	updateDisplay();
}

void DataEditorView::switchToParticleEditor()
{
	saveTabPositionForCellEditor();
	if (_editorSelector != EditorSelector::Particle) {
		_mainTabWidget->clear();
		_mainTabWidget->addTab(_particleTab, "particle");
		_editorSelector = EditorSelector::Particle;
	}
	updateDisplay();
}

void DataEditorView::switchToSelectionEditor()
{
	saveTabPositionForCellEditor();
	if (_editorSelector != EditorSelector::Selection) {
		_mainTabWidget->clear();
		_mainTabWidget->addTab(_selectionTab, "selection");
		_editorSelector = EditorSelector::Selection;
	}
	updateDisplay();
}

void DataEditorView::show(bool visible)
{
	_visible = visible;
	updateDisplay();
}
