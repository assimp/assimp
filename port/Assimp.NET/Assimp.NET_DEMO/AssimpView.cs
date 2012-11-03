/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

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

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

namespace Assimp.Viewer
{
    public partial class AssimpView : Form
    {
        public AssimpView()
        {
            InitializeComponent();

            // Window title
            this.Text = "Assimp .NET Viewer";

            // Ignore WM_ERASEBKGND messages to prevent flicker
            this.SetStyle(ControlStyles.AllPaintingInWmPaint, true);

            // Listen to mouse
            this.MouseDown += new MouseEventHandler(AssimpView_MouseDown);
            this.MouseMove += new MouseEventHandler(AssimpView_MouseMove);
            this.MouseUp += new MouseEventHandler(AssimpView_MouseUp);
            this.MouseWheel += new MouseEventHandler(AssimpView_MouseWheel);
        }

        void AssimpView_MouseWheel(object sender, MouseEventArgs e) {
			g_sCamera.vPos.Z *= 1.0f - (e.Delta / 1000.0f);
            Refresh();
        }

        private void AssimpView_MouseDown(object sender, MouseEventArgs e) {
            UpdateMouseState(e);
        }

        private void AssimpView_MouseMove(object sender, MouseEventArgs e) {
            UpdateMouseState(e);
        }

        private void AssimpView_MouseUp(object sender, MouseEventArgs e) {
            UpdateMouseState(e);
        }

        private void UpdateMouseState(MouseEventArgs e) {
            g_bMousePressed = (e.Button == MouseButtons.Left);
            g_bMousePressedR = (e.Button == MouseButtons.Right);
            g_bMousePressedM = (e.Button == MouseButtons.Middle);
            g_bMousePressedBoth = (e.Button == (MouseButtons.Left | MouseButtons.Right));
            Refresh();
        }

        protected override void OnPaintBackground(PaintEventArgs pevent) {
            /* Do nothing to prevent flicker during resize */
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            InitializeDevice();
            InitializeAssimp();
        }

        private void Form1_SizeChanged(object sender, EventArgs e) {
            presentParams.BackBufferWidth = Width;
            presentParams.BackBufferHeight = Height;
            device.Reset(presentParams);
            Invalidate();
        }

        public void InitializeDevice()
        {
            // Improve Performance
            // http://blogs.msdn.com/b/tmiller/archive/2003/11/14/57531.aspx
            Device.IsUsingEventHandlers = false;

            // For Windowed mode leave Width and Height at 0
            var adapter = Manager.Adapters.Default;
            presentParams = new PresentParameters();
            presentParams.AutoDepthStencilFormat = DepthFormat.D16;
            presentParams.EnableAutoDepthStencil = true;
            presentParams.Windowed = true;
            presentParams.SwapEffect = SwapEffect.Discard;

            // Keep precision in floating point calculations
            // http://blogs.msdn.com/b/tmiller/archive/2004/06/01/145596.aspx
            var createFlags = CreateFlags.FpuPreserve;

            // Pure device for performance - all device render states are now write only
            var deviceType = DeviceType.Hardware;
            var caps = Manager.GetDeviceCaps(adapter.Adapter, deviceType);
            if (caps.DeviceCaps.SupportsPureDevice) {
                createFlags |= CreateFlags.PureDevice;
            }

            // Warning: some drivers lie about supporting HT&L. Use Software if you see problems.
            if (caps.DeviceCaps.SupportsHardwareTransformAndLight) {
                createFlags |= CreateFlags.HardwareVertexProcessing;
            }
            else {
                createFlags |= CreateFlags.SoftwareVertexProcessing;
            }

            // Create Device
            device = new Device(adapter.Adapter, deviceType, this, createFlags, presentParams);

            // Add event handlers
            device.DeviceResizing += new CancelEventHandler(device_DeviceResizing);
        }

        private void device_DeviceResizing(object sender, CancelEventArgs e) {
            device.Reset();
        }

