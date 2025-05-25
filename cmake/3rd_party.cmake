include(FetchContent)

FetchContent_Declare(
    openal
    GIT_REPOSITORY  https://github.com/kcat/openal-soft.git
    GIT_TAG         1.24.3
    GIT_SHALLOW     ON
)

FetchContent_Declare(
    libogg
    GIT_REPOSITORY  https://github.com/xiph/ogg.git
    GIT_TAG         fa80aae9d50096160f2b56ada35527d7aee3f746
    GIT_SHALLOW     ON
)

# Support old cmake files
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_Declare(
    libvorbis
    GIT_REPOSITORY  https://github.com/xiph/vorbis.git
    GIT_TAG         84c023699cdf023a32fa4ded32019f194afcdad0
    GIT_SHALLOW     ON
)

if (WIN32)
    FetchContent_MakeAvailable(libogg libvorbis openal)
elseif(LINUX)
    include(FindPkgConfig)
    # On Linux use package manager
    pkg_check_modules(OGG REQUIRED ogg)
    pkg_check_modules(VORBISFILE REQUIRED vorbisfile)
    pkg_check_modules(OPENAL REQUIRED openal)
endif()

add_library(storm-audio-deps INTERFACE)
target_link_libraries(storm-audio-deps
    INTERFACE
        $<$<PLATFORM_ID:Windows>:Ogg::ogg Vorbis::vorbisfile OpenAL>
	$<$<PLATFORM_ID:Linux>:${OGG_LIBRARIES} ${VORBISFILE_LIBRARIES} ${OPENAL_LIBRARIES}>
)
target_include_directories(storm-audio-deps
    INTERFACE
    $<$<PLATFORM_ID:Linux>:${OGG_INCLUDE_DIRS} ${VORBISFILE_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIRS}>
)
