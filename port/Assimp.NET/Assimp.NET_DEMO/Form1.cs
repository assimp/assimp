using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Microsoft.DirectX.Direct3D;

namespace Assimp.NET_DEMO
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            InitializeDevice();
            InitializeAssimp();
        }

        public void InitializeDevice()
        {
            PresentParameters presentParams = new PresentParameters();
            presentParams.Windowed = true;
            presentParams.SwapEffect = SwapEffect.Discard;

            device = new Device(0, DeviceType.Hardware, this, CreateFlags.SoftwareVertexProcessing, presentParams);
        }

        public void InitializeAssimp()
        {
            
            importer = new Importer();
            try
            {
                //aiScene human = importer.ReadFile_s("fff.obj", 0);
                //importer.SetExtraVerbose(true);
                String s = "fff.obj";
                importer.ReadFile(s, 0);
            }
            catch (Exception)
            {
                
                throw;
            }

            return;
        }

        protected override void OnPaint(System.Windows.Forms.PaintEventArgs e)
        {
            device.Clear(ClearFlags.Target, Color.DarkSlateBlue, 1.0f, 0);
            device.Present();
        }

        private Device device;
        private Importer importer;
    }
}