        // default pp steps
        private static aiPostProcessSteps ppsteps =
            aiPostProcessSteps.aiProcess_CalcTangentSpace | // calculate tangents and bitangents if possible
            aiPostProcessSteps.aiProcess_JoinIdenticalVertices | // join identical vertices/ optimize indexing
            aiPostProcessSteps.aiProcess_ValidateDataStructure | // perform a full validation of the loader's output
            aiPostProcessSteps.aiProcess_ImproveCacheLocality | // improve the cache locality of the output vertices
            aiPostProcessSteps.aiProcess_RemoveRedundantMaterials | // remove redundant materials
            aiPostProcessSteps.aiProcess_FindDegenerates | // remove degenerated polygons from the import
            aiPostProcessSteps.aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
            aiPostProcessSteps.aiProcess_GenUVCoords | // convert spherical, cylindrical, box and planar mapping to proper UVs
            aiPostProcessSteps.aiProcess_TransformUVCoords | // preprocess UV transformations (scaling, translation ...)
            aiPostProcessSteps.aiProcess_FindInstances | // search for instanced meshes and remove them by references to one master
            aiPostProcessSteps.aiProcess_LimitBoneWeights | // limit bone weights to 4 per vertex
            aiPostProcessSteps.aiProcess_OptimizeMeshes | // join small meshes, if possible;
		    (aiPostProcessSteps)0;


        public void InitializeAssimp() {
            var flags = ( ppsteps |
                aiPostProcessSteps.aiProcess_GenSmoothNormals | // generate smooth normal vectors if not existing
                aiPostProcessSteps.aiProcess_SplitLargeMeshes | // split large, unrenderable meshes into submeshes
                aiPostProcessSteps.aiProcess_Triangulate | // triangulate polygons with more than 3 edges
                aiPostProcessSteps.aiProcess_ConvertToLeftHanded | // convert everything to D3D left handed space
                aiPostProcessSteps.aiProcess_SortByPType | // make 'clean' meshes which consist of a single typ of primitives
                (aiPostProcessSteps)0);

            // default model
            var path = "../../../../../test/models/3DS/test1.3ds";

            importer = new Importer();
            string[] args = Environment.GetCommandLineArgs();
            if (args.Length > 1) {
                path = args[1];
            }

            
            //var path = "man.3ds";
            scene = importer.ReadFile(path, flags);
            if (scene != null)
            {
                directory = Path.GetDirectoryName(path);
                CacheMaterials(scene.mMaterials);
                CacheMeshes(scene.mMeshes);
                SetupCamera(scene.mCameras);
            }
            else {
                MessageBox.Show("Failed to open file: " + path + ". Either Assimp screwed up or the path is not valid.");
                Application.Exit();
            }
        }

        private void CacheMeshes(aiMeshVector meshes) {
            var numMeshes = meshes.Count;
            meshCache = new Mesh[numMeshes];
            meshBoundsMin = new Vector3[numMeshes];
            meshBoundsMax = new Vector3[numMeshes];

            for (int i = 0; i < numMeshes; ++i) {
                var mesh = meshes[i];

                switch (mesh.mPrimitiveTypes) {
                case aiPrimitiveType.aiPrimitiveType_TRIANGLE:
                    Vector3 min, max;
                    meshCache[i] = CreateMesh(mesh, out min, out max);
                    meshBoundsMin[i] = min;
                    meshBoundsMax[i] = max;
                    break;
                }
            }
        }

        private void CacheMaterials(aiMaterialVector materials) {
            var numMaterials = materials.Count;
            materialCache = new ExtendedMaterial[numMaterials];
            textureCache = new Dictionary<string, Texture>();

            for (int i = 0; i < numMaterials; ++i) {
                var material = materials[i];
                var dxExtendedMaterial = new ExtendedMaterial();
                var dxMaterial = new Material();
                dxMaterial.AmbientColor = material.Ambient.ToColorValue();
                dxMaterial.DiffuseColor = material.Diffuse.ToColorValue();
                dxMaterial.EmissiveColor = material.Emissive.ToColorValue();
                dxMaterial.SpecularColor = material.Specular.ToColorValue();
                dxMaterial.SpecularSharpness = material.ShininessStrength;
                dxExtendedMaterial.Material3D = dxMaterial;
                dxExtendedMaterial.TextureFilename = material.TextureDiffuse0;

                materialCache[i] = dxExtendedMaterial;

                var textureFilename = dxExtendedMaterial.TextureFilename;
                if (!string.IsNullOrEmpty(textureFilename) && !textureCache.ContainsKey(textureFilename)) {
                    textureCache.Add(textureFilename, CreateTexture(textureFilename));
                }
            }
        }

        private Texture CreateTexture(string fileName) {
            var path = Path.Combine(directory, fileName);
            try {
                using (var data = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read)) {
                    var dxTexture = Texture.FromStream(device, data, Usage.None, Pool.Managed);
                    textureCache[path] = dxTexture;
                    return dxTexture;
                }
            }
            catch (Exception) {
                return null;
            }
        }

