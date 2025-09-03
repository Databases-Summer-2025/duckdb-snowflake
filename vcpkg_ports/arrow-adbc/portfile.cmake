vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO apache/arrow-adbc
    REF apache-arrow-adbc-18
    SHA512 c65e3b8e33e09c0fcaf69bfddb87c5f87f5bb7beea97ce64e0a973833c8b4e6adc2a88e3f3fe456db5d35c6c38a8a2c8e06bb032e71f60a4bb24e19837a4ea67
    HEAD_REF main
)

# Windows-specific: Set Arrow paths explicitly
if(VCPKG_TARGET_IS_WINDOWS)
    set(ARROW_PREFIX_PATH "${CURRENT_INSTALLED_DIR}")
    set(CMAKE_PREFIX_PATH "${CURRENT_INSTALLED_DIR};${CMAKE_PREFIX_PATH}")
    
    # Find Arrow package
    find_package(Arrow CONFIG REQUIRED PATHS "${CURRENT_INSTALLED_DIR}" NO_DEFAULT_PATH)
    
    # Set Arrow variables for ADBC
    set(ARROW_INCLUDE_DIR "${CURRENT_INSTALLED_DIR}/include")
    set(ARROW_LIB_DIR "${CURRENT_INSTALLED_DIR}/lib")
    
    # Additional Windows-specific options
    set(ADBC_CMAKE_OPTIONS
        -DADBC_DRIVER_SNOWFLAKE=ON
        -DADBC_BUILD_TESTS=OFF
        -DADBC_BUILD_BENCHMARKS=OFF
        -DADBC_BUILD_EXAMPLES=OFF
        -DADBC_BUILD_INTEGRATION=OFF
        -DArrow_DIR="${CURRENT_INSTALLED_DIR}/share/arrow"
        -DARROW_PREFIX_PATH="${CURRENT_INSTALLED_DIR}"
        -DCMAKE_PREFIX_PATH="${CURRENT_INSTALLED_DIR}"
    )
else()
    set(ADBC_CMAKE_OPTIONS
        -DADBC_DRIVER_SNOWFLAKE=ON
        -DADBC_BUILD_TESTS=OFF
        -DADBC_BUILD_BENCHMARKS=OFF
        -DADBC_BUILD_EXAMPLES=OFF
        -DADBC_BUILD_INTEGRATION=OFF
    )
endif()

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        postgresql  ADBC_DRIVER_POSTGRESQL
        sqlite      ADBC_DRIVER_SQLITE
        flightsql   ADBC_DRIVER_FLIGHTSQL
        snowflake   ADBC_DRIVER_SNOWFLAKE
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/c"
    OPTIONS 
        ${ADBC_CMAKE_OPTIONS}
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/AdbcDriverManager)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Copy the Snowflake driver library to the correct location
if(VCPKG_TARGET_IS_WINDOWS)
    if(EXISTS "${CURRENT_PACKAGES_DIR}/lib/adbc_driver_snowflake.dll")
        file(COPY "${CURRENT_PACKAGES_DIR}/lib/adbc_driver_snowflake.dll" 
             DESTINATION "${CURRENT_PACKAGES_DIR}/bin")
    endif()
    if(EXISTS "${CURRENT_PACKAGES_DIR}/debug/lib/adbc_driver_snowflake.dll")
        file(COPY "${CURRENT_PACKAGES_DIR}/debug/lib/adbc_driver_snowflake.dll" 
             DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin")
    endif()
endif()

vcpkg_copy_pdbs()

file(INSTALL "${SOURCE_PATH}/LICENSE.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)