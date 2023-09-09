class mar expands Actor;

#exec MESH IMPORT MESH=mar ANIVFILE=MODELS\mar_rifle_a.3d DATAFILE=MODELS\mar_rifle_d.3d X=0 Y=0 Z=0
#exec MESH ORIGIN MESH=mar X=0 Y=0 Z=0

#exec MESH SEQUENCE MESH=mar SEQ=All STARTFRAME=0 NUMFRAMES=30
//#exec MESH SEQUENCE MESH=mar SEQ=??? STARTFRAME=0 NUMFRAMES=30

#exec MESHMAP NEW MESHMAP=mar MESH=mar
#exec MESHMAP SCALE MESHMAP=mar X=0.1 Y=0.1 Z=0.2

#exec TEXTURE IMPORT NAME=Jtex1 FILE=m_rifl.bmp GROUP=Skins FLAGS=2
#exec TEXTURE IMPORT NAME=Jtex1 FILE=texture1.pcx GROUP=Skins PALETTE=Jtex1
#exec MESHMAP SETTEXTURE MESHMAP=mar NUM=1 TEXTURE=Jtex1

defaultproperties
{
    DrawType=DT_Mesh
    Mesh=mar
}
