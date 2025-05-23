include(FetchContent)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
    GIT_TAG         release-2.32.6
    GIT_SHALLOW     ON
)

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
    FetchContent_MakeAvailable(SDL2 libogg libvorbis openal)
elseif(LINUX)
    # On Linux use package manager
    find_package(SDL2 REQUIRED)
    find_package(OGG REQUIRED)
    find_package(VORBIS REQUIRED)
    find_package(OpenAL REQUIRED)
endif()

add_library(storm-audio-deps INTERFACE)
target_link_libraries(storm-audio-deps
    INTERFACE
        $<$<PLATFORM_ID:Windows>:SDL2::SDL2 Ogg::ogg Vorbis::vorbisfile OpenAL>
        $<$<PLATFORM_ID:Linux>:${SDL2_LIBRARIES} ${OGG_LIBRARIES} ${VORBIS_LIBRARIES} ${OpenAL_LIBRARIES}>
)
target_include_directories(storm-audio-deps
    INTERFACE
        $<$<PLATFORM_ID:Linux>:${SDL2_INCLUDE_DIRS} ${OGG_INCLUDE_DIRS} ${VORBIS_INCLUDE_DIRS} ${OpenAL_INCLUDE_DIRS}>
)
