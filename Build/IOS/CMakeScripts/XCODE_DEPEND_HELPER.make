# DO NOT EDIT
# This makefile makes sure all linkable targets are
# up-to-date with anything they link to
default:
	echo "Do not invoke directly"

# Rules to remove targets that are older than anything to which they
# link.  This forces Xcode to relink the targets from scratch.  It
# does not seem to check these dependencies itself.
PostBuild.IrrXML.Debug:
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/Debug/libIrrXML.a:
	/bin/rm -f /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/Debug/libIrrXML.a


PostBuild.assimp.Debug:
PostBuild.IrrXML.Debug: /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/Debug/libassimp.dylib
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/Debug/libassimp.dylib:\
	/usr/lib/libz.dylib\
	/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/Debug/libIrrXML.a
	/bin/rm -f /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/Debug/libassimp.dylib


PostBuild.IrrXML.Release:
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/Release/libIrrXML.a:
	/bin/rm -f /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/Release/libIrrXML.a


PostBuild.assimp.Release:
PostBuild.IrrXML.Release: /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/Release/libassimp.dylib
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/Release/libassimp.dylib:\
	/usr/lib/libz.dylib\
	/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/Release/libIrrXML.a
	/bin/rm -f /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/Release/libassimp.dylib


PostBuild.IrrXML.MinSizeRel:
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/MinSizeRel/libIrrXML.a:
	/bin/rm -f /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/MinSizeRel/libIrrXML.a


PostBuild.assimp.MinSizeRel:
PostBuild.IrrXML.MinSizeRel: /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/MinSizeRel/libassimp.dylib
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/MinSizeRel/libassimp.dylib:\
	/usr/lib/libz.dylib\
	/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/MinSizeRel/libIrrXML.a
	/bin/rm -f /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/MinSizeRel/libassimp.dylib


PostBuild.IrrXML.RelWithDebInfo:
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/RelWithDebInfo/libIrrXML.a:
	/bin/rm -f /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/RelWithDebInfo/libIrrXML.a


PostBuild.assimp.RelWithDebInfo:
PostBuild.IrrXML.RelWithDebInfo: /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/RelWithDebInfo/libassimp.dylib
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/RelWithDebInfo/libassimp.dylib:\
	/usr/lib/libz.dylib\
	/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/RelWithDebInfo/libIrrXML.a
	/bin/rm -f /Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/code/RelWithDebInfo/libassimp.dylib




# For each target create a dummy ruleso the target does not have to exist
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/Debug/libIrrXML.a:
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/MinSizeRel/libIrrXML.a:
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/RelWithDebInfo/libIrrXML.a:
/Users/m_lukyanov/GameDev/LIBS/Assimp/Build/IOS/contrib/irrXML/Release/libIrrXML.a:
/usr/lib/libz.dylib:
