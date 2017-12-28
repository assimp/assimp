/// \file   mainwindow.hpp
/// \brief  Main window and algorhytms.
/// \author smal.root@gmail.com
/// \date   2016

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

// Header files, Assimp.
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>

#ifndef __unused
	#define __unused	__attribute__((unused))
#endif // __unused

/**********************************/
/************ Functions ***********/
/**********************************/

/********************************************************************/
/********************* Import/Export functions **********************/
/********************************************************************/

void MainWindow::ImportFile(const QString &pFileName)
{
using namespace Assimp;

QTime time_begin = QTime::currentTime();

	if(mScene != nullptr)
	{
		mImporter.FreeScene();
		mGLView->FreeScene();
	}

	// Try to import scene.
	mScene = mImporter.ReadFile(pFileName.toStdString(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_ValidateDataStructure | \
															aiProcess_GenUVCoords | aiProcess_TransformUVCoords | aiProcess_FlipUVs);
	if(mScene != nullptr)
	{
		ui->lblLoadTime->setText(QString::number(time_begin.secsTo(QTime::currentTime())));
		LogInfo("Import done: " + pFileName);
		// Prepare widgets for new scene.
		ui->leFileName->setText(pFileName.right(pFileName.length() - pFileName.lastIndexOf('/') - 1));
		ui->lstLight->clear();
		ui->lstCamera->clear();
		ui->cbxLighting->setChecked(true);	mGLView->Lighting_Enable();
		ui->cbxBBox->setChecked(false);		mGLView->Enable_SceneBBox(false);
		ui->cbxTextures->setChecked(true);	mGLView->Enable_Textures(true);
		//
		// Fill info labels
		//
		// Cameras
		ui->lblCameraCount->setText(QString::number(mScene->mNumCameras));
		// Lights
		ui->lblLightCount->setText(QString::number(mScene->mNumLights));
		// Meshes, faces, vertices.
		size_t qty_face = 0;
		size_t qty_vert = 0;

		for(size_t idx_mesh = 0; idx_mesh < mScene->mNumMeshes; idx_mesh++)
		{
			qty_face += mScene->mMeshes[idx_mesh]->mNumFaces;
			qty_vert += mScene->mMeshes[idx_mesh]->mNumVertices;
		}

		ui->lblMeshCount->setText(QString::number(mScene->mNumMeshes));
		ui->lblFaceCount->setText(QString::number(qty_face));
		ui->lblVertexCount->setText(QString::number(qty_vert));
		// Animation
		if(mScene->mNumAnimations)
			ui->lblHasAnimation->setText("yes");
		else
			ui->lblHasAnimation->setText("no");

		//
		// Set scene for GL viewer.
		//
		mGLView->SetScene(mScene, pFileName);
		// Select first camera
		ui->lstCamera->setCurrentRow(0);
		mGLView->Camera_Set(0);
		// Scene is loaded, do first rendering.
		LogInfo("Scene is ready for rendering.");
		mGLView->updateGL();
	}
	else
	{
		ResetSceneInfos();

		QString errorMessage = QString("Error parsing \'%1\' : \'%2\'").arg(pFileName).arg(mImporter.GetErrorString());
		QMessageBox::critical(this, "Import error", errorMessage);
		LogError(errorMessage);
	}// if(mScene != nullptr)
}

void MainWindow::ResetSceneInfos()
{
	ui->lblLoadTime->clear();
	ui->leFileName->clear();
	ui->lblMeshCount->setText("0");
	ui->lblFaceCount->setText("0");
	ui->lblVertexCount->setText("0");
	ui->lblCameraCount->setText("0");
	ui->lblLightCount->setText("0");
	ui->lblHasAnimation->setText("no");
}

/********************************************************************/
/************************ Logging functions *************************/
/********************************************************************/

void MainWindow::LogInfo(const QString& pMessage)
{
	Assimp::DefaultLogger::get()->info(pMessage.toStdString());
}

void MainWindow::LogError(const QString& pMessage)
{
	Assimp::DefaultLogger::get()->error(pMessage.toStdString());
}

/********************************************************************/
/*********************** Override functions ************************/
/********************************************************************/

void MainWindow::mousePressEvent(QMouseEvent* pEvent)
{
	if(pEvent->button() & Qt::LeftButton)
		mPosition_Pressed_LMB = pEvent->pos();
	else if(pEvent->button() & Qt::RightButton)
		mPosition_Pressed_RMB = pEvent->pos();
}

void MainWindow::mouseMoveEvent(QMouseEvent* pEvent)
{
	if(pEvent->buttons() & Qt::LeftButton)
	{
		GLfloat dx = 180 * GLfloat(pEvent->x() - mPosition_Pressed_LMB.x()) / mGLView->width();
		GLfloat dy = 180 * GLfloat(pEvent->y() - mPosition_Pressed_LMB.y()) / mGLView->height();

		if(pEvent->modifiers() & Qt::ShiftModifier)
			mGLView->Camera_RotateScene(dy, 0, dx);// Rotate around oX and oZ axises.
		else
			mGLView->Camera_RotateScene(dy, dx, 0);// Rotate around oX and oY axises.

		mGLView->updateGL();
		mPosition_Pressed_LMB = pEvent->pos();
	}

	if(pEvent->buttons() & Qt::RightButton)
	{
		GLfloat dx = 180 * GLfloat(pEvent->x() - mPosition_Pressed_RMB.x()) / mGLView->width();
		GLfloat dy = 180 * GLfloat(pEvent->y() - mPosition_Pressed_RMB.y()) / mGLView->height();

		if(pEvent->modifiers() & Qt::ShiftModifier)
			mGLView->Camera_Rotate(dy, 0, dx);// Rotate around oX and oZ axises.
		else
			mGLView->Camera_Rotate(dy, dx, 0);// Rotate around oX and oY axises.

		mGLView->updateGL();
		mPosition_Pressed_RMB = pEvent->pos();
	}
}

void MainWindow::keyPressEvent(QKeyEvent* pEvent)
{
GLfloat step;

	if(pEvent->modifiers() & Qt::ControlModifier)
		step = 10;
	else if(pEvent->modifiers() & Qt::AltModifier)
		step = 100;
	else
		step = 1;

	if(pEvent->key() == Qt::Key_A)
		mGLView->Camera_Translate(-step, 0, 0);
	else if(pEvent->key() == Qt::Key_D)
		mGLView->Camera_Translate(step, 0, 0);
	else if(pEvent->key() == Qt::Key_W)
		mGLView->Camera_Translate(0, step, 0);
	else if(pEvent->key() == Qt::Key_S)
		mGLView->Camera_Translate(0, -step, 0);
	else if(pEvent->key() == Qt::Key_Up)
		mGLView->Camera_Translate(0, 0, -step);
	else if(pEvent->key() == Qt::Key_Down)
		mGLView->Camera_Translate(0, 0, step);

	mGLView->updateGL();
}

/********************************************************************/
/********************** Constructor/Destructor **********************/
/********************************************************************/

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow),
		mScene(nullptr)
{
using namespace Assimp;

	ui->setupUi(this);
	// Create OpenGL widget
	mGLView = new CGLView(this);
	mGLView->setMinimumSize(800, 600);
	mGLView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	mGLView->setFocusPolicy(Qt::StrongFocus);
	// Connect to GLView signals.
	connect(mGLView, SIGNAL(Paint_Finished(size_t, GLfloat)), SLOT(Paint_Finished(size_t, GLfloat)));
	connect(mGLView, SIGNAL(SceneObject_Camera(QString)), SLOT(SceneObject_Camera(QString)));
	connect(mGLView, SIGNAL(SceneObject_LightSource(QString)), SLOT(SceneObject_LightSource(QString)));
	// and add it to layout
	ui->hlMainView->insertWidget(0, mGLView, 4);
	// Create logger
	mLoggerView = new CLoggerView(ui->tbLog);
	DefaultLogger::create("", Logger::VERBOSE);
	DefaultLogger::get()->attachStream(mLoggerView, DefaultLogger::Debugging | DefaultLogger::Info | DefaultLogger::Err | DefaultLogger::Warn);

	ResetSceneInfos();
}

