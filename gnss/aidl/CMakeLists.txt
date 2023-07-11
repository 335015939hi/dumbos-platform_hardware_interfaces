cmake_minimum_required(VERSION 3.18)

project(libbinder CXX)

set(CMAKE_CXX_STANDARD 20)

set(LIBBINDER_DIR "${ANDROID_BUILD_TOP}/frameworks/native/libs/binder")

add_subdirectory("${LIBBINDER_DIR}/linux" libbinder)

set(GNSS_CXX_FLAGS
    "-DNDEBUG"
    "-Wall"
    "-Wextra"
    "-fPIC"
#    "-Wno-attributes"
#    "-Wno-unknown-pragmas"
#    "-Wno-missing-field-initializers"
#    "-Wno-strict-aliasing"
#    "-Wno-unused-variable"
    "-Wno-deprecated"
#    "-Wno-sign-compare"
#    "-Wno-sequence-point"
#    "-Wno-pessimizing-move"
#    "-Wno-unused-result"
#    "-Wno-format"
#    "-Wno-psabi"
    "-Wno-reorder-init-list"
)

set(GNSS_AIDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(GNSS_AIDL_SRCS
    "android/hardware/gnss/measurement_corrections/IMeasurementCorrectionsCallback.aidl"
    "android/hardware/gnss/measurement_corrections/IMeasurementCorrectionsInterface.aidl"
    "android/hardware/gnss/measurement_corrections/MeasurementCorrections.aidl"
    "android/hardware/gnss/measurement_corrections/ReflectingPlane.aidl"
    "android/hardware/gnss/measurement_corrections/SingleSatCorrection.aidl"
    "android/hardware/gnss/visibility_control/IGnssVisibilityControl.aidl"
    "android/hardware/gnss/visibility_control/IGnssVisibilityControlCallback.aidl"
    "android/hardware/gnss/BlocklistedSource.aidl"
    "android/hardware/gnss/CorrelationVector.aidl"
    "android/hardware/gnss/ElapsedRealtime.aidl"
    "android/hardware/gnss/GnssClock.aidl"
    "android/hardware/gnss/GnssConstellationType.aidl"
    "android/hardware/gnss/GnssData.aidl"
    "android/hardware/gnss/GnssLocation.aidl"
    "android/hardware/gnss/GnssMeasurement.aidl"
    "android/hardware/gnss/GnssMultipathIndicator.aidl"
    "android/hardware/gnss/GnssPowerStats.aidl"
    "android/hardware/gnss/GnssSignalType.aidl"
    "android/hardware/gnss/IAGnss.aidl"
    "android/hardware/gnss/IAGnssCallback.aidl"
    "android/hardware/gnss/IAGnssRil.aidl"
    "android/hardware/gnss/IAGnssRilCallback.aidl"
    "android/hardware/gnss/IGnss.aidl"
    "android/hardware/gnss/IGnssAntennaInfo.aidl"
    "android/hardware/gnss/IGnssAntennaInfoCallback.aidl"
    "android/hardware/gnss/IGnssBatching.aidl"
    "android/hardware/gnss/IGnssBatchingCallback.aidl"
    "android/hardware/gnss/IGnssCallback.aidl"
    "android/hardware/gnss/IGnssConfiguration.aidl"
    "android/hardware/gnss/IGnssDebug.aidl"
    "android/hardware/gnss/IGnssGeofence.aidl"
    "android/hardware/gnss/IGnssGeofenceCallback.aidl"
    "android/hardware/gnss/IGnssMeasurementCallback.aidl"
    "android/hardware/gnss/IGnssMeasurementInterface.aidl"
    "android/hardware/gnss/IGnssNavigationMessageCallback.aidl"
    "android/hardware/gnss/IGnssNavigationMessageInterface.aidl"
    "android/hardware/gnss/IGnssPowerIndication.aidl"
    "android/hardware/gnss/IGnssPowerIndicationCallback.aidl"
    "android/hardware/gnss/IGnssPsds.aidl"
    "android/hardware/gnss/IGnssPsdsCallback.aidl"
    "android/hardware/gnss/PsdsType.aidl"
    "android/hardware/gnss/SatelliteClockInfo.aidl"
    "android/hardware/gnss/SatellitePositionEcef.aidl"
    "android/hardware/gnss/SatellitePvt.aidl"
    "android/hardware/gnss/SatelliteVelocityEcef.aidl"
)
set(GNSS_AIDL_NDK_GENDIR "${PROJECT_BINARY_DIR}/gens_ndk")
set(GNSS_AIDL_NDK_GEN_SRCS)
foreach(AIDL_FILE ${GNSS_AIDL_SRCS})
    get_filename_component(ABS_AIDL_FILEPATH "${GNSS_AIDL_DIR}/${AIDL_FILE}" ABSOLUTE)
    get_filename_component(AIDL_FILE_WE_DIR_TMP ${AIDL_FILE} DIRECTORY)
    get_filename_component(AIDL_FILE_WE_NAME_TMP ${AIDL_FILE} NAME_WE)
    set(AIDL_FILE_WE "${AIDL_FILE_WE_DIR_TMP}/${AIDL_FILE_WE_NAME_TMP}")
    #get_filename_component(ABS_AIDL_DIR ${ABS_AIDL_FILEPATH} DIRECTORY)
    set(ABS_AIDL_DIR "${GNSS_AIDL_DIR}")
    #get_filename_component(AIDL_FILENAME ${AIDL_FILE} NAME)
    #get_filename_component(AIDL_FILENAME_WE ${AIDL_FILE} NAME_WE)
    set(AIDL_FILENAME_WE "${AIDL_FILE_WE}")
    set(AIDL_CPP_FILE "${GNSS_AIDL_NDK_GENDIR}/${AIDL_FILENAME_WE}.cpp")
    add_custom_command(
        OUTPUT "${AIDL_CPP_FILE}"
        DEPENDS aidl
        COMMAND "${CMAKE_CURRENT_BINARY_DIR}/libbinder/aidl/aidl"
        ARGS
        --stability=vintf
        --lang=ndk
        -Weverything
        -Wno-missing-permission-annotation
        -Wno-mixed-oneway
        -Wno-redundant-name
        -Wno-enum-explicit-default
        -t
        --min_sdk_version current
        --structured
        --ninja
        -d "${GNSS_AIDL_NDK_GENDIR}/${AIDL_FILENAME_WE}.cpp.d"
        -h "${GNSS_AIDL_NDK_GENDIR}/include"
        -o "${GNSS_AIDL_NDK_GENDIR}"
        -I "${ABS_AIDL_DIR}"
        "${ABS_AIDL_FILEPATH}"
    )
list(APPEND GNSS_AIDL_NDK_GEN_SRCS "${AIDL_CPP_FILE}")
endforeach()

set(GNSS_AIDL_GENDIR "${PROJECT_BINARY_DIR}/gens_platform")
set(GNSS_AIDL_GEN_SRCS)
foreach(AIDL_FILE ${GNSS_AIDL_SRCS})
    get_filename_component(ABS_AIDL_FILEPATH "${GNSS_AIDL_DIR}/${AIDL_FILE}" ABSOLUTE)
    get_filename_component(AIDL_FILE_WE_DIR_TMP ${AIDL_FILE} DIRECTORY)
    get_filename_component(AIDL_FILE_WE_NAME_TMP ${AIDL_FILE} NAME_WE)
    set(AIDL_FILE_WE "${AIDL_FILE_WE_DIR_TMP}/${AIDL_FILE_WE_NAME_TMP}")
    #get_filename_component(ABS_AIDL_DIR ${ABS_AIDL_FILEPATH} DIRECTORY)
    set(ABS_AIDL_DIR "${GNSS_AIDL_DIR}")
    #get_filename_component(AIDL_FILENAME ${AIDL_FILE} NAME)
    #get_filename_component(AIDL_FILENAME_WE ${AIDL_FILE} NAME_WE)
    set(AIDL_FILENAME_WE "${AIDL_FILE_WE}")
    set(AIDL_CPP_FILE "${GNSS_AIDL_GENDIR}/${AIDL_FILENAME_WE}.cpp")
    add_custom_command(
        OUTPUT "${AIDL_CPP_FILE}"
        DEPENDS aidl
        COMMAND "${CMAKE_CURRENT_BINARY_DIR}/libbinder/aidl/aidl"
        ARGS
        --stability=vintf
        --lang=cpp
        -Weverything
        -Wno-missing-permission-annotation
        -Wno-mixed-oneway
        -Wno-redundant-name
        -Wno-enum-explicit-default
        -t
        --min_sdk_version current
        --structured
        --ninja
        -d "${GNSS_AIDL_GENDIR}/${AIDL_FILENAME_WE}.cpp.d"
        -h "${GNSS_AIDL_GENDIR}/include"
        -o "${GNSS_AIDL_GENDIR}"
        -I "${ABS_AIDL_DIR}"
        "${ABS_AIDL_FILEPATH}"
    )
list(APPEND GNSS_AIDL_GEN_SRCS "${AIDL_CPP_FILE}")
endforeach()

add_library(android_gnss_aidl_ndk ${GNSS_AIDL_NDK_GEN_SRCS})
target_compile_options(android_gnss_aidl_ndk
    PRIVATE
    ${GNSS_CXX_FLAGS}
)
target_include_directories(android_gnss_aidl_ndk
    PUBLIC
    "${GNSS_AIDL_NDK_GENDIR}/include"
)
target_link_libraries(android_gnss_aidl_ndk
    android_libbinder
    android_libbinder_ndk_hdrs
    android_libandroid
)

add_library(android_gnss_aidl ${GNSS_AIDL_GEN_SRCS})
target_compile_options(android_gnss_aidl
    PRIVATE
    ${GNSS_CXX_FLAGS}
)
target_include_directories(android_gnss_aidl
    PUBLIC
    "${GNSS_AIDL_GENDIR}/include"
)
target_link_libraries(android_gnss_aidl
    android_libbinder
    android_libbinder_ndk_hdrs
    android_libandroid
)


set(GNSS_SERVICE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/default")
set(GNSS_SERVICE_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common/utils/default")
set(GNSS_SERVICE_SRCS
    "${GNSS_SERVICE_DIR}/AGnss.cpp"
    "${GNSS_SERVICE_DIR}/AGnssRil.cpp"
    "${GNSS_SERVICE_DIR}/GnssAntennaInfo.cpp"
    "${GNSS_SERVICE_DIR}/GnssBatching.cpp"
    "${GNSS_SERVICE_DIR}/GnssConfiguration.cpp"
    "${GNSS_SERVICE_DIR}/Gnss.cpp"
    "${GNSS_SERVICE_DIR}/GnssDebug.cpp"
    "${GNSS_SERVICE_DIR}/GnssGeofence.cpp"
    "${GNSS_SERVICE_DIR}/GnssMeasurementInterface.cpp"
    "${GNSS_SERVICE_DIR}/GnssNavigationMessageInterface.cpp"
    "${GNSS_SERVICE_DIR}/GnssPowerIndication.cpp"
    "${GNSS_SERVICE_DIR}/GnssPsds.cpp"
    "${GNSS_SERVICE_DIR}/GnssVisibilityControl.cpp"
    "${GNSS_SERVICE_DIR}/MeasurementCorrectionsInterface.cpp"
    "${GNSS_SERVICE_DIR}/service.cpp"
    "${GNSS_SERVICE_COMMON_DIR}/DeviceFileReader.cpp"
    "${GNSS_SERVICE_COMMON_DIR}/FixLocationParser.cpp"
    "${GNSS_SERVICE_COMMON_DIR}/GnssRawMeasurementParser.cpp"
    "${GNSS_SERVICE_COMMON_DIR}/GnssReplayUtils.cpp"
    "${GNSS_SERVICE_COMMON_DIR}/MockLocation.cpp"
    "${GNSS_SERVICE_COMMON_DIR}/NmeaFixInfo.cpp"
    "${GNSS_SERVICE_COMMON_DIR}/ParseUtils.cpp"
    "${GNSS_SERVICE_COMMON_DIR}/Utils.cpp"
)
add_executable(android_gnss_service ${GNSS_SERVICE_SRCS})
target_include_directories(android_gnss_service
    PRIVATE
    "${GNSS_SERVICE_COMMON_DIR}/include"
)
target_link_libraries(android_gnss_service
    android_gnss_aidl_ndk
    android_libbinder_ndk
    android_libandroid
)
target_compile_options(android_gnss_service
    PRIVATE
    ${GNSS_CXX_FLAGS}
)

if(false)
cmake_policy(SET CMP0012 NEW)
find_package(GTest REQUIRED)
set(GNSS_VTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vts")
set(GNSS_VTS_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common/utils/vts")
set(GNSS_VTS_SRCS
    "${GNSS_VTS_DIR}/gnss_hal_test.cpp"
    "${GNSS_VTS_DIR}/gnss_hal_test_cases.cpp"
    "${GNSS_VTS_DIR}/AGnssCallbackAidl.cpp"
    "${GNSS_VTS_DIR}/AGnssRilCallbackAidl.cpp"
    "${GNSS_VTS_DIR}/GnssAntennaInfoCallbackAidl.cpp"
    "${GNSS_VTS_DIR}/GnssBatchingCallback.cpp"
    "${GNSS_VTS_DIR}/GnssCallbackAidl.cpp"
    "${GNSS_VTS_DIR}/GnssGeofenceCallback.cpp"
    "${GNSS_VTS_DIR}/GnssMeasurementCallbackAidl.cpp"
    "${GNSS_VTS_DIR}/GnssNavigationMessageCallback.cpp"
    "${GNSS_VTS_DIR}/GnssPowerIndicationCallback.cpp"
    "${GNSS_VTS_DIR}/GnssVisibilityControlCallback.cpp"
    "${GNSS_VTS_DIR}/MeasurementCorrectionsCallback.cpp"
    "${GNSS_VTS_DIR}/VtsHalGnssTargetTest.cpp"
)
add_executable(android_gnss_vts ${GNSS_VTS_SRCS})
target_include_directories(android_gnss_vts
    PRIVATE
    "${GNSS_VTS_COMMON_DIR}/include"
    "${ANDROID_BUILD_TOP}/system/libvintf/libaidlvintf_test_helper/include/"
)
target_link_libraries(android_gnss_vts
    android_gnss_aidl
    GTest::gtest
)
target_compile_options(android_gnss_vts
    PRIVATE
    ${GNSS_CXX_FLAGS}
)
endif()
