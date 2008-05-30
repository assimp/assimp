/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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


package assimp.test;

import assimp.*;


import java.io.FileWriter;
import java.io.IOException;


/**
 * Example class to demonstrate how to use jAssimp to load an asset from
 * a file. The Main() method expects two parameters, the first being
 * the path to the file to be opened, the second being the output path.
 * The class writes a text file with the asset data inside.
 */
public class DumpToFile {

    public static void Main(String[] arguments) throws IOException {

        /* Use output.txt as default output file if none was specified
         * However, at least one parameter is expected
         */
        if (1 == arguments.length) {
            String s = arguments[0];
            arguments = new String[2];
            arguments[0] = s;
            arguments[1] = "output.txt";
        } else if (2 != arguments.length) return;

        FileWriter stream;
        try {
            stream = new FileWriter(arguments[1]);
        } catch (IOException e) {
            e.printStackTrace();
            System.out.println("Unable to open output file");
            throw e;
        }

        /* Try to create an instance of class assimp.Importer.
         * The constructor throws an assimp.NativeError exception
         * if the native jAssimp library is not available.It must
         * be placed in the jar/class directory of the application
         */
        Importer imp;
        try {
            imp = new Importer();
        } catch (NativeError nativeError) {
            nativeError.printStackTrace();
            System.out.println("NativeError exception [#1]: " + nativeError.getMessage());
            return;
        }

        /* Now setup the correct postprocess steps. Triangulation is
         * automatically performed, DX conversion is not necessary,
         * However, a few steps are normally useful. Especially
         * JoinVertices since it will dramantically reduce the size
         * of the output file
         */
        imp.addPostProcessStep(PostProcessStep.CalcTangentSpace);
        imp.addPostProcessStep(PostProcessStep.GenSmoothNormals);
        imp.addPostProcessStep(PostProcessStep.JoinIdenticalVertices);

        /* Load the asset into memory. Again, a NativeError exception
         * could be thrown if an unexpected errors occurs in the
         * native interface. If assimp is just unable to load the asset
         * null is the return value and no exception is thrown
         */
        Scene scene;
        try {
            scene = imp.readFile(arguments[0]);
        } catch (NativeError nativeError) {
            nativeError.printStackTrace();
            System.out.println("NativeError exception [#2] :" + nativeError.getMessage());
            return;
        }
        if (null == scene) {
            System.out.println("Unable to load asset: " + arguments[0]);
            return;
        }

        /* Now iterate through all meshes that have been loaded
         */
        int iMesh = 0;
        for (Mesh mesh : scene.getMeshes()) {

            stream.write("Mesh " + iMesh + "\n");
            stream.write("\tNum Vertices: " + mesh.getNumVertices() + "\n");
            stream.write("\tNum Faces: " + mesh.getNumFaces() + "\n");
            stream.write("\tNum Bones: " + mesh.getNumBones() + "\n\n");

            /* Output all vertices. First get direct access to jAssimp's buffers
             * UV coords and colors could also be there, but we don't output them
             * at the moment!
             */
            float[] positions = mesh.getPositionArray();
            float[] normals = mesh.getNormalArray();
            float[] tangents = mesh.getTangentArray();
            float[] bitangents = mesh.getBitangentArray();
            for (int i = 0; i < mesh.getNumVertices(); ++i) {

                // format: "Vertex pos(x|y|z) nor(x|y|z) tan(x|y|) bit(x|y|z)"
                // great that this IDE is automatically able to replace + with append ... ;-)
                stream.write(new StringBuilder().
                        append("\tVertex: pos(").
                        append(positions[i * 3]).append("|").
                        append(positions[i * 3 + 1]).append("|").
                        append(positions[i * 3 + 2]).append(")\t").
                        append("nor(").
                        append(normals[i * 3]).append("|").
                        append(normals[i * 3 + 1]).append("|").
                        append(normals[i * 3 + 2]).append(")\t").
                        append("tan(").
                        append(tangents[i * 3]).append("|").
                        append(tangents[i * 3 + 1]).append("|").
                        append(tangents[i * 3 + 2]).append(")\t").
                        append("bit(").
                        append(bitangents[i * 3]).append("|").
                        append(bitangents[i * 3 + 1]).append("|").
                        append(bitangents[i * 3 + 2]).append(")\n").toString());

            }
            stream.write("\n\n");

            /* Now write a list of all faces in this model
             */
            int[] faces = mesh.getFaceArray();
            for (int i = 0; i < mesh.getNumFaces(); ++i) {
                stream.write(new StringBuilder().append("\tFace (").
                        append(faces[i * 3]).append("|").
                        append(faces[i * 3 + 1]).append("|").
                        append(faces[i * 3 + 2]).append(")\n").toString());
            }

            ++iMesh;
        }


        stream.close();
    }
}
