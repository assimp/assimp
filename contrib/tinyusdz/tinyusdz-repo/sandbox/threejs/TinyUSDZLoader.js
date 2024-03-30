// Based on GLTFLoader and USDZLoader
// https://github.com/mrdoob/three.js/blob/master/examples/jsm/loaders/USDZLoader.js
// https://github.com/mrdoob/three.js/blob/master/examples/jsm/loaders/GLTFLoader.js

import {
	BufferAttribute,
	BufferGeometry,
	ClampToEdgeWrapping,
	FileLoader,
	Group,
	Loader,
	Mesh,
	MeshStandardMaterial,
	MirroredRepeatWrapping,
	RepeatWrapping,
	sRGBEncoding,
	TextureLoader,
	Object3D,
} from 'three';


///
/// Currently USDZ only.
/// Both USDC and USDA are supported as a USD content in USDZ. 
/// TODO: Add feature to directly load USDC and USDA
///
class TinyUSDZLoader extends Loader {

  constructor( manager ) {
    super( manager );
  }

  load(url, onLoad, onProgress, onError) {
    const scope = this;

    const loader = new FileLoader( scope.manager );
		loader.setPath( scope.path );
		loader.setResponseType( 'arraybuffer' );
		loader.setRequestHeader( scope.requestHeader );
		loader.setWithCredentials( scope.withCredentials );

    loader.load( url, function ( text ) {

			try {

				onLoad( scope.parse( text ) );

			} catch ( e ) {

				if ( onError ) {

					onError( e );

				} else {

					console.error( e );

				}

				scope.manager.itemError( url );

			}

		}, onProgress, onError );

  }

}

export { TinyUSDZLoader };
