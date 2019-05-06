/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team



All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

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

namespace Ui { 
    class MainWindow; 
}

/// \class MainWindow
/// Main window and algorithms.
class MainWindow : public QMainWindow {
    Q_OBJECT

    struct SMouse_Transformation;

public:
    /// @brief  The class constructor.
    /// \param [in] pParent - pointer to parent widget.
    explicit MainWindow( QWidget* pParent = 0 );

    /// @brief  The class destructor.
    ~MainWindow();
    
    /// Import scene from file.
	/// \param [in] pFileName - path and name of the file.
	void ImportFile(const QString& pFileName);

	/// Reset informations about the scene
	void ResetSceneInfos();

	/// Add message with severity "Warning" to log.
	void LogInfo(const QString& pMessage);

	/// Add message with severity "Error" to log.
	void LogError(const QString& pMessage);

protected:
	/// Override function which handles mouse event "button pressed".
	/// \param [in] pEvent - pointer to event data.
	void mousePressEvent(QMouseEvent* pEvent) override;

	/// Override function which handles mouse event "button released".
	/// \param [in] pEvent - pointer to event data.
	void mouseReleaseEvent(QMouseEvent *pEvent) override;

	/// Override function which handles mouse event "move".
	/// \param [in] pEvent - pointer to event data.
	void mouseMoveEvent(QMouseEvent* pEvent) override;

	/// Override function which handles key event "key pressed".
	/// \param [in] pEvent - pointer to event data.
	void keyPressEvent(QKeyEvent* pEvent) override;

private slots:
	/// Show paint/render time and distance between camera and center of the scene.
	/// \param [in] pPaintTime_ms - paint time in milliseconds.
	void Paint_Finished(const size_t pPaintTime_ms, const GLfloat pDistance);

	/// Add camera name to list.
	/// \param [in] pName - name of the camera.
	void SceneObject_Camera(const QString& pName);

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

private:
    Ui::MainWindow *ui;
    CGLView *mGLView;///< Pointer to OpenGL render.
    CLoggerView *mLoggerView;///< Pointer to logging object.
    Assimp::Importer mImporter;///< Assimp importer.
    const aiScene* mScene;///< Pointer to loaded scene (\ref aiScene).

    /// \struct SMouse_Transformation
    /// Holds data about transformation of the scene/camera when mouse us used.
    struct SMouse_Transformation {
        bool Position_Pressed_Valid;///< Mouse button pressed on GLView.
        QPoint Position_Pressed_LMB;///< Position where was pressed left mouse button.
        QPoint Position_Pressed_RMB;///< Position where was pressed right mouse button.
        aiMatrix4x4 Rotation_AroundCamera;///< Rotation matrix which set rotation angles of the scene around camera.
        aiMatrix4x4 Rotation_Scene;///< Rotation matrix which set rotation angles of the scene around own center.
    } mMouse_Transformation;
};
