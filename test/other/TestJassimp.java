import jassimp.*;
import java.io.*;

/*
   System.setProperty(“java.library.path”, “/path/to/library”);
// or from cmdline
// java -Djava.library.path=<path_to_dll> <main_class>
*/
class TestJassimp{
	static{
		System.loadLibrary("assimp");
	}

	static AiScene mScene;
	private static AiWrapperProvider<?, ?, ?, ?, ?> mDefaultWrapper;

	public static void main(String[] args){
		try{
			mScene = Jassimp.importFile(args[0]);
		}catch(IOException e){
			System.out.println("Error loading "+args[0]+" dude.");
		} finally {
			mDefaultWrapper = Jassimp.getWrapperProvider();
		}
		doDumpScene(mScene);
	}

	public static void doDumpScene(AiScene scene) {
		System.out.println("Start Dump Scene");
		if (scene == null)
			return;

		System.out.println(String.format("Scene has %s mesh(es)", scene.getNumMeshes()));

		for (AiMesh mesh : scene.getMeshes()) {
			System.out.println(mesh.getName() +"\n "+ mesh.toString());
			//System.out.println("Mesh Positions");
			//JaiDebug.dumpPositions(mesh);
			//System.out.println("Mesh Faces");
			//JaiDebug.dumpFaces(mesh);
			for (AiCamera camera : scene.getCameras()) {
				System.out.println(String.format("Scene camera %s [%s]", camera.getName(), camera.getPosition(mDefaultWrapper).toString()));
				//System.out.println(String.format("Scene camera %s [%s]", camera.getName(), camera.getPosition(mWrapper).toString()));
			}
			for (AiLight light : scene.getLights()) {
				//System.out.println(String.format("Scene light %s [%s]", light.getName(), light.getPosition(mWrapper).toString()));
				System.out.println(String.format("Scene light %s [%s]", light.getName(), light.getPosition(mDefaultWrapper).toString()));
				System.out.println(String.format("Scene light %s [%s]", light.getName(), light.getColorDiffuse(mDefaultWrapper).toString()));
			}
			AiMaterial material = scene.getMaterials().get(mesh.getMaterialIndex());
			JaiDebug.dumpMaterial(material);
			// Vector4f color = material.getReflectiveColor(mWrapper);
			// System.out.println(String.format("material %s has color %s",
			// material.getName(), color.toString()));
		}

		AiNode rootNode = (AiNode)scene.getSceneRoot(mDefaultWrapper);
		for(AiNode node : rootNode.getChildren()){
			System.out.println("Node: " + node.getName());
				System.out.println("Num Meshes: " + node.getNumMeshes());
				int[] meshArr = node.getMeshes();
				for (int i = 0; i < node.getNumMeshes(); i++) {
					System.out.println("   Mesh "+i+": " + scene.getMeshes().get(meshArr[i]).getName());
				}
			AiMatrix4f mat = (AiMatrix4f) node.getTransform(mDefaultWrapper);
			if("Camera".equals(node.getName())){
				System.out.println("Camera");
			}
			else if ("Lamp".equals(node.getName())){
				System.out.println("Light");
			}
			else{
				System.out.println("some Object");
			}
			System.out.println(String.format("Node Transform\n%s\n", mat.toString()));
		}
			for(AiAnimation anim : scene.getAnimations()){
				System.out.println("Animation: " + anim.getName());
				for(AiNodeAnim nanim : anim.getChannels()){
					System.out.println(nanim.getNodeName());
					JaiDebug.dumpNodeAnim(nanim);
				}
			}	
	}
}
