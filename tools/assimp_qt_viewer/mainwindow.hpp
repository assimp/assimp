/// \file   mainwindow.hpp
/// \brief  Main window and algorhytms.
/// \author smal.root@gmail.com
/// \date   2016

#pragma once

// Header files, Qt.
#if defined ASSIMP_QT4_VIEWER
#	include <QMainWindow>
#else
#	include <QtWidgets>
#endif

// Header files, project.
#include "glview.hpp"
#include "loggerview.hpp"

// Header files, Assimp.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

namespace Ui { class MainWindow; }

/// \class MainWindow
/// Main window and algorhytms.
class MainWindow : public QMainWindow
{
	Q_OBJECT

	/**********************************/
	/************ Variables ***********/
	/**********************************/

private:

	Ui::MainWindow *ui;

	CGLView* mGLView;///< Pointer to OpenGL render.
	CLoggerView* mLoggerView;///< Pointer to logging object.
	Assimp::Importer mImporter;///< Assimp importer.
	const aiScene* mScene;///< Pointer to loaded scene (\ref aiScene).

	/// \struct SMouse_Transformation
	/// Holds data about transformation of the scene/camera when mouse us used.
	struct SMouse_Transformation
	{
		bool Position_Pressed_Valid;///< Mouse button pressed on GLView.
		QPoint Position_Pressed_LMB;///< Position where was pressed left mouse button.
		QPoint Position_Pressed_RMB;///< Position where was pressed right mouse button.
		aiMatrix4x4 Rotation_AroundCamera;///< Rotation matrix which set rotation angles of the scene around camera.
		aiMatrix4x4 Rotation_Scene;///< Rotation matrix which set rotation angles of the scene around own center.
	} mMouse_Transformation;

	/**********************************/
	/************ Functions ***********/
	/**********************************/

	/********************************************************************/
	/********************* Import/Export functions **********************/
	/********************************************************************/

	/// \fn void ImportFile(const QString& pFileName)
	/// Import scene from file.
	/// \param [in] pFileName - path and name of the file.
	void ImportFile(const QString& pFileName);


	/// \fn void ResetSceneInfos()
	/// Reset informations about the scene
	void ResetSceneInfos();

	/********************************************************************/
	/************************ Logging functions *************************/
	/********************************************************************/

	/// \fn void LogInfo(const QString& pMessage)
	/// Add message with severity "Warning" to log.
	void LogInfo(const QString& pMessage);

	/// \fn void LogError(const QString& pMessage)
	/// Add message with severity "Error" to log.
	void LogError(const QString& pMessage);

	/********************************************************************/
	/*********************** Override functions ************************/
	/********************************************************************/

protected:

	/// \fn void mousePressEvent(QMouseEvent* pEvent) override
	/// Override function which handles mouse event "button pressed".
	/// \param [in] pEvent - pointer to event data.
	void mousePressEvent(QMouseEvent* pEvent) override;

	/// \fn void mouseReleaseEvent(QMouseEvent *pEvent) override
	/// Override function which handles mouse event "button released".
	/// \param [in] pEvent - pointer to event data.
	void mouseReleaseEvent(QMouseEvent *pEvent) override;

	/// \fn void mouseMoveEvent(QMouseEvent* pEvent) override
	/// Override function which handles mouse event "move".
	/// \param [in] pEvent - pointer to event data.
	void mouseMoveEvent(QMouseEvent* pEvent) override;

	/// \fn void keyPressEvent(QKeyEvent* pEvent) override
	/// Override function which handles key event "key pressed".
	/// \param [in] pEvent - pointer to event data.
	void keyPressEvent(QKeyEvent* pEvent) override;

public:

	/********************************************************************/
	/********************** Constructor/Destructor **********************/
	/********************************************************************/

	/// \fn explicit MainWindow(QWidget* pParent = 0)
	/// \param [in] pParent - pointer to parent widget.
	explicit MainWindow(QWidget* pParent = 0);

	/// \fn ~MainWindow()
	/// Destructor.
	~MainWindow();

	/********************************************************************/
	/****************************** Slots *******************************/
	/********************************************************************/

private slots:

	/// \fn void Paint_Finished(const int pPaintTime)
	/// Show paint/render time and distance between camera and center of the scene.
	/// \param [in] pPaintTime_ms - paint time in milliseconds.
	void Paint_Finished(const size_t pPaintTime_ms, const GLfloat pDistance);

	/// \fn void SceneObject_Camera(const QString& pName)
	/// Add camera name to list.
	/// \param [in] pName - name of the camera.
	void SceneObject_Camera(const QString& pName);

	/// \fn void SceneObject_LightSource(const QString& pName)
	/// Add lighting source name to list.
	/// \param [in] pName - name of the light source,
	void SceneObject_LightSource(const QString& pName);

	void on_butOpenFile_clicked();
	void on_butExport_clicked();
	void on_cbxLighting_clicked(bool pChecked);
	void on_lstLight_itemSelectionChanged();
	void on_lstCamera_clicked(const QModelIndex &index);
	void on_cbxBBox_clicked(bool checked);
	void on_cbxTextures_clicked(bool checked);
	void on_cbxDrawAxes_clicked(bool checked);
};
