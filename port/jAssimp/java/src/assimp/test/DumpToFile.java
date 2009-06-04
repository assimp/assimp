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
 * Example class to demonstrate how to use jAssimp to load an asset from a file.
 * The Main() method expects two parameters, the first being the path to the
 * file to be opened, the second being the output path. The class writes a text
 * file with the asset data inside.
 */
public class DumpToFile {

	/**
	 * Count all nodes recursively
	 * 
	 * @param node
	 *            Current node
	 * @return Number of nodes
	 */
	public static int CountNodes(Node node) {

		int ret = 1;
		if (0 != node.getNumChildren()) {
			for (Node n : node.getChildren()) {

				ret += CountNodes(n);
			}
		}
		return ret;
	}

	/**
	 * Entry point of the application
	 * 
	 * @param arguments
	 *            The first argument is the name of the mesh to be opened, the
	 *            second is the name of the primary output file.
	 * @throws IOException
	 */
	public static void main(String[] arguments) throws IOException {

		/*
		 * Use output.txt as default output file if none was specified.
		 */
		if (1 == arguments.length) {
			String s = arguments[0];
			arguments = new String[2];
			arguments[0] = s;
			arguments[1] = "output.txt";
		} else if (2 != arguments.length) {
			System.exit(-5);
		}

		if (!arguments[1].endsWith(".txt")) {
			System.out.println("The output file extension must be .txt");
			System.exit(-10);
			return;
		}

		FileWriter stream;
		try {
			stream = new FileWriter(arguments[1]);
		} catch (IOException e) {
			e.printStackTrace();
			System.out.println("Unable to open output file for writing");
			System.exit(-15);
			return;
		}

		/*
		 * Try to create an instance of class assimp.Importer. The constructor
		 * throws an assimp.NativeException exception if the native jAssimp
		 * library is not available.It must be placed in the jar/class directory
		 * of the application
		 */
		Importer imp;
		try {
			imp = new Importer();
		} catch (NativeException nativeException) {
			nativeException.printStackTrace();
			System.out.println("NativeException caught [#1]: "
					+ nativeException.getMessage());
			return;
		}

		/*
		 * Load the asset into memory. Again, a NativeException exception could
		 * be thrown if an unexpected errors occurs in the native interface. If
		 * assimp is just unable to load the asset null is the return value and
		 * no exception is thrown
		 */
		Scene scene;
		try {
			scene = imp.readFile(arguments[0],
					PostProcessing.Preset.TargetRealtime_MaxQuality);
		} catch (NativeException nativeException) {
			nativeException.printStackTrace();
			System.out.println("NativeException caught [#2] :"
					+ nativeException.getMessage());
			return;
		}
		if (null == scene) {
			System.out.println("Unable to load asset: " + arguments[0]);
			return;
		}

		/*
		 * Now iterate through all meshes that have been loaded
		 */
		if (0 != scene.getNumMeshes()) {
			for (Mesh mesh : scene.getMeshes()) {

				stream.write("Mesh\n");
				stream.write("\tNum Vertices: " + mesh.getNumVertices() + "\n");
				stream.write("\tNum Faces: " + mesh.getNumFaces() + "\n");
				stream.write("\tNum Bones: " + mesh.getNumBones() + "\n\n");

				/*
				 * Output all vertices. First get direct access to jAssimp's
				 * buffers
				 */
				float[] positions = mesh.getPositionArray();
				float[] normals = mesh.getNormalArray();
				float[] tangents = mesh.getTangentArray();
				float[] bitangents = mesh.getBitangentArray();

				float[][] uvs = new float[Mesh.MAX_NUMBER_OF_TEXTURECOORDS][];
				for (int i = 0; i < Mesh.MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
					if (mesh.hasUVCoords((i)))
						uvs[i] = mesh.getTexCoordArray(i);
					else
						break;
				}

				float[][] vcs = new float[Mesh.MAX_NUMBER_OF_COLOR_SETS][];
				for (int i = 0; i < Mesh.MAX_NUMBER_OF_COLOR_SETS; ++i) {
					if (mesh.hasVertexColors((i)))
						uvs[i] = mesh.getVertexColorArray(i);
					else
						break;
				}

				for (int i = 0; i < mesh.getNumVertices(); ++i) {

					// format:
					// "Vertex pos(x|y|z) nor(x|y|z) tan(x|y|) bit(x|y|z)"
					// great that this IDE is automatically able to replace +
					// with append ... ;-)
					if (mesh.hasPositions()) {
						stream.write(new StringBuilder().append(
								"\tVertex: pos(").append(positions[i * 3])
								.append("|").append(positions[i * 3 + 1])
								.append("|").append(positions[i * 3 + 2])
								.append(")").toString());
					}
					if (mesh.hasNormals()) {
						stream.write(new StringBuilder().append("\tnor(")
								.append(normals[i * 3]).append("|").append(
										normals[i * 3 + 1]).append("|").append(
										normals[i * 3 + 2]).append(")")
								.toString());
					}
					if (mesh.hasTangentsAndBitangents()) {
						stream.write(new StringBuilder().append("\ttan(")
								.append(tangents[i * 3]).append("|").append(
										tangents[i * 3 + 1]).append("|")
								.append(tangents[i * 3 + 2]).append(")")
								.toString());

						stream.write(new StringBuilder().append("\tbit(")
								.append(bitangents[i * 3]).append("|").append(
										bitangents[i * 3 + 1]).append("|")
								.append(bitangents[i * 3 + 2]).append(")")
								.toString());
					}

					for (int a = 0; i < Mesh.MAX_NUMBER_OF_TEXTURECOORDS; ++a) {
						if (!mesh.hasUVCoords((a)))
							break;

						stream.write(new StringBuilder().append("\tuv").append(
								a).append("(").append(uvs[a][i * 3])
								.append("|").append(uvs[a][i * 3 + 1]).append(
										"|").append(uvs[a][i * 3 + 2]).append(
										")").toString());
					}

					for (int a = 0; i < Mesh.MAX_NUMBER_OF_COLOR_SETS; ++a) {
						if (!mesh.hasVertexColors((a)))
							break;

						stream.write(new StringBuilder().append("\tcol")
								.append(a).append("(").append(vcs[a][i * 4])
								.append("|").append(vcs[a][i * 4 + 1]).append(
										"|").append(vcs[a][i * 4 + 2]).append(
										"|").append(vcs[a][i * 4 + 3]).append(
										")").toString());
					}
					stream.write("\n");
				}
				stream.write("\n");

				/*
				 * Now write a list of all faces in this model
				 */
				Face[] faces = mesh.getFaceArray();
				for (int i = 0; i < mesh.getNumFaces(); ++i) {
					stream.write(new StringBuilder().append("\tFace (").append(
							faces[i * 3]).append("|").append(faces[i * 3 + 1])
							.append("|").append(faces[i * 3 + 2]).append(")\n")
							.toString());
				}
				stream.write("\n");

				/*
				 * Now write a list of all bones of this model
				 */
				if (mesh.hasBones()) {
					Bone[] bones = mesh.getBonesArray();
					for (Bone bone : bones) {

						stream.write("\tBone " + bone.getName() + "\n");
						Bone.Weight[] weights = bone.getWeightsArray();
						for (Bone.Weight weight : weights) {
							stream.write("\t\tWeight (" + weight.index + "|"
									+ weight.weight + ")\n");
						}

					}
				}
				stream.write("\n");
			}
		}

		/*
		 * Now iterate through all animations that have been loaded
		 */
		if (0 != scene.getNumAnimations()) {
			for (Animation anim : scene.getAnimations()) {

				stream.write("Animation\n" + "\tName: " + anim.getName() + "\n"
						+ "\tDuration: " + anim.getDuration() + "\n"
						+ "\tTicks/s: " + anim.getTicksPerSecond() + "\n"
						+ "\tNum BoneAnim channels: "
						+ anim.getNumNodeAnimChannels() + "\n\n");

				/*
				 * Write all bone animation channels
				 */
				if (0 != anim.getNumNodeAnimChannels()) {
					for (NodeAnim boneAnim : anim.getNodeAnimChannels()) {

						stream.write("\tBoneAnim\n" + "\tName: "
								+ boneAnim.getName() + "\n"
								+ "\tNum QuatKeys: "
								+ boneAnim.getNumQuatKeys() + "\n");

						/*
						 * Write all rotation keys
						 */
						for (NodeAnim.KeyFrame<Quaternion> key : boneAnim
								.getQuatKeys()) {
							stream.write("\t\tQuatKey: \n" + "\t\t\tTicks: "
									+ key.time + "\n" + "\t\t\tValue: ("
									+ key.value.x + "|" + key.value.y + "|"
									+ key.value.z + "|" + key.value.w + ")\n");
						}
						stream.write("\tNum SclKeys: "
								+ boneAnim.getNumScalingKeys() + "\n");

						/*
						 * Write all scaling keys
						 */
						for (NodeAnim.KeyFrame<float[]> key : boneAnim
								.getScalingKeys()) {
							stream.write("\t\tSclKey: \n" + "\t\t\tTicks: "
									+ key.time + "\n" + "\t\t\tValue: ("
									+ key.value[0] + "|" + key.value[1] + "|"
									+ key.value[2] + ")\n");
						}
						stream.write("\tNum PosKeys: "
								+ boneAnim.getNumPosKeys() + "\n");

						/*
						 * Write all position keys
						 */
						for (NodeAnim.KeyFrame<float[]> key : boneAnim
								.getPosKeys()) {
							stream.write("\t\tPosKey: \n" + "\t\t\tTicks: "
									+ key.time + "\n" + "\t\t\tValue: ("
									+ key.value[0] + "|" + key.value[1] + "|"
									+ key.value[2] + ")\n");
						}
						stream.write("\n");
					}
				}
			}
		}

		/*
		 * Now print all nodes -> recursively
		 */
		stream.write("Nodegraph\n" + "\tNodes: "
				+ CountNodes(scene.getRootNode()) + "\n\n");
		// scene.getRootNode().printNodes( stream, "\t");
		stream.write("\n");

		/*
		 * Now print all textures .. hm ... export them to proper TGA files
		 */
		if (0 != scene.getNumTextures()) {
			int i = 0;
			for (Texture texture : scene.getTextures()) {

				String path = arguments[1].substring(0,
						arguments[1].length() - 4)
						+ "_tex" + i++ + ".tga";
				stream.write("Emb. Texture\n" + "\tExportPath: " + path
						+ "\n\n");

				Texture.SaveTextureToTGA(texture, path);
			}
		}

		/*
		 * Now print all materials
		 */

		// ...
		// close the stream again
		stream.close();
	}
}
