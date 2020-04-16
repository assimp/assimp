# Compile internal irrXML only if system is not requested
if( NOT ASSIMP_SYSTEM_IRRXML )
    add_subdirectory(irrXML)
endif()