        private Mesh CreateMesh(aiMesh aiMesh, out Vector3 min, out Vector3 max) {
            var numFaces = (int) aiMesh.mNumFaces;
            var numVertices = (int) aiMesh.mNumVertices;

            var dxMesh = new Mesh(numFaces, numVertices, MeshFlags.Managed | MeshFlags.Use32Bit,
                CustomVertex.PositionNormalTextured.Format, device);

            var aiPositions = aiMesh.mVertices;
            var aiNormals = aiMesh.mNormals;
            var aiTextureCoordsAll = aiMesh.mTextureCoords;
            var aiTextureCoords = (aiTextureCoordsAll!=null) ? aiTextureCoordsAll[0] : null;
            var dxVertices = new CustomVertex.PositionNormalTextured[numVertices];
            for (int i = 0; i < numVertices; ++i) {
                dxVertices[i].Position = aiPositions[i].ToVector3();
                if (aiNormals != null) {
                    dxVertices[i].Normal = aiNormals[i].ToVector3();
                }
                if (aiTextureCoords != null) {
                    var uv = aiTextureCoords[i];
                    dxVertices[i].Tu = uv.x;
                    dxVertices[i].Tv = uv.y;
                }
            }
            dxMesh.VertexBuffer.SetData(dxVertices, 0, LockFlags.None);

            var aiFaces = aiMesh.mFaces;
            var dxIndices = new uint[numFaces * 3];
            for (int i = 0; i < numFaces; ++i) {
                var aiFace = aiFaces[i];
                var aiIndices = aiFace.mIndices;
                for (int j = 0; j < 3; ++j) {
                    dxIndices[i * 3 + j] = aiIndices[j];
                }
            }
            dxMesh.IndexBuffer.SetData(dxIndices, 0, LockFlags.None);

            var dxAttributes = dxMesh.LockAttributeBufferArray(LockFlags.None);
            // TODO: Set face material index for attributes
            dxMesh.UnlockAttributeBuffer(dxAttributes);

            var adjacency = new int[numFaces * 3];
            dxMesh.GenerateAdjacency(0.0f, adjacency);
            dxMesh.OptimizeInPlace(MeshFlags.OptimizeAttributeSort, adjacency);

            Geometry.ComputeBoundingBox(dxVertices, CustomVertex.PositionNormalTextured.StrideSize, out min, out max);

            return dxMesh;
        }

        //-------------------------------------------------------------------------------
        // Calculate the boundaries of a given node and all of its children
        // The boundaries are in Worldspace (AABB)
        // piNode Input node
        // min/max Receives the min/max boundaries
        // piMatrix Transformation matrix of the graph at this position
        //-------------------------------------------------------------------------------
        private void CalculateBounds(aiNode piNode, ref Vector3 min, ref Vector3 max, Matrix piMatrix) {
            Debug.Assert(null != piNode);

            var mTemp = piNode.mTransformation.ToMatrix();
            var aiMe = mTemp * piMatrix;

            var meshes = piNode.mMeshes;
            var numMeshes = meshes.Count;
            for (int i = 0; i < numMeshes; ++i) {
                var mesh = meshes[i];
                var meshMin = Vector3.TransformCoordinate(meshBoundsMin[mesh], aiMe);
                var meshMax = Vector3.TransformCoordinate(meshBoundsMax[mesh], aiMe);
                min.X = Math.Min(min.X, meshMin.X);
                min.Y = Math.Min(min.Y, meshMin.Y);
                min.Z = Math.Min(min.Z, meshMin.Z);
                max.X = Math.Max(max.X, meshMax.X);
                max.Y = Math.Max(max.Y, meshMax.Y);
                max.Z = Math.Max(max.Z, meshMax.Z);
            }
            var children = piNode.mChildren;
            var numChildren = children.Count;
            for (int i = 0; i < numChildren; ++i) {
                CalculateBounds(children[i], ref min, ref max, aiMe);
            }
        }

        protected override void OnPaint(System.Windows.Forms.PaintEventArgs e) {
            HandleMouseInputLocal();
            Render();
        }

        private void Render() {
            device.Clear(ClearFlags.Target|ClearFlags.ZBuffer, Color.DarkSlateBlue, 1.0f, 0);
            device.BeginScene();

            SetupMatrices();

            if (scene != null && scene.mRootNode != null) {
                SetupLights(scene.mLights);
                RenderNode(scene.mRootNode, g_mWorldRotate);
            }

            device.EndScene();
            device.Present();
        }

        private void SetupMatrices() {
            var fovy = Geometry.DegreeToRadian(45.0f);
            var aspect = ((float)this.Width / (float)this.Height);
            device.Transform.Projection = Matrix.PerspectiveFovLH(fovy, aspect, g_zNear, g_zFar);
            device.Transform.View = g_sCamera.GetMatrix();
        }

        private void SetupCamera(aiCameraVector cameras) {
            // Get scene bounds
            var min = new Vector3(float.MaxValue, float.MaxValue, float.MaxValue);
            var max = new Vector3(float.MinValue, float.MinValue, float.MinValue);
            CalculateBounds(scene.mRootNode, ref min, ref max, Matrix.Identity);

            // Projection Depth
            var size = (max-min);
            g_zFar = Math.Max(Math.Max(size.X, size.Y), size.Z) * 100;

            // Starting View Position
            var center = min + size * 0.5f;
            var lookAt = new Vector3(0,0,1); // Unit vector not coordinate
            var position = new Vector3(center.X, center.Y, center.Z-(size.Z*10));
            var up = new Vector3(0, 1, 0);
            if (cameras.Count > 0) {
                var camera = cameras[0];
                lookAt = camera.mLookAt.ToVector3();
                position = camera.mPosition.ToVector3();
                up = camera.mUp.ToVector3();
            }
            g_sCamera.vLookAt = lookAt;
            g_sCamera.vPos = position;
            g_sCamera.vUp = up;
        }

        private void SetupLights(aiLightVector lights) {
            var numLights = lights.Count;

            if (numLights == 0) {
                var light = device.Lights[0];
                light.Type = LightType.Directional;
                light.Diffuse = Color.LightGray;
                light.Direction = new Vector3(-1, -1, 1);
                light.Enabled = true;
            }
            else {
                // TODO: setup lights
            }

            // Enables lighting
            device.RenderState.Lighting = true;
            device.RenderState.Ambient = Color.DarkGray;
            device.RenderState.SpecularEnable = true;
        }

        private void RenderNode(aiNode node, Matrix parentMatrix) {
            var nodeMatrix = node.mTransformation.ToMatrix() * parentMatrix;

            var children = node.mChildren;
            var numChildren = node.mNumChildren;
            for (int i = 0; i < numChildren; ++i) {
                var child = children[i];
                RenderNode(child, nodeMatrix);
            }

            device.Transform.World = nodeMatrix;
            var meshes = node.mMeshes;
            var numMeshes = meshes.Count;
            for (int i = 0; i < numMeshes; ++i) {
                var meshId = (int)meshes[i];
                var mesh = scene.mMeshes[meshId];
                var materialId = (int)mesh.mMaterialIndex;

                var dxMaterial = materialCache[materialId];
                device.Material = dxMaterial.Material3D;

                Texture dxTexture = null;
                if (!string.IsNullOrEmpty(dxMaterial.TextureFilename)) {
                    textureCache.TryGetValue(dxMaterial.TextureFilename, out dxTexture);
                }
                device.SetTexture(0, dxTexture);

                var dxMesh = meshCache[meshId];
                RenderMesh(dxMesh);
            }
        }

        private void RenderMesh(Mesh mesh) {
            for (int i = 0; i < mesh.NumberAttributes; ++i) {
                mesh.DrawSubset(i);
            }
        }

        private Device device;
        private PresentParameters presentParams;

        private Importer importer;
        private string directory;
        private aiScene scene;
        private Mesh[] meshCache;
        private ExtendedMaterial[] materialCache;
        private Dictionary<string, Texture> textureCache;
        private Vector3[] meshBoundsMin;
        private Vector3[] meshBoundsMax;

        private Camera g_sCamera = new Camera();
        private float g_zNear = 0.2f;
        private float g_zFar = 1000.0f;
        private Matrix g_mProjection = Matrix.Identity;
        private Matrix g_mWorldRotate = Matrix.Identity;
    }

    //-------------------------------------------------------------------------------
    // Position of the cursor relative to the 3ds max' like control circle
    //-------------------------------------------------------------------------------
    public enum EClickPos {
        // The click was inside the inner circle (x,y axis)
        Circle,
        // The click was inside one of tghe vertical snap-ins
        CircleVert,
        // The click was inside onf of the horizontal snap-ins
        CircleHor,
        // the cklick was outside the circle (z-axis)
        Outside
    };
}