MainWindow::~MainWindow()
{
using namespace Assimp;

	DefaultLogger::get()->detatchStream(mLoggerView, DefaultLogger::Debugging | DefaultLogger::Info | DefaultLogger::Err | DefaultLogger::Warn);
	DefaultLogger::kill();

	if(mScene != nullptr) mImporter.FreeScene();
	if(mLoggerView != nullptr) delete mLoggerView;
	if(mGLView != nullptr) delete mGLView;
	delete ui;
}

/********************************************************************/
/****************************** Slots *******************************/
/********************************************************************/

void MainWindow::Paint_Finished(const size_t pPaintTime_ms, const GLfloat pDistance)
{
	ui->lblRenderTime->setText(QString::number(pPaintTime_ms));
	ui->lblDistance->setText(QString::number(pDistance));
}

void MainWindow::SceneObject_Camera(const QString& pName)
{
	ui->lstCamera->addItem(pName);
}

void MainWindow::SceneObject_LightSource(const QString& pName)
{
	ui->lstLight->addItem(pName);
	// After item added "currentRow" is still contain old value (even '-1' if first item added). Because "currentRow"/"currentItem" is changed by user interaction,
	// not by "addItem". So, "currentRow" must be set manually.
	ui->lstLight->setCurrentRow(ui->lstLight->count() - 1);
	// And after "selectAll" handler of "signal itemSelectionChanged" will get right "currentItem" and "currentRow" values.
	ui->lstLight->selectAll();
}

