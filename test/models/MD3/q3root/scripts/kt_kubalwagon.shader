//KubalWagon - texture 'culled' so the mudguard and Steering wheel
//have a visible underside

// --- modified textures so now only applied to smaller tga --- //

models/mapobjects/kt_kubalwagon/euro_frnt_2
{
    	cull disable
//	surfaceparm playerclip
    
        {
                map models/mapobjects/kt_kubalwagon/euro_frnt_2.tga
                alphaFunc GE128
		depthWrite
		rgbGen vertex
        }


}

// --- ORIGINAL SCRIPT --- //

//models/mapobjects/kt_kubalwagon/european_fnt
//{
//    	cull disable
//	surfaceparm playerclip
//    
//        {
//                map models/mapobjects/kt_kubalwagon/european_fnt.tga
//                alphaFunc GE128
//		depthWrite
//		rgbGen vertex
//        }
//
//
//}