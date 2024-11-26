cmake_minimum_required (VERSION 3.20)


project (oatpp-s2t)

set (oatpp_s2t_oatpp_VERSION_MAJOR 0)
set (oatpp_s2t_VERSION_MINOR 1)
set (oatpp_s2t_VERSION_REVISION 1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_VERBOSE_MAKEFILE OFF)


################################################################################
# Cmake modules
################################################################################

#Including local extra cmake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

#Function for generation of export macros for libraries
include(GenerateExportHeader) #(toffy_export.h - TOFFY_EXPORT)

# Include module with fuction 'write_basic_package_version_file' for ToffyConfig.cmake
include(CMakePackageConfigHelpers)

# Build dir for generated config, export and package files
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/generated")


################################################################################
# Install options
################################################################################
MESSAGE(STATUS "CMAKE_INSTALL_PREFIX: " ${CMAKE_INSTALL_PREFIX})

set(INSTALL_LIB_DIR lib/ CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin/ CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include/${PROJECT_NAME} CACHE PATH "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR cmake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/cmake/${PROJECT_NAME})
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

################################################################################
#Version stuff
################################################################################

# Make a version file containing the current version from git.
include(GetGitRevisionDescription)
git_describe(VERSION --tag --long)

##parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" PROJECT_VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" PROJECT_VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" PROJECT_VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" PROJECT_VERSION_TWEAK "${VERSION}")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(PROJECT_DESCRIPTION "Toffy 3D computer vision framework")

set(PROJECT_VERSION_FULL "${PROJECT_VERSION}${PROJECT_VERSION_TWEAK}")

message (STATUS "version: " ${VERSION} )
message (STATUS ${PROJECT_NAME}" version: " ${PROJECT_VERSION_FULL} )

# Generate Config file y_config.h
set(config_h_in "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}_config.h.in")
set(config_h "${generated_dir}/include/${PROJECT_NAME}/${PROJECT_NAME}_config.h")
configure_file("${config_h_in}" "${config_h}")

list(APPEND headers ${config_h})
install(
   FILES ${config_h}
   DESTINATION "${INSTALL_INCLUDE_DIR}/common"
)

################################################################################
#
################################################################################

#
# needed for architecture-specific extensions like gpio trigger etc.
#
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
    add_definitions(-DLINUX=1 )
endif()


# Boost
find_package(Boost COMPONENTS log_setup log program_options system
        filesystem date_time serialization chrono thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LIBS ${Boost_LIBRARIES} -lm)

add_definitions(-DBOOST_ALL_DYN_LINK )


# OAT++
find_package(oatpp          1.4.0 REQUIRED)
find_package(oatpp-swagger  1.4.0 REQUIRED)

include_directories("${oatpp-swagger_INCLUDE_DIRS}" ${oatpp_INCLUDE_DIRS})

list(APPEND LIBS oatpp::oatpp  oatpp::oatpp-test oatpp::oatpp-swagger)
message("OAT INC " ${oatpp_INCLUDE_DIRS})
message("OAT INC " ${oatpp-swagger_INCLUDE_DIRS})

add_definitions(
        ## define path to swagger-ui static resources folder
        -DOATPP_SWAGGER_RES_PATH="${oatpp-swagger_INCLUDE_DIRS}/../bin/oatpp-swagger/res"
)

################################################################################

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )

# if(CMAKE_BUILD_TYPE MATCHES Release)
#     message("RELEASE BUILD")
#     add_definitions(-O2 -funroll-loops )
# endif()

add_definitions(-Wall -ggdb -O0 )
# add_definitions( -Weffc++ )

add_subdirectory(src)