void MainWindow::on_butOpenFile_clicked()
{
aiString filter_temp;
QString filename, filter;

	mImporter.GetExtensionList(filter_temp);
	filter = filter_temp.C_Str();
	filter.replace(';', ' ');
	filter.append(" ;; All (*.*)");
	filename = QFileDialog::getOpenFileName(this, "Choose the file", "", filter);

	if(!filename.isEmpty()) ImportFile(filename);
}


void MainWindow::on_butExport_clicked()
{
using namespace Assimp;

QString filename, filter, format_id;
Exporter exporter;
QTime time_begin;
aiReturn rv;
QStringList exportersList;
QMap<QString, const aiExportFormatDesc*> exportersMap;


	if(mScene == nullptr)
	{
		QMessageBox::critical(this, "Export error", "Scene is empty");

		return;
	}

	for (int i = 0; i < exporter.GetExportFormatCount(); ++i)
	{
		const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
		exportersList.push_back(desc->id + QString(": ") + desc->description);
		exportersMap.insert(desc->id, desc);
	}

	// get an exporter
	bool dialogSelectExporterOk;
	QString selectedExporter = QInputDialog::getItem(this, "Export format", "Select the exporter : ", exportersList, 0, false, &dialogSelectExporterOk);
	if (!dialogSelectExporterOk)
		return;

	// build the filter
	QString selectedId = selectedExporter.left(selectedExporter.indexOf(':'));
	filter = QString("*.") + exportersMap[selectedId]->fileExtension;

	// get file path
	filename = QFileDialog::getSaveFileName(this, "Set file name", "", filter);
	// if it's canceled
	if (filename == "")
		return;

	// begin export
	time_begin = QTime::currentTime();
	rv = exporter.Export(mScene, selectedId.toLocal8Bit(), filename.toLocal8Bit(), aiProcess_FlipUVs);
	ui->lblExportTime->setText(QString::number(time_begin.secsTo(QTime::currentTime())));
	if(rv == aiReturn_SUCCESS)
		LogInfo("Export done: " + filename);
	else
	{
		QString errorMessage = QString("Export failed: ") + filename;
		LogError(errorMessage);
		QMessageBox::critical(this, "Export error", errorMessage);
	}
}

void MainWindow::on_cbxLighting_clicked(bool pChecked)
{
	if(pChecked)
		mGLView->Lighting_Enable();
	else
		mGLView->Lighting_Disable();

	mGLView->updateGL();
}

void MainWindow::on_lstLight_itemSelectionChanged()
{
bool selected = ui->lstLight->isItemSelected(ui->lstLight->currentItem());

	if(selected)
		mGLView->Lighting_EnableSource(ui->lstLight->currentRow());
	else
		mGLView->Lighting_DisableSource(ui->lstLight->currentRow());

	mGLView->updateGL();
}

void MainWindow::on_lstCamera_clicked( const QModelIndex &)
{
	mGLView->Camera_Set(ui->lstLight->currentRow());
	mGLView->updateGL();
}

void MainWindow::on_cbxBBox_clicked(bool checked)
{
	mGLView->Enable_SceneBBox(checked);
	mGLView->updateGL();
}

void MainWindow::on_cbxTextures_clicked(bool checked)
{
	mGLView->Enable_Textures(checked);
	mGLView->updateGL();
}
